	.text
	.globl	_main
_main:
	stp x29, x30, [sp, #-176]!
	mov x29, sp
main_BB_0:
	mov w0, #2
	str w0, [sp, #24]
	ldr w0, [sp, #24]
	str w0, [sp, #16]
	mov w0, #3
	str w0, [sp, #40]
	ldr w0, [sp, #40]
	str w0, [sp, #32]
	ldr w0, [sp, #16]
	str w0, [sp, #56]
	mov w0, #1
	str w0, [sp, #64]
	ldr w0, [sp, #56]
	ldr w1, [sp, #64]
	cmp w0, w1
	cset w0, gt
	str w0, [sp, #72]
	ldr w0, [sp, #32]
	str w0, [sp, #80]
	mov w0, #5
	str w0, [sp, #88]
	ldr w0, [sp, #80]
	ldr w1, [sp, #88]
	cmp w0, w1
	cset w0, lt
	str w0, [sp, #96]
	ldr w0, [sp, #72]
	cmp w0, #0
	cset w0, ne
	ldr w1, [sp, #96]
	cmp w1, #0
	cset w1, ne
	and w0, w0, w1
	str w0, [sp, #48]
	cmp w0, #0
	b.eq BB_3
	b BB_1
BB_3:
	mov w0, #0
	str w0, [sp, #160]
	ldr w0, [sp, #160]
BB_1:
	ldr w0, [sp, #16]
	str w0, [sp, #112]
	ldr w0, [sp, #32]
	str w0, [sp, #120]
	ldr w0, [sp, #112]
	ldr w1, [sp, #120]
	add w0, w0, w1
	str w0, [sp, #104]
	mov w0, #5
	str w0, [sp, #128]
	ldr w0, [sp, #104]
	ldr w1, [sp, #128]
	cmp w0, w1
	cset w0, eq
	str w0, [sp, #136]
	cmp w0, #0
	b.eq BB_6
	b BB_4
BB_6:
	mov w0, #1
	str w0, [sp, #152]
	ldr w0, [sp, #152]
BB_4:
	mov w0, #42
	str w0, [sp, #144]
	ldr w0, [sp, #144]
	ldp x29, x30, [sp], #176
	ret
