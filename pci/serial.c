#include "console.h"
#include "serial.h"
#include "ioport.h"

#define SERIAL_PORT_IO_BASE 0x3f8
#define REG_DATA            (SERIAL_PORT_IO_BASE)
#define REG_DLL             (SERIAL_PORT_IO_BASE)
#define REG_IER             (SERIAL_PORT_IO_BASE + 1)
#define REG_DLH             (SERIAL_PORT_IO_BASE + 1)
#define REG_FCR             (SERIAL_PORT_IO_BASE + 2)
#define REG_LCR             (SERIAL_PORT_IO_BASE + 3)
#define REG_LSR             (SERIAL_PORT_IO_BASE + 5)

#define FCR_EFIFO           BIT_CONST(0)
#define FCR_14BYTES         (BIT_CONST(6) | BIT_CONST(7))
#define LCR_8BIT            (BIT_CONST(0) | BIT_CONST(1))
#define LCR_DLAB            BIT_CONST(7)
#define LSR_TX_READY        BIT_CONST(5)

static void serial_putchar(int c)
{
	while (!(in8(REG_LSR) & LSR_TX_READY));
	out8(REG_DATA, c);
}

static void serial_write(const char *buf, unsigned long size)
{
	for (unsigned long i = 0; i != size; ++i)
		serial_putchar(buf[i]);
}

void setup_serial(void)
{
	out8(REG_IER, 0);
	out8(REG_LCR, LCR_DLAB);
	out8(REG_DLL, 0x0C);
	out8(REG_DLH, 0x00);
	out8(REG_LCR, LCR_8BIT);

	static struct console serial_console = {
		.write = &serial_write
	};

	register_console(&serial_console);
}
