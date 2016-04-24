#ifndef __ELF_H__
#define __ELF_H__

#include <stdint.h>

#define ELF_NIDENT      16
#define ELF_CLASS       4
#define ELF_DATA        5
#define ELF_CLASS64     2
#define ELF_DATA2LSB    1
#define ELF_EXEC        2

struct elf_hdr {
	uint8_t e_ident[ELF_NIDENT];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
} __attribute__((packed));


#define PT_NULL 0
#define PT_LOAD 1
#define PF_X    0x1
#define PF_W    0x2
#define PF_R    0x4

struct elf_phdr {
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_paddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
} __attribute__((packed));

#endif /*__ELF_H__*/
