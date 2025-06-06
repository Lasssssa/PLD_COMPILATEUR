	.section	.text
	.globl	main
	.type	main, @function
main:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
BB_0:
	movl	$10, %eax
	movl	%eax, 0(%rbp)
	movl	0(%rbp), %eax
	movl	%eax, 0(%rbp)
	movl	0(%rbp), %eax
	movl	%eax, -8(%rbp)
	movl	-8(%rbp), %eax
	movl	%eax, 0(%rbp)
	movl	0(%rbp), %eax
	ret
	.size	main, .-main
	.section	.note.GNU-stack,"",@progbits
