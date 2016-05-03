#include "pci.h"
#include "stdio.h"
#include "kernel.h"
#include "ioport.h"

#define CONFIG_ADDRESS 0xcf8
#define CONFIG_DATA    0xcfc

static unsigned long pci_config_read(int bus, int device, int func, int offs)
{
	const uint32_t address = ((uint32_t)(bus & 0xffu) << 16) |
				((uint32_t)(device & 0x1fu) << 11) |
				((uint32_t)(func & 0x07u) << 8) |
				((uint32_t)(offs & 0xfcu)) |
				(uint32_t)BIT_CONST(31);

	out32(CONFIG_ADDRESS, address);
	return in32(CONFIG_DATA);
}

static long pci_vendor_id(int bus, int device, int func)
{
	const unsigned long ven_dev = pci_config_read(bus, device, func, 0);

	if ((ven_dev & 0xffffu) == 0xffffu)
		return -1;

	return ven_dev & 0xfffflu;
}

static long pci_device_id(int bus, int device, int func)
{
	const unsigned long ven_dev = pci_config_read(bus, device, func, 0);

	if ((ven_dev & 0xffffu) == 0xffffu)
		return -1;

	return (ven_dev >> 16) & 0xfffflu;
}

static void pci_scan(void)
{
	for (int bus = 0; bus != 256; ++bus) {
		for (int slot = 0; slot != 32; ++slot) {
			const long vendor = pci_vendor_id(bus, slot, 0);
			const long device = pci_device_id(bus, slot, 0);

			if (vendor < 0)
				continue;

			printf("Device [%d:%d] Vendor ID: %#lx, Device ID: %#lx\n",
				bus, slot, vendor, device);
		}
	}
}

void setup_pci(void)
{
	pci_scan();
}
