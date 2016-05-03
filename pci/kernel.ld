OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(entry)

SECTIONS
{
	. = 1M;
	.header    : { *(.header) }

	.text   : { *(.text)   *(.text.*) }
	.rodata : { *(.rodata) *(.rodata.*) }
	.data   : { *(.data)   *(.data.*) }
	.bss    : { *(.bss) }

	.debug_aranges 0 : { *(.debug_aranges) }
	.debug_info    0 : { *(.debug_info) }
	.debug_abbrev  0 : { *(.debug_abbrev) }
	.debug_line    0 : { *(.debug_line) }
        .debug_str     0 : { *(.debug_str) }

	/DISCARD/ : { *(.eh_frame .note.* .comment) }
}
