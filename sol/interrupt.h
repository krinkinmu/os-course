#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <stdint.h>

struct idt_ptr {
	uint16_t size;
	uint64_t base;
} __attribute__((packed));

struct irqchip {
	void (*map)(unsigned);
	void (*mask)(unsigned);
	void (*unmask)(unsigned);
	void (*eoi)(unsigned);
};

static inline void set_idt(const struct idt_ptr *ptr)
{ __asm__ volatile ("lidt (%0)" : : "a"(ptr)); }

static inline void local_irq_disable(void)
{ __asm__ volatile ("cli" : : : "cc"); }

static inline void local_irq_enable(void)
{ __asm__ volatile ("sti" : : : "cc"); }

static inline void irqchip_map(struct irqchip *chip, unsigned offset)
{ if (chip->map) chip->map(offset); }

static inline void irqchip_mask(struct irqchip *chip, unsigned irq)
{ if (chip->mask) chip->mask(irq); }

static inline void irqchip_unmask(struct irqchip *chip, unsigned irq)
{ if (chip->unmask) chip->unmask(irq); }

static inline void irqchip_eoi(struct irqchip *chip, unsigned irq)
{ if (chip->eoi) chip->eoi(irq); }


typedef void (*irq_t)(int irq);
void register_irq_handler(int irq, irq_t isr);
void unregister_irq_handler(int irq, irq_t isr);

void setup_ints(void);

#endif /*__INTERRUPT_H__*/
