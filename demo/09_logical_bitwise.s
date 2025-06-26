	.text
	.globl	_main
_main:
	stp x29, x30, [sp, #-112]!
	mov x29, sp
main_BB_0:
	mov w0, #1
	str w0, [sp, #32]
	mov w0, #2
	str w0, [sp, #40]
	ldr w0, [sp, #32]
	ldr w1, [sp, #40]
	orr w0, w0, w1
	str w0, [sp, #24]
	ldr w0, [sp, #24]
	str w0, [sp, #16]
	ldr w0, [sp, #16]
	str w0, [sp, #64]
	mov w0, #3
	str w0, [sp, #72]
	ldr w0, [sp, #64]
	ldr w1, [sp, #72]
	and w0, w0, w1
	str w0, [sp, #56]
	ldr w0, [sp, #56]
	str w0, [sp, #48]
	ldr w0, [sp, #48]
	str w0, [sp, #88]
	mov w0, #1
	str w0, [sp, #96]
	ldr w0, [sp, #88]
	ldr w1, [sp, #96]
	eor w0, w0, w1
	str w0, [sp, #80]
	ldr w0, [sp, #80]
	ldp x29, x30, [sp], #112
	ret
