	.text
	.globl	_main
_main:
	stp x29, x30, [sp, #-80]!
	mov x29, sp
main_BB_0:
	mov w0, #2
	str w0, [sp, #24]
	ldr w0, [sp, #24]
	str w0, [sp, #16]
	ldr w0, [sp, #16]
	str w0, [sp, #32]
	mov w0, #1
	str w0, [sp, #40]
	ldr w0, [sp, #32]
	ldr w1, [sp, #40]
	cmp w0, w1
	cset w0, gt
	str w0, [sp, #48]
	cmp w0, #0
	b.eq BB_3
	b BB_1
BB_3:
	mov w0, #20
	str w0, [sp, #64]
	ldr w0, [sp, #64]
BB_1:
	mov w0, #10
	str w0, [sp, #56]
	ldr w0, [sp, #56]
	ldp x29, x30, [sp], #80
	ret
