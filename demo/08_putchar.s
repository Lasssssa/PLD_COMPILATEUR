	.text
	.globl	_main
_main:
	stp x29, x30, [sp, #-48]!
	mov x29, sp
main_BB_0:
	mov w0, #90
	str w0, [sp, #24]
	ldr w0, [sp, #24]
	bl _putchar
	str w0, [sp, #16]
	mov w0, #0
	str w0, [sp, #32]
	ldr w0, [sp, #32]
	ldp x29, x30, [sp], #48
	ret
