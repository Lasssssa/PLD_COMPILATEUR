	.text
	.globl	_main
_main:
	stp x29, x30, [sp, #-48]!
	mov x29, sp
main_BB_2:
	mov w0, #1
	str w0, [sp, #24]
	mov w0, #2
	str w0, [sp, #32]
	mov w0, #3
	str w0, [sp, #40]
	ldr w0, [sp, #24]
	ldr w1, [sp, #32]
	ldr w2, [sp, #40]
	bl sum
	str w0, [sp, #16]
	ldr w0, [sp, #16]
	ldp x29, x30, [sp], #48
	ret
	.globl	sum
sum:
	stp x29, x30, [sp, #-80]!
	mov x29, sp
sum_BB_0:
	str w0, [sp, #16]
	str w1, [sp, #24]
	str w2, [sp, #32]
	ldr w0, [sp, #16]
	str w0, [sp, #56]
	ldr w0, [sp, #24]
	str w0, [sp, #64]
	ldr w0, [sp, #56]
	ldr w1, [sp, #64]
	add w0, w0, w1
	str w0, [sp, #48]
	ldr w0, [sp, #32]
	str w0, [sp, #72]
	ldr w0, [sp, #48]
	ldr w1, [sp, #72]
	add w0, w0, w1
	str w0, [sp, #40]
	ldr w0, [sp, #40]
	ldp x29, x30, [sp], #80
	ret
