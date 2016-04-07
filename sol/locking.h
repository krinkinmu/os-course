#ifndef __LOCKING_H__
#define __LOCKING_H__

#include "threads_defs.h"
#include "stdio.h"
#include "list.h"

struct spinlock { void *padding; };

#define SPINLOCK_INIT(name)	{ 0 }
#define DEFINE_SPINLOCK(name) 	struct spinlock name = SPINLOCK_INIT(name)

static inline void spinlock_init(struct spinlock *lock)
{ lock->padding = 0; }

static inline void spin_lock(struct spinlock *lock)
{
	(void) lock;

	local_preempt_disable();
}

static inline void spin_unlock(struct spinlock *lock)
{
	(void) lock;

	local_preempt_enable();
}

static inline bool spin_lock_irqsave(struct spinlock *lock)
{
	(void) lock;

	return local_preempt_save();
}

static inline void spin_unlock_irqrestore(struct spinlock *lock, bool enabled)
{
	(void) lock;

	local_preempt_restore(enabled);
}


struct wait_queue {
	struct list_head threads;
	struct spinlock lock;
};

#define WAIT_QUEUE_INIT(name) {	\
	LIST_HEAD_INIT((name).threads),	\
	SPINLOCK_INIT((name).lock)	\
}
#define DEFINE_WAIT_QUEUE(name)	\
	struct wait_queue name = WAIT_QUEUE_INIT(name)

struct wait_head {
	struct list_head link;
	struct thread *thread;
};

static inline void wait_queue_init(struct wait_queue *wq)
{
	list_init(&wq->threads);
	spinlock_init(&wq->lock);
}

void wait_queue_notify(struct wait_queue *queue);
void wait_queue_notify_all(struct wait_queue *queue);

#define WAIT_EVENT(wq, cond) \
	do { 								\
		DBG_ASSERT(local_preempt_enabled());			\
		struct wait_queue *__WAIT_EVENT_wq = (wq);		\
		struct wait_head __WAIT_EVENT_wh;			\
									\
		__WAIT_EVENT_wh.thread = current();			\
		spin_lock(&__WAIT_EVENT_wq->lock);			\
									\
		while (!(cond)) {					\
			__WAIT_EVENT_wh.thread->state = THREAD_BLOCKED;	\
			list_add_tail(&__WAIT_EVENT_wh.link,		\
				&__WAIT_EVENT_wq->threads);		\
			spin_unlock(&__WAIT_EVENT_wq->lock);		\
			schedule();					\
			spin_lock(&__WAIT_EVENT_wq->lock);		\
		}							\
									\
		spin_unlock(&__WAIT_EVENT_wq->lock);			\
	} while (0);


#define MUTEX_STATE_UNLOCKED 0
#define MUTEX_STATE_LOCKED   1

struct mutex {
	struct wait_queue wq;
	int state;
};

#define MUTEX_INIT(name) {	\
	WAIT_QUEUE_INIT(name.wq),	\
	MUTEX_STATE_UNLOCKED		\
}
#define DEFINE_MUTEX(name) struct mutex name = MUTEX_INIT(name)

static inline void mutex_init(struct mutex *mutex)
{
	wait_queue_init(&mutex->wq);
	mutex->state = MUTEX_STATE_UNLOCKED;
}

void mutex_lock(struct mutex *mutex);
void mutex_unlock(struct mutex *mutex);


struct condition {
	struct wait_queue wq;
};

#define CONDITION_INIT(name) {	\
	WAIT_QUEUE_INIT((name).wq)	\
}

#define DEFINE_CONDITION(name) struct condition name = CONDITION_INIT(name)

static inline void condition_init(struct condition *condition)
{
	wait_queue_init(&condition->wq);
}

void condition_wait(struct mutex *mutex, struct condition *condition);
void condition_notify(struct condition *condition);
void condition_notify_all(struct condition *condition);

#endif /*__LOCKING_H__*/
