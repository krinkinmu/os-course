#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <stdbool.h>

#define RFLAGS_IF (1ul << 9)

typedef void (*irq_t)(int irq);

inline static void local_irq_disable(void)
{ __asm__ volatile ("cli" : : : "cc"); }

inline static void local_irq_enable(void)
{ __asm__ volatile ("sti" : : : "cc"); }

inline static unsigned long local_save_flags(void)
{
	unsigned long flags;

	__asm__ ("pushfq ; popq %0" : "=rm"(flags) : : "memory");
	return flags;
}

inline static void local_restore_flags(unsigned long flags)
{ __asm__ ("pushq %0 ; popfq" : : "g"(flags) : "memory"); }

inline static bool local_irq_enabled(void)
{ return (local_save_flags() & RFLAGS_IF) != 0; }

inline static bool local_irq_disabled(void)
{ return !local_irq_enabled(); }

inline static unsigned long local_irqsave(void)
{
	const unsigned long flags = local_save_flags();

	local_irq_disable();
	return flags;
}

inline static void local_irqrestore(unsigned long flags)
{
	if (flags & RFLAGS_IF)
		local_irq_enable();
}

void register_irq_handler(int irq, irq_t isr);
void unregister_irq_handler(int irq, irq_t isr);
void setup_ints(void);

#endif /*__INTERRUPT_H__*/
