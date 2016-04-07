#ifndef __IRQCHIP_H__
#define __IRQCHIP_H__

struct irqchip {
	void (*map)(unsigned);
	void (*mask)(unsigned);
	void (*unmask)(unsigned);
	void (*eoi)(unsigned);
};

extern const struct irqchip i8259a;

static inline void irqchip_map(const struct irqchip *chip, unsigned offset)
{ if (chip->map) chip->map(offset); }

static inline void irqchip_mask(const struct irqchip *chip, unsigned irq)
{ if (chip->mask) chip->mask(irq); }

static inline void irqchip_unmask(const struct irqchip *chip, unsigned irq)
{ if (chip->unmask) chip->unmask(irq); }

static inline void irqchip_eoi(const struct irqchip *chip, unsigned irq)
{ if (chip->eoi) chip->eoi(irq); }

#endif /* __IRQCHIP_H__ */
