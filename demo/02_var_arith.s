	.text
	.globl	_main
_main:
	stp x29, x30, [sp, #-96]!
	mov x29, sp
main_BB_0:
	mov w0, #5
	str w0, [sp, #24]
	ldr w0, [sp, #24]
	str w0, [sp, #16]
	ldr w0, [sp, #16]
	str w0, [sp, #48]
	mov w0, #3
	str w0, [sp, #56]
	ldr w0, [sp, #48]
	ldr w1, [sp, #56]
	add w0, w0, w1
	str w0, [sp, #40]
	ldr w0, [sp, #40]
	str w0, [sp, #32]
	ldr w0, [sp, #32]
	str w0, [sp, #72]
	mov w0, #2
	str w0, [sp, #80]
	ldr w0, [sp, #72]
	ldr w1, [sp, #80]
	mul w0, w0, w1
	str w0, [sp, #64]
	ldr w0, [sp, #64]
	ldp x29, x30, [sp], #96
	ret
