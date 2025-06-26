	.text
	.globl	_main
_main:
	stp x29, x30, [sp, #-32]!
	mov x29, sp
main_BB_0:
	mov w0, #42
	str w0, [sp, #16]
	ldr w0, [sp, #16]
	ldp x29, x30, [sp], #32
	ret
