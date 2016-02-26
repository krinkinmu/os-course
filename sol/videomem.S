#define COLS 80
#define ROWS 25
#define ADDR 0xb8000
#define ATTR 7

	.section .bootstrap, "ax"
	.code32
	.global videomem_puts, videomem_putchar, videomem_clrscr

videomem_putchar_at:
	movl 8(%esp), %eax     // store row number in eax
	imul $COLS, %eax       // multiply by number of columns in row
	addl 12(%esp), %eax    // add column number
	shll $1, %eax          // multiply by 2 (2 bytes per one character)
	addl $ADDR, %eax       // add video memory offset
	movl 4(%esp), %edx     // store char to edx
	orl $(ATTR << 8), %edx // save attrs
	movw %dx, (%eax)       // write into video memory
	ret

videomem_empty_line:
	pushl %ebx
	movl 8(%esp), %eax
	subl $12, %esp
	movl $(' ' | (ATTR << 8)), (%esp)
	movl %eax, 4(%esp)
	xorl %ebx, %ebx
1:
	cmpl $COLS, %ebx
	je 1f
	movl %ebx, 8(%esp)
	call videomem_putchar_at
	inc %ebx
	jmp 1b
1:
	addl $12, %esp
	popl %ebx
	ret

videomem_row:
	.long 0x00
videomem_col:
	.long 0x00

videomem_putchar:
	movl 4(%esp), %ecx

	cmpl $'\n', %ecx
	je newline
	cmpl $'\r', %ecx
	je carrige

	movl videomem_col, %eax
	movl videomem_row, %edx

	pushl %eax
	pushl %edx
	pushl %ecx

	call videomem_putchar_at

	popl %ecx
	popl %edx
	popl %eax

	inc %eax
	cmpl $COLS, %eax
	je newline
	movl %eax, videomem_col
	ret

newline:
	xorl %edx, %edx
	movl videomem_row, %eax
	inc %eax
	movl $ROWS, %ecx
	div %ecx
	pushl %edx
	call videomem_empty_line
	popl %edx
	movl %edx, videomem_row
carrige:
	movl $0x0, videomem_col
	ret

videomem_puts:
	movl 4(%esp), %eax
	subl $8, %esp
	movl %esi, 4(%esp)
	movl %eax, %esi
1:
	xorl %eax, %eax
	lodsb
	test %al, %al
	je 1f
	movl %eax, (%esp)
	call videomem_putchar
	jmp 1b
1:
	movl 4(%esp), %esi
	addl $8, %esp
	ret

videomem_clrscr:
	pushl %edi
	movl $ADDR, %edi
	movl $((' ' | (ATTR << 8)) | ((' ' | (ATTR << 8)) << 16)), %eax
	movl $(COLS * ROWS / 2), %ecx
	rep stosl
	popl %edi

	movw $0x3cc, %dx
	inb %dx, %al
	xorb $1, %al
	movw $0x3c2, %dx
	out %al, %dx

	movw $0x3b4, %dx
	movb $0x0a, %al
	outb %al, %dx
	movw $0x3b5, %dx
	movb $(1 << 5), %al
	outb %al, %dx

	movw $0x3b4, %dx
	movb $0x0b, %al
	outb %al, %dx
	movw $0x3b5, %dx
	movb $0, %al
	outb %al, %dx
	ret
