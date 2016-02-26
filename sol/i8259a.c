#include "interrupt.h"
#include "ioport.h"

#define PIC_MASTER_CMD_PORT  0x20
#define PIC_MASTER_DATA_PORT 0x21
#define PIC_SLAVE_CMD_PORT   0xA0
#define PIC_SLAVE_DATA_PORT  0xA1

#define PIC_SLAVE_IRQ        0x02
#define PIC_SLAVE_IRQ_BIT    (1 << PIC_SLAVE_IRQ)
#define PIC_IRQS_PER_CHIP    8
#define PIC_EOI_CMD          0x60
#define PIC_ISR              0x0B

#define PIC_ICW1_EXPECT_ICW4 (1 << 0)
#define PIC_ICW1_INIT        (1 << 4)
#define PIC_ICW4_8086        (1 << 0)

static unsigned pic_irq_mask = 0xFFFFu;

static void pic_remap(unsigned offset)
{
	out8(PIC_MASTER_CMD_PORT, PIC_ICW1_EXPECT_ICW4 | PIC_ICW1_INIT);
	out8(PIC_SLAVE_CMD_PORT, PIC_ICW1_EXPECT_ICW4 | PIC_ICW1_INIT);
	out8(PIC_MASTER_DATA_PORT, offset);
	out8(PIC_SLAVE_DATA_PORT, offset + PIC_IRQS_PER_CHIP);
	out8(PIC_MASTER_DATA_PORT, PIC_SLAVE_IRQ_BIT);
	out8(PIC_SLAVE_DATA_PORT, PIC_SLAVE_IRQ);
	out8(PIC_MASTER_DATA_PORT, PIC_ICW4_8086);
	out8(PIC_SLAVE_DATA_PORT, PIC_ICW4_8086);

	out8(PIC_MASTER_DATA_PORT, pic_irq_mask & 0xFFU);
	out8(PIC_SLAVE_DATA_PORT, (pic_irq_mask >> 8) & 0xFFU);
}

static void pic_mask(unsigned irq)
{
	pic_irq_mask |= 1u << irq;
	if (irq < 8)
		out8(PIC_MASTER_DATA_PORT, pic_irq_mask & 0xFFU);
	else
		out8(PIC_SLAVE_DATA_PORT, (pic_irq_mask >> 8) & 0xFFU);
}

static void pic_unmask(unsigned irq)
{
	pic_irq_mask &= ~(1u << irq);
	if (irq < 8)
		out8(PIC_MASTER_DATA_PORT, pic_irq_mask & 0xFFU);
	else
		out8(PIC_SLAVE_DATA_PORT, (pic_irq_mask >> 8) & 0xFFU);
}

static void pic_eoi(unsigned irq)
{
	if (irq >= 8) {
		out8(PIC_SLAVE_CMD_PORT, PIC_EOI_CMD + (irq & 7));
		out8(PIC_MASTER_CMD_PORT, PIC_EOI_CMD + PIC_SLAVE_IRQ);
	} else {
		out8(PIC_MASTER_CMD_PORT, PIC_EOI_CMD + irq);
	}
}

struct irqchip i8259a = {
	.map = &pic_remap,
	.mask = &pic_mask,
	.unmask = &pic_unmask,
	.eoi = &pic_eoi
};
