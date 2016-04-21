#ifndef __MISC_H__
#define __MISC_H__

struct mmap_entry {
	unsigned long long addr;
	unsigned long long length;
	unsigned long type;
};

static const unsigned long MMAP_AVAILABLE = 1;

extern const char *cmdline;
extern struct mmap_entry memory_map[];
extern int memory_map_size;

extern unsigned long initrd_begin;
extern unsigned long initrd_end;

void setup_misc(void);

#endif /*__MISC_H__*/
