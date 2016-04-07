#include "thread_regs.h"
#include "threads.h"
#include "memory.h"
#include "kernel.h"
#include "string.h"
#include "paging.h"
#include "error.h"
#include "stdio.h"
#include "time.h"
#include "mm.h"

#include <stdint.h>


#ifndef CONFIG_KERNEL_STACK
#define KERNEL_STACK_ORDER 1
#else
#define KERNEL_STACK_ORDER CONFIG_KERNEL_STACK
#endif

struct switch_stack_frame {
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t rbx;
	uint64_t rbp;
	uint64_t entry;
} __attribute__((packed));

struct thread_start_frame {
	struct switch_stack_frame frame;
	struct thread_regs regs;
} __attribute__((packed));

struct thread_iter {
	struct rb_node *parent;
	struct rb_node **plink;
	struct thread *thread;
};

#define IO_MAP_BITS  BIT_CONST(16)
#define IO_MAP_WORDS (IO_MAP_BITS / sizeof(unsigned long))

struct tss {
	uint32_t rsrv0;
	uint64_t rsp[3];
	uint64_t rsrv1;
	uint64_t ist[7];
	uint64_t rsrv2;
	uint16_t rsrv3;
	uint16_t iomap_base;
	unsigned long iomap[IO_MAP_WORDS + 1];
} __attribute__((packed));

static struct thread *current_thread;
static struct thread bootstrap;
static DEFINE_SPINLOCK(threads_lock);
static struct rb_tree threads;
static struct scheduler *scheduler;

static void check_stack(struct thread *thread)
{
	const size_t stack_order = KERNEL_STACK_ORDER;
	const size_t stack_pages = 1ul << stack_order;
	const size_t stack_size = PAGE_SIZE * stack_pages;
	const char *begin = page_addr(thread->stack);

	if (((char *)thread->stack_pointer < begin) ||
			((char *)thread->stack_pointer >= begin + stack_size)) {
		DBG_ERR("thread %p out of stack", thread);
		DBG_ERR("\tstack_pointer %p", thread->stack_pointer);
		DBG_ERR("\tstack [%p-%p]", begin, begin + stack_size);
		while (1);
	}
}

void idle(void)
{ while (1) schedule(); }

static void preempt_thread(struct thread *thread)
{
	if (thread == &bootstrap)
		return;

	if (scheduler->preempt)
		scheduler->preempt(thread);
}

static size_t thread_stack_size(void)
{
	return 1ul << (KERNEL_STACK_ORDER + PAGE_BITS);
}

void *thread_stack_begin(struct thread *thread)
{
	extern char init_stack_bottom[];

	if (thread == &bootstrap)
		return init_stack_bottom;
	return page_addr(thread->stack);
}

void *thread_stack_end(struct thread *thread)
{
	extern char init_stack_top[];

	if (thread == &bootstrap)
		return init_stack_top;
	return (char *)thread_stack_begin(thread) + thread_stack_size();
}

static void place_thread(struct thread *thread)
{
	struct thread *prev = current_thread;

	if (prev != &bootstrap)
		check_stack(prev);

	current_thread = thread;

	store_pml4(page_paddr(current_thread->mm->pt));

	if (prev->state == THREAD_FINISHED)
		prev->state = THREAD_DEAD;

	if (thread == &bootstrap)
		return;

	if (scheduler->place)
		scheduler->place(thread);

	thread->time = jiffies;
}

int thread_entry(struct thread *thread, int (*fptr)(void *),
			void *arg)
{
	place_thread(thread);
	local_preempt_enable();

	if (fptr)
		return fptr(arg);
	return 0;
}

static int __lookup_thread(struct thread_iter *iter, pid_t pid)
{
	struct rb_node **plink = &threads.root;
	struct rb_node *parent = 0;

	while (*plink) {
		struct thread *t = TREE_ENTRY(*plink, struct thread, node);
		const pid_t p = thread_pid(t);

		if (p == pid) {
			iter->parent = parent;
			iter->plink = plink;
			iter->thread = t;
			return 1;
		}

		parent = *plink;
		if (p < pid)
			plink = &parent->right;
		else
			plink = &parent->left;
	}

	iter->parent = parent;
	iter->plink = plink;
	iter->thread = 0;
	return 0;
}

struct thread *lookup_thread(pid_t pid)
{
	struct thread_iter iter;
	const bool enabled = spin_lock_irqsave(&threads_lock);

	if (__lookup_thread(&iter, pid))
		get_thread(iter.thread);
	spin_unlock_irqrestore(&threads_lock, enabled);

	return iter.thread;
}

static void register_thread(struct thread *thread)
{
	static pid_t next_pid;
	struct thread_iter iter;

	const bool enabled = spin_lock_irqsave(&threads_lock);

	thread->pid = next_pid++;

	while (__lookup_thread(&iter, thread_pid(thread)))
		thread->pid = next_pid++;

	rb_link(&thread->node, iter.parent, iter.plink);
	rb_insert(&thread->node, &threads);
	spin_unlock_irqrestore(&threads_lock, enabled);
}

static struct thread *alloc_thread(void)
{
	const size_t stack_order = KERNEL_STACK_ORDER;
	const size_t stack_pages = (size_t)1 << stack_order;
	const size_t stack_size = stack_pages << PAGE_BITS;

	struct page *stack = alloc_pages(stack_order);

	if (!stack)
		return 0;

	struct thread *thread = scheduler->alloc();

	if (!thread) {
		free_pages(stack, stack_order);
		return 0;
	}

	thread->mm = create_mm();
	if (!thread->mm) {
		scheduler->free(thread);
		free_pages(stack, stack_order);
		return 0;
	}

