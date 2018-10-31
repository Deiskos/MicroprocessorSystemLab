	.syntax unified
	.cpu cortex-m4
	.thumb
.data
	result: .byte 0
.text
	.global main
	.equ X, 0x55AA
	.equ Y, 0xAA55
hamm:
	// the code is C++ code from Wikipedia translated to ARM
	//r4 = val, r5 = val-1, r6 = dist
	eor R4, R1, R2
	mov R6, 0
	LOOP:
		cmp R4, 0
		beq EXIT
		add r6, r6, 1
		sub r5, r4, 1
		and r4, r5
	b LOOP
	EXIT:
	mov R0, R6
	bx lr

main:
	mov R1, #X
	mov R2, #Y
	ldr R3, =result
	bl hamm
	str r0, [r3]
	nop // for debug purposes
L: b L
