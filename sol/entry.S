	.text
	.code64

#define SAVE_VOLATILE \
	movq %r15, 0(%rsp);	\
	movq %r14, 8(%rsp);	\
	movq %r13, 16(%rsp);	\
	movq %r12, 24(%rsp);	\
	movq %rbp, 32(%rsp);	\
	movq %rbx, 40(%rsp);

#define SAVE_NONVOLATILE \
	movq %r11, 48(%rsp);	\
	movq %r10, 56(%rsp);	\
	movq %r9, 64(%rsp);	\
	movq %r8, 72(%rsp);	\
	movq %rax, 80(%rsp);	\
	movq %rcx, 88(%rsp);	\
	movq %rdx, 96(%rsp);	\
	movq %rsi, 104(%rsp);	\
	movq %rdi, 112(%rsp);	

#define RESTORE_VOLATILE \
	movq 0(%rsp), %r15;	\
	movq 8(%rsp), %r14;	\
	movq 16(%rsp), %r13;	\
	movq 24(%rsp), %r12;	\
	movq 32(%rsp), %rbp;	\
	movq 40(%rsp), %rbx;

#define RESTORE_NONVOLATILE \
	movq 48(%rsp), %r11;	\
	movq 56(%rsp), %r10;	\
	movq 64(%rsp), %r9;	\
	movq 72(%rsp), %r8;	\
	movq 80(%rsp), %rax;	\
	movq 88(%rsp), %rcx;	\
	movq 96(%rsp), %rdx;	\
	movq 104(%rsp), %rsi;	\
	movq 112(%rsp), %rdi;	

	.extern isr_common_handler
common_handler:
	subq $120, %rsp
	SAVE_VOLATILE
	SAVE_NONVOLATILE

	cld
	movq %rsp, %rdi
	call isr_common_handler

	RESTORE_VOLATILE
	RESTORE_NONVOLATILE
	addq $136, %rsp
	iretq

#define ENTRY_NAME(nr) entry ## nr

#define ISR(nr) \
	.align 16;	\
ENTRY_NAME(nr):		\
	pushq $0;	\
	pushq $nr;	\
	jmp common_handler

#define EXC(nr) \
	.align 16;	\
ENTRY_NAME(nr):		\
	pushq $nr;	\
	jmp common_handler

/* isr and exception entries */
	ISR(0)
	ISR(1)
	ISR(2)
	ISR(3)
	ISR(4)
	ISR(5)
	ISR(6)
	ISR(7)
	EXC(8)
	ISR(9)
	EXC(10)
	EXC(11)
	EXC(12)
	EXC(13)
	EXC(14)
	ISR(15)
	ISR(16)
	EXC(17)
	ISR(18)
	ISR(19)
	ISR(20)
	ISR(21)
	ISR(22)
	ISR(23)
	ISR(24)
	ISR(25)
	ISR(26)
	ISR(27)
	ISR(28)
	ISR(29)
	ISR(30)
	ISR(31)
	ISR(32)
	ISR(33)
	ISR(34)
	ISR(35)
	ISR(36)
	ISR(37)
	ISR(38)
	ISR(39)
	ISR(40)
	ISR(41)
	ISR(42)
	ISR(43)
	ISR(44)
	ISR(45)
	ISR(46)
	ISR(47)

	.align 16
	.global isr_entry
isr_entry:
	.quad ENTRY_NAME(0)
	.quad ENTRY_NAME(1)
	.quad ENTRY_NAME(2)
	.quad ENTRY_NAME(3)
	.quad ENTRY_NAME(4)
	.quad ENTRY_NAME(5)
	.quad ENTRY_NAME(6)
	.quad ENTRY_NAME(7)
	.quad ENTRY_NAME(8)
	.quad ENTRY_NAME(9)
	.quad ENTRY_NAME(10)
	.quad ENTRY_NAME(11)
	.quad ENTRY_NAME(12)
	.quad ENTRY_NAME(13)
	.quad ENTRY_NAME(14)
	.quad ENTRY_NAME(15)
	.quad ENTRY_NAME(16)
	.quad ENTRY_NAME(17)
	.quad ENTRY_NAME(18)
	.quad ENTRY_NAME(19)
	.quad ENTRY_NAME(20)
	.quad ENTRY_NAME(21)
	.quad ENTRY_NAME(22)
	.quad ENTRY_NAME(23)
	.quad ENTRY_NAME(24)
	.quad ENTRY_NAME(25)
	.quad ENTRY_NAME(26)
	.quad ENTRY_NAME(27)
	.quad ENTRY_NAME(28)
	.quad ENTRY_NAME(29)
	.quad ENTRY_NAME(30)
	.quad ENTRY_NAME(31)
	.quad ENTRY_NAME(32)
	.quad ENTRY_NAME(33)
	.quad ENTRY_NAME(34)
	.quad ENTRY_NAME(35)
	.quad ENTRY_NAME(36)
	.quad ENTRY_NAME(37)
	.quad ENTRY_NAME(38)
	.quad ENTRY_NAME(39)
	.quad ENTRY_NAME(40)
	.quad ENTRY_NAME(41)
	.quad ENTRY_NAME(42)
	.quad ENTRY_NAME(43)
	.quad ENTRY_NAME(44)
	.quad ENTRY_NAME(45)
	.quad ENTRY_NAME(46)
	.quad ENTRY_NAME(47)

