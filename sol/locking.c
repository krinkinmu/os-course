#include "locking.h"
#include "threads.h"


static void __wait_queue_notify(struct wait_queue *queue)
{
	if (!list_empty(&queue->threads)) {
		struct list_head *ptr = list_first(&queue->threads);
		struct wait_head *wh = LIST_ENTRY(ptr, struct wait_head, link);

		list_del(ptr);
		activate_thread(wh->thread);
	}
}

void wait_queue_notify(struct wait_queue *queue)
{
	const unsigned long flags = spin_lock_irqsave(&queue->lock);

	__wait_queue_notify(queue);
	spin_unlock_irqrestore(&queue->lock, flags);
}

void wait_queue_notify_all(struct wait_queue *queue)
{
	LIST_HEAD(threads);
	const unsigned long flags = spin_lock_irqsave(&queue->lock);

	list_splice(&queue->threads, &threads);
	spin_unlock_irqrestore(&queue->lock, flags);

	struct list_head *head = &threads;
	struct list_head *ptr = head->next;

	for (; ptr != head; ptr = ptr->next) {
		struct wait_head *wh = LIST_ENTRY(ptr, struct wait_head, link);

		activate_thread(wh->thread);
	}
}

static bool __mutex_try_lock(struct mutex *mutex)
{
	if (mutex->state == MUTEX_STATE_UNLOCKED) {
		mutex->state = MUTEX_STATE_LOCKED;
		return true;
	}
	return false;
}

void mutex_lock(struct mutex *mutex)
{
	WAIT_EVENT(&mutex->wq, __mutex_try_lock(mutex));
}

void mutex_unlock(struct mutex *mutex)
{
	const bool enabled = spin_lock_irqsave(&mutex->wq.lock);

	mutex->state = MUTEX_STATE_UNLOCKED;
	__wait_queue_notify(&mutex->wq);
	spin_unlock_irqrestore(&mutex->wq.lock, enabled);
}

void condition_wait(struct mutex *mutex, struct condition *condition)
{
	struct thread *self = current();
	struct wait_head wh;

	wh.thread = self;

	spin_lock(&condition->wq.lock);
	self->state = THREAD_BLOCKED;
	list_add_tail(&wh.link, &condition->wq.threads);
	mutex_unlock(mutex);
	spin_unlock(&condition->wq.lock);

	schedule();

	mutex_lock(mutex);
}

void condition_notify(struct condition *condition)
{
	wait_queue_notify(&condition->wq);
}

void condition_notify_all(struct condition *condition)
{
	wait_queue_notify_all(&condition->wq);
}
