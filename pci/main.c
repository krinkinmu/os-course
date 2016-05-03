#include "serial.h"
#include "stdio.h"
#include "acpi.h"
#include "pci.h"

void main(void)
{
	setup_serial();
	setup_acpi();
	setup_pci();
}
