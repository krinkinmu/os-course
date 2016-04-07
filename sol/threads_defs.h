#ifndef __THREADS_DEFS_H__
#define __THREADS_DEFS_H__

#include <stdbool.h>

#include "interrupt.h"

#define barrier() __asm__ volatile ("" : : : "memory")

static inline void local_preempt_disable(void)
{
	local_irq_disable();
	barrier();
}

static inline void local_preempt_enable(void)
{
	barrier();
	local_irq_enable();
}

static inline bool local_preempt_enabled(void)
{ return local_irq_enabled(); }

static inline bool local_preempt_disabled(void)
{ return local_irq_disabled(); }

static inline bool local_preempt_save(void)
{
	const bool enabled = local_preempt_enabled();

	local_preempt_disable();
	return enabled;
}

static inline void local_preempt_restore(bool enabled)
{
	if (enabled)
		local_preempt_enable();
}

#endif /*__THREADS_DEFS_H__*/
