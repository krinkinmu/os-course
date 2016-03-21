#include "multiboot.h"
#include "string.h"
#include "stdio.h"
#include "misc.h"

#include <stdint.h>


#define MMAP_MAX_COUNT  20
#define MAX_CMDLINE_LEN 256

const char *cmdline;
struct mmap_entry memory_map[MMAP_MAX_COUNT];
int memory_map_size;

unsigned long initrd_begin;
unsigned long initrd_end;

struct mboot_mmap_entry {
	uint32_t size;
	uint64_t addr;
	uint64_t length;
	uint32_t type;
} __attribute__((packed));

static void setup_memory_map(struct multiboot_info *info)
{
	DBG_ASSERT(info->flags & (1ul << 6));

	const char *begin = (const char *)((uintptr_t)info->mmap_addr);
	const char *end = begin + info->mmap_length;

	while (begin < end) {
		const struct mboot_mmap_entry *entry =
					(const struct mboot_mmap_entry *)begin;

		memory_map[memory_map_size].addr = entry->addr;
		memory_map[memory_map_size].length = entry->length;
		memory_map[memory_map_size].type = entry->type;

		begin += entry->size + sizeof(entry->size);
		++memory_map_size;
	}
}

static void setup_cmdline(const struct multiboot_info *info)
{
	static char cmdline_buf[MAX_CMDLINE_LEN];

	if (info->flags & (1ul << 2)) {
		strcpy(cmdline_buf, (const char *)((uintptr_t)info->cmdline));
		cmdline = cmdline_buf;
	}
}

void setup_misc(void)
{
	extern const uint32_t mboot_info;

	struct multiboot_info *pmboot_info = (void *)((uintptr_t)mboot_info);

	setup_memory_map(pmboot_info);
	setup_cmdline(pmboot_info);
}
