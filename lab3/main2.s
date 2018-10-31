	.syntax unified
	.cpu cortex-m4
	.thumb

.data
	result: .word 0
	max_size: .word 0
	sp_start: .word 0

.text
	.global main
	m: .word 0x5E
	n: .word 0x60

	.align 4

GCD:
	stmfd sp!, {r1-r2, lr}



	cmp r1, r2
	bne GCD_NOT_EQUAL
		mov r3, r1
		b GCD_EXIT
	GCD_NOT_EQUAL:

	cmp r1, 0
	bne GCD_R1_NOT_EQUAL_ZERO
		mov r3, r2
		b GCD_EXIT
	GCD_R1_NOT_EQUAL_ZERO:

	cmp r2, 0
	bne GCD_R2_NOT_EQUAL_ZERO
		mov r3, r1
		b GCD_EXIT
	GCD_R2_NOT_EQUAL_ZERO:

	tst r1, 1
	bne R1_ODD
		// r1 is even
		tst r2, 1
		beq R2_EVEN
			// r2 is odd
			asr r1, 1 // m >> 1
			bl GCD
			b GCD_EXIT
		R2_EVEN:
			asr r1, 1
			asr r2, 1
			bl GCD
			lsl r3, 1
			b GCD_EXIT
	R1_ODD:

	tst r2, 1
	bne R2_ODD
		// r2 is even
		asr r2, 1
		bl GCD
		b GCD_EXIT
	R2_ODD:

	cmp r1, r2
	ble R1_LESS_OR_EQ_R2
		// r1 > r2
		sub r1, r2
		asr r1, 1
		bl GCD
		b GCD_EXIT
	R1_LESS_OR_EQ_R2:

	push {r1}
	sub r1, r2, r1
	asr r1, 1
	pop {r2}
	bl GCD

	GCD_EXIT:

	ldr r0, =max_size
	ldr r5, [r0]
	mov r6, sp
	cmp r5, r6
	ble SP_CHANGED //aka less
		ldr r0, =max_size
		str sp, [r0]
	SP_CHANGED:

	ldmfd sp!, {r1-r2, pc}
	bx lr



main:
	ldr r0, =m
	ldr r1, [r0]

	ldr r0, =n
	ldr r2, [r0]

	ldr r0, =sp_start
	str sp, [r0]
	ldr r0, =max_size
	str sp, [r0]

	mov r4, sp
	mov r5, 0

	bl GCD

	ldr r0, =sp_start
	ldr r4, [r0]
	ldr r0, =max_size
	ldr r5, [r0]
	sub r5, r4, r5
	str r5, [r0]

	ldr r0, =result
	str r3, [r0]

	nop
L:	B L
