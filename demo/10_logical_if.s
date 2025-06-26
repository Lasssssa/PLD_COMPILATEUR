	.text
	.globl	_main
_main:
	stp x29, x30, [sp, #-96]!
	mov x29, sp
main_BB_0:
	mov w0, #0
	str w0, [sp, #24]
	ldr w0, [sp, #24]
	str w0, [sp, #16]
	mov w0, #1
	str w0, [sp, #40]
	ldr w0, [sp, #40]
	str w0, [sp, #32]
	ldr w0, [sp, #16]
	str w0, [sp, #56]
	ldr w0, [sp, #32]
	str w0, [sp, #64]
	ldr w0, [sp, #56]
	cmp w0, #0
	cset w0, ne
	ldr w1, [sp, #64]
	cmp w1, #0
	cset w1, ne
	orr w0, w0, w1
	str w0, [sp, #48]
	cmp w0, #0
	b.eq BB_3
	b BB_1
BB_3:
	mov w0, #0
	str w0, [sp, #80]
	ldr w0, [sp, #80]
BB_1:
	mov w0, #1
	str w0, [sp, #72]
	ldr w0, [sp, #72]
	ldp x29, x30, [sp], #96
	ret
