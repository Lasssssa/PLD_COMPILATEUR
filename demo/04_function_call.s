	.text
	.globl	f
f:
	stp x29, x30, [sp, #-64]!
	mov x29, sp
f_BB_0:
	str w0, [sp, #16]
	str w1, [sp, #24]
	ldr w0, [sp, #16]
	str w0, [sp, #40]
	ldr w0, [sp, #24]
	str w0, [sp, #48]
	ldr w0, [sp, #40]
	ldr w1, [sp, #48]
	add w0, w0, w1
	str w0, [sp, #32]
	ldr w0, [sp, #32]
	ldp x29, x30, [sp], #64
	ret
	.globl	_main
_main:
	stp x29, x30, [sp, #-48]!
	mov x29, sp
main_BB_2:
	mov w0, #3
	str w0, [sp, #24]
	mov w0, #4
	str w0, [sp, #32]
	ldr w0, [sp, #24]
	ldr w1, [sp, #32]
	bl f
	str w0, [sp, #16]
	ldr w0, [sp, #16]
	ldp x29, x30, [sp], #48
	ret
