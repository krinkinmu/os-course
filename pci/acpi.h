#ifndef __ACPI_H__
#define __ACPI_H__

#include <stdint.h>
#include <stddef.h>

#define ACPI_RSDP_SIGN_SIZE 8
#define ACPI_OEM_ID_SIZE    6

struct acpi_rsdp {
	char sign[ACPI_RSDP_SIGN_SIZE];
	uint8_t checksum;
	char oem_id[ACPI_OEM_ID_SIZE];
	uint8_t revision;
	uint32_t rsdt_paddr;
	uint32_t size;
	uint64_t xsdt_paddr;
	uint8_t extended_checksum;
	uint8_t reserved[3];
} __attribute__((packed));


#define ACPI_OEM_TABLE_ID_SIZE 8
#define ACPI_TABLE_SIGN_SIZE   4

struct acpi_table_header {
	char sign[ACPI_TABLE_SIGN_SIZE];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem_id[ACPI_OEM_ID_SIZE];
	char oem_table_id[ACPI_OEM_TABLE_ID_SIZE];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t create_revision;
} __attribute__((packed));


struct acpi_rsdt {
	struct acpi_table_header header;
	uint32_t entries[];
} __attribute__((packed));

void setup_acpi(void);
const struct acpi_table_header *acpi_table_find(const char *sign);

#endif /*__ACPI_H__*/
