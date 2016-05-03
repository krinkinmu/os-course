#include "acpi.h"
#include "string.h"
#include "stdio.h"

static const void *acpi_find_rsdp_sign(const void *from, const void *to)
{
	static const char RSDP_SIGN[] = "RSD PTR ";

	const char *l = (const char *)from;
	const char *r = (const char *)to;

	for (const char *ptr = l; ptr < r; ptr += 16) {
		if (!memcmp(RSDP_SIGN, ptr, sizeof(RSDP_SIGN) - 1))
			return ptr;
	}
	return 0;
}

static const void *acpi_find_rsdp_addr(void)
{
	const unsigned long ebda_begin = (unsigned long)
		*((const uint16_t *)(0x40EUL << 4));
	const unsigned long ebda_end = ebda_begin + 0x400;
	const void *addr = acpi_find_rsdp_sign((const void *)ebda_begin,
		(const void *)ebda_end);

	if (addr)
		return addr;

	return acpi_find_rsdp_sign((const void *)0x0E0000UL, 
		(const void *)0x100000UL);
}

static const struct acpi_rsdp *rsdp;
static const struct acpi_rsdt *rsdt;

static int acpi_tables(void)
{
	const uint32_t *begin = rsdt->entries;
	const uint32_t *end = (uint32_t *)((char *)rsdt + rsdt->header.length);

	return end - begin;
}

static const struct acpi_table_header *acpi_get_table(int idx)
{
	if (!rsdt)
		return 0;

	return (struct acpi_table_header *)rsdt->entries[idx];
}

const struct acpi_table_header *acpi_table_find(const char *sign)
{
	const int tables = acpi_tables();

	for (int i = 0; i != tables; ++i) {
		const struct acpi_table_header *hdr = acpi_get_table(i);

		printf("%c%c%c%c\n", hdr->sign[0], hdr->sign[1], hdr->sign[2], hdr->sign[3]);
		if (!memcmp(hdr->sign, sign, ACPI_TABLE_SIGN_SIZE))
			return hdr;
	}

	return 0;
}

void setup_acpi(void)
{
	rsdp = (struct acpi_rsdp *)acpi_find_rsdp_addr();
	if (!rsdp) {
		puts("ACPI not found");
		return;
	}
	rsdt = (struct acpi_rsdt *)rsdp->rsdt_paddr;
}
