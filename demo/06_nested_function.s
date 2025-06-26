	.text
	.globl	double_val
double_val:
	stp x29, x30, [sp, #-48]!
	mov x29, sp
double_val_BB_0:
	str w0, [sp, #16]
	ldr w0, [sp, #16]
	str w0, [sp, #32]
	mov w0, #2
	str w0, [sp, #40]
	ldr w0, [sp, #32]
	ldr w1, [sp, #40]
	mul w0, w0, w1
	str w0, [sp, #24]
	ldr w0, [sp, #24]
	ldp x29, x30, [sp], #48
	ret
	.globl	_main
_main:
	stp x29, x30, [sp, #-32]!
	mov x29, sp
main_BB_2:
	mov w0, #21
	str w0, [sp, #24]
	ldr w0, [sp, #24]
	bl double_val
	str w0, [sp, #16]
	ldr w0, [sp, #16]
	ldp x29, x30, [sp], #32
	ret
