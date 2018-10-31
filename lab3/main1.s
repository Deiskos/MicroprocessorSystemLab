	.syntax unified
	.cpu cortex-m4
	.thumb

.data
	user_stack: .zero 128
	expr_result: .word 0

.text
	.global main
	postfix_expr: .asciz "-100 -10 + 5 - 15 +"
	.align 4
main:
	//r1 - temp
	//r2 - return value for atoi OR lvalue
	//r3 - rvalue
	//r12 - old sp value

	mov r12, sp
	ldr sp, =user_stack
	add sp, 128
	ldr r0, =postfix_expr


	MAIN_LOOP:
		ldrb r1, [r0]
		cmp r1, 0
		beq MAIN_LOOP_EXIT

		cmp r1, ' '
		bne NOT_SPACE
			add r0, 1
			b MAIN_LOOP
		NOT_SPACE:

		cmp r1, '-'
		bne NOT_MINUS
			add r0, 1
			ldrb r1, [r0]
			cmp r1, ' '
			beq NOT_A_NEG_NUM
				sub r0, 1
				bl atoi			// if it's a negative number
				push {r2}
				b MAIN_LOOP
			NOT_A_NEG_NUM:
			pop {r2-r3}
			sub r2, r3, r2 // r2 = r3-r2
			push {r2}
			b MAIN_LOOP
		NOT_MINUS:


		cmp r1, '+'
		bne NOT_PLUS
			add r0, 1
			pop {r2-r3}
			add r2, r3, r2 // r2 = r3-r2
			push {r2}
			b MAIN_LOOP
		NOT_PLUS:

		sub r0, 1
		bl atoi
		push {r2}

	b MAIN_LOOP

	MAIN_LOOP_EXIT:

	pop {r1}

	ldr r0, =expr_result
	str r1, [r0]

	mov sp, r12

	nop
L:	B L


atoi:
	mov r1, 0 // character OR temp
	mov r2, 0 // answer
	mov r3, 1 // sign

	ldrb r1, [r0]
	add r0, 1
	cmp r1, '-'
	bne NOT_MINUS_ATOI
		mov r3, -1
	NOT_MINUS_ATOI:
		ATOI_LOOP:
			ldrb r1, [r0]
			add r0, 1
			cmp r1, ' '
			beq ATOI_EXIT
			cmp r1, 0
			beq ATOI_EXIT

			ATOI_NOT_END_OF_TOKEN:
			sub r1, '0'
			push {r1}
			mov r1, 10
			mul r2, r1
			pop {r1}
			add r2, r1
		b ATOI_LOOP

	ATOI_EXIT:
	sub r0, 1
	mul r2, r3

	bx lr
