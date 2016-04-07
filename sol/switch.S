	.code64
	.text
	.global switch_threads
switch_threads:
	pushq %rbp
	pushq %rbx
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15

	movq %rsp, (%rdi)
	movq %rsi, %rsp

	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbx
	popq %rbp

	ret
