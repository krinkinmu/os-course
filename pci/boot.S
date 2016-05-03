#define MB_HEADER_MAGIC 0x1BADB002

	.section ".header", "ax"
	.align 4
	.long MB_HEADER_MAGIC
	.long 0
	.long -(MB_HEADER_MAGIC + 0)

	.text
	.global entry
entry:
	movl $stack_top, %esp
	cld
	call main
halt:
	hlt
	jmp	halt

	.bss
	.space 4096
stack_top:
