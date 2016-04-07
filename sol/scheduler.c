#include "kmem_cache.h"
#include "threads.h"
#include "stdio.h"
#include "time.h"
#include "list.h"

#define RR_SCHED_SLICE 20
#define RR_MS          1000

struct rr_thread {
	struct thread thread;
	struct list_head link;
};


static struct rr_thread *RR_THREAD(struct thread *thread)
{ return (struct rr_thread *)thread; }

static struct thread *THREAD(struct rr_thread *thread)
{ return &thread->thread; }


static struct kmem_cache *rr_thread_cache;
static LIST_HEAD(rr_active_list);


static struct thread *rr_alloc_thread(void)
{
	struct rr_thread *thread = kmem_cache_alloc(rr_thread_cache);

	if (!thread)
		return 0;

	list_init(&thread->link);

	return THREAD(thread);
}

static void rr_free_thread(struct thread *thread)
{
	struct rr_thread *rr_thread = RR_THREAD(thread);

	kmem_cache_free(rr_thread_cache, rr_thread);
}

static bool rr_need_preempt(struct thread *thread)
{
	return (jiffies - thread->time) * RR_MS > RR_SCHED_SLICE * HZ;
}

static struct thread *rr_next_thread(void)
{
	DBG_ASSERT(local_preempt_disabled());

	if (list_empty(&rr_active_list))
		return 0;

	struct list_head *first = list_first(&rr_active_list);
	struct rr_thread *thread = LIST_ENTRY(first, struct rr_thread, link);

	list_del(&thread->link);

	return THREAD(thread);
}

static void rr_activate_thread(struct thread *thread)
{
	DBG_ASSERT(local_preempt_disabled());
	DBG_ASSERT(thread->state == THREAD_ACTIVE);

	list_add_tail(&RR_THREAD(thread)->link, &rr_active_list);
}

static void rr_preempt_thread(struct thread *thread)
{
	DBG_ASSERT(local_preempt_disabled());

	if (thread->state == THREAD_ACTIVE)
		list_add_tail(&RR_THREAD(thread)->link, &rr_active_list);
}


struct scheduler round_robin = {
	.alloc = rr_alloc_thread,
	.free = rr_free_thread,
	.activate = rr_activate_thread,
	.need_preempt = rr_need_preempt,
	.next = rr_next_thread,
	.preempt = rr_preempt_thread
};

void setup_round_robin(void)
{
	DBG_ASSERT((rr_thread_cache = KMEM_CACHE(struct rr_thread)) != 0);
}
