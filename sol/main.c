#include "interrupt.h"
#include "serial.h"
#include "stdio.h"
#include "time.h"

void main(void)
{
	setup_serial();
	setup_ints();
	setup_time();

	local_irq_enable();

	while (1);
}