	spinlock_init(&thread->lock);
	thread->refcount = 1; // one for wait
	thread->stack = stack;
	thread->state = THREAD_BLOCKED;
	thread->pid = -1;

	memset(thread_stack_begin(thread), 0, stack_size);
	thread->stack_pointer = thread_stack_end(thread);

	return thread;
}

static pid_t __create_thread(int (*fptr)(void *), void *arg)
{
	const size_t frame_size = sizeof(struct thread_start_frame);
	extern void __thread_entry(void);

	struct thread *thread = alloc_thread();

	if (!thread)
		return -ENOMEM;

	struct thread_start_frame *frame =
		(void *)((char *)thread_stack_end(thread) - frame_size);

	/* all kernel threads start in kernel_thread_entry */
	frame->frame.entry = (uint64_t)&__thread_entry;

	/* kernel_thread_entry arguments */
	frame->frame.r15 = (uint64_t)thread;
	frame->frame.r14 = (uint64_t)fptr;
	frame->frame.r13 = (uint64_t)arg;

	/*
	 * after thread_entry finished we stil have thread_regs on the stack,
	 * initialize it to iret to exit. Thread function can overwrite it to
	 * jump in userspace.
	 */
	frame->regs.ss = (uint64_t)KERNEL_DATA;
	frame->regs.cs = (uint64_t)KERNEL_CODE;
	frame->regs.rsp = (uint64_t)(thread_stack_end(thread));
	frame->regs.rip = (uint64_t)&exit; // finish thread on exit

	thread->stack_pointer = frame;
	register_thread(thread);

	return thread_pid(thread);
}

pid_t create_kthread(int (*fptr)(void *), void *arg)
{
	const pid_t pid = __create_thread(fptr, arg);

	if (pid < 0)
		return pid;

	struct thread *thread = lookup_thread(pid);

	DBG_ASSERT(thread != 0);

	activate_thread(thread);
	put_thread(thread);
	return pid;
}

/*
 * every thread in kernel except dying one has thread_regs at the top of
 * the stack - this is contract!!!
 */
struct thread_regs *thread_regs(struct thread *thread)
{
	char *stack_end = thread_stack_end(thread);

	return (void *)(stack_end - sizeof(struct thread_regs));
}

static void __wait_thread(struct thread *thread)
{
	while (thread->state != THREAD_DEAD) {
		barrier();
		schedule();
	}
}

static int reap_thread(struct thread *thread)
{
	bool locked = spin_lock_irqsave(&thread->lock);
	const int state = thread->state;

	thread->state = THREAD_REAPED;
	spin_unlock_irqrestore(&thread->lock, locked);

	if (state == THREAD_REAPED)
		return 0;

	locked = spin_lock_irqsave(&threads_lock);
	rb_erase(&thread->node, &threads);
	spin_unlock_irqrestore(&threads_lock, locked);	
	put_thread(thread);

	return 1;
}

int wait(pid_t pid)
{
	struct thread *thread = lookup_thread(pid);

	if (!thread)
		return -ENOENT;

	__wait_thread(thread);

	const int rc = reap_thread(thread) ? 0 : -ENOENT;

	put_thread(thread);

	return rc;
}

void activate_thread(struct thread *thread)
{
	const bool locked = spin_lock_irqsave(&thread->lock);

	DBG_ASSERT(thread != &bootstrap);

	if (thread->state == THREAD_BLOCKED) {
		thread->state = THREAD_ACTIVE;
		scheduler->activate(thread);
	}
	spin_unlock_irqrestore(&thread->lock, locked);
}

void exit(void)
{
	local_preempt_disable();
	current_thread->state = THREAD_FINISHED;
	schedule();
	DBG_ASSERT(0 && "Unreachable");
}

struct thread *current(void)
{ return current_thread; }

static void release_thread(struct thread *thread)
{
	release_mm(thread->mm);
	free_pages(thread->stack, KERNEL_STACK_ORDER);
	scheduler->free(thread);
}

void get_thread(struct thread *thread)
{
	const bool locked = spin_lock_irqsave(&thread->lock);

	++thread->refcount;
	spin_unlock_irqrestore(&thread->lock, locked);
}

void put_thread(struct thread *thread)
{
	const bool locked = spin_lock_irqsave(&thread->lock);
	const int refcount = --thread->refcount;

	spin_unlock_irqrestore(&thread->lock, locked);

	if (!refcount)
		release_thread(thread);
}

static void switch_to(struct thread *next)
{
	struct thread *prev = current_thread;

	void switch_threads(void **prev, void *next);

	preempt_thread(prev);
	switch_threads(&prev->stack_pointer, next->stack_pointer);
	place_thread(prev);
}

static struct thread *next_thread(void)
{ return scheduler->next(); }

void schedule(void)
{
	const bool enabled = local_preempt_save();
	struct thread *thread = next_thread();

	if (thread == current_thread) {
		local_preempt_restore(enabled);
		return;
	}
	
	const bool force = (current_thread->state != THREAD_ACTIVE);

	if (!force && !thread) {
		local_preempt_restore(enabled);
		return;
	}

	switch_to(thread ? thread : &bootstrap);
	local_preempt_restore(enabled);
}

bool need_resched(void)
{
	if (current_thread == &bootstrap)
		return true;
	return scheduler->need_preempt(current_thread);
}

void setup_threading(void)
{
	extern struct scheduler round_robin;
	void setup_round_robin(void);

	setup_round_robin();
	scheduler = &round_robin;

	setup_mm();

	static struct mm mm;

	bootstrap.state = THREAD_ACTIVE;
	bootstrap.mm = &mm;
	mm.pt = pfn2page(load_pml4() >> PAGE_BITS);
	current_thread = &bootstrap;
}
