OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(start32)

PAGE_SIZE = 0x1000;
VIRTUAL_BASE = 0xffffffff80000000;

SECTIONS
{
	. = 1M + SIZEOF_HEADERS;

	text_phys_begin = .;
	text_virt_begin = . + VIRTUAL_BASE;
	.bootstrap : { *(.bootstrap) }

	. += VIRTUAL_BASE;
	.text : AT(ADDR(.bootstrap) + SIZEOF(.bootstrap)) { *(.text) }

	data_phys_begin = . - VIRTUAL_BASE;
	data_virt_begin = .;
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	data_phys_end = . - VIRTUAL_BASE;
	data_virt_end = .;
	. = ALIGN(PAGE_SIZE);

	bss_phys_begin = . - VIRTUAL_BASE;
	bss_virt_begin = .;
	.bss : {
		. += 4 * PAGE_SIZE;
		*(.bss)
	}
	bss_phys_end = . - VIRTUAL_BASE;
	bss_virt_end = .;
}
