#ifndef __THREADS_H__
#define __THREADS_H__

#include <stdbool.h>
#include <stddef.h>

#include "locking.h"
#include "kernel.h"
#include "rbtree.h"


enum thread_state {
	THREAD_NONE,
	THREAD_ACTIVE,
	THREAD_BLOCKED,
	THREAD_FINISHED,
	THREAD_DEAD,
	THREAD_REAPED
};

typedef intptr_t pid_t;

struct thread {
	struct rb_node node;
	pid_t pid;
	void *stack_pointer;
	unsigned long long time;
	enum thread_state state;
	struct page *stack;
	struct mm *mm;
	struct spinlock lock;
	int refcount;
};

struct scheduler {
	struct thread *(*alloc)(void);
	void (*free)(struct thread *);
	void (*activate)(struct thread *);
	bool (*need_preempt)(struct thread *);
	struct thread *(*next)(void);
	void (*preempt)(struct thread *);
	void (*place)(struct thread *);
};


static inline pid_t thread_pid(const struct thread *thread)
{ return thread->pid; }

pid_t create_kthread(int (*fptr)(void *), void *arg);

struct thread_regs;

struct thread_regs *thread_regs(struct thread *thread);
struct thread *lookup_thread(pid_t pid);
struct thread *current(void);
void *thread_stack_begin(struct thread *thread);
void *thread_stack_end(struct thread *thread);
void put_thread(struct thread *thread);
void get_thread(struct thread *thread);
void schedule(void);
bool need_resched(void);


static inline pid_t getpid(void)
{ return thread_pid(current()); }

void activate_thread(struct thread *thread);
int wait(pid_t pid);
void exit(void);


void idle(void);
void setup_threading(void);

#endif /*__THREADS_H__*/
