	.syntax unified
	.cpu cortex-m4
	.thumb
.data
	fibnum: .word 0
	fibcount: .word 2 // max is 39
	held: .word 0
	button_ignore: .byte 0

.text
	.global main

	.equ RCC_AHB2ENR, 	0x4002104C
	.equ GPIOB_MODER, 	0x48000400
	.equ GPIOB_OTYPER, 	0x48000404
	.equ GPIOB_OSPEEDR, 0x48000408
	.equ GPIOB_PUPDR, 	0x4800040C
	.equ GPIOB_IDR,		0x48000410
	.equ GPIOB_ODR,		0x48000414
	.equ GPIOB_BSRR, 	0x48000418
	.equ GPIOB_BRR,		0x48000428

	.equ GPIOC_MODER, 	0x48000800
	.equ GPIOC_OTYPER, 	0x48000804
	.equ GPIOC_OSPEEDR, 0x48000808
	.equ GPIOC_PUPDR, 	0x4800080C
	.equ GPIOC_IDR,		0x48000810
	.equ GPIOC_ODR,		0x48000814

main:
	bl GPIO_init
	bl max7219_init

	MAIN_LOOP:

		bl button

		bl display

		mov r1, 20
		mov r2, 1000
		bl delay_r1r2

	b MAIN_LOOP

L:	B L

button:
	push {lr}

	ldr r0, =GPIOC_IDR
	ldr r1, [r0]
	and r1, r1, #0b0000000000000000010000000000000
	lsr r1, 13

	cmp r1, 1
	beq BUTTON_1
		// button 0

		ldr r0, =button_ignore
		ldrb r2, [r0]
		cmp r2, 1
		beq BTN_IGNORE
			// btn pressed
			ldr r0, =fibcount
			ldr r1, [r0]
			mov r0, r1

			bl fib
			ldr r0, =fibnum
			str r4, [r0]

			ldr r0, =fibcount
			ldr r4, [r0]
			cmp r4, 39
			bne fib_ok
				/*
					ldr r0, =fibcount
					mov r4, 0
					str r4, [r0]
					ldr r0, =fibnum
					str r4, [r0]
					bl display_reset
				*/
				ldr r0, =fibnum
				mov r1, -1
				str r1, [r0]
				ldr r0, =0b1011 // scan limit register
				ldr r1, =0b00000001 // 0-1
				bl max7219_send
				bl display_reset
			fib_ok:
				ldr r0, =fibcount
				ldr r4, [r0]
				add r4, 1
				str r4, [r0]

			ldr r0, =button_ignore
			mov r1, 1
			strb r1, [r0]

		BTN_IGNORE:
			ldr r0, =held
			ldr r1, [r0]
			add r1, r1, 1
			str r1, [r0]
			cmp r1, 100
			blt BTN_STATE_EXIT
				mov r1, 0
				str r1, [r0]

				ldr r0, =fibcount
				str r1, [r0]
				ldr r0, =fibnum
				str r1, [r0]

				ldr r0, =0b1011 // scan limit register
				ldr r1, =0b00000111 // 0-6
				bl max7219_send

				bl display_reset
				b BTN_STATE_EXIT
	BUTTON_1:
		ldr r0, =button_ignore
		mov r2, 0
		strb r2, [r0]
		ldr r0, =held
		str r2, [r0]




	BTN_STATE_EXIT:


		mov r1, 10
		mov r2, 10
		BL delay_r1r2

	pop {lr}

	BX LR







display_reset:
	push {lr}
	mov r4, 8
	reset_loop:

		mov r0, r4
		mov r1, 0
		bl max7219_send

		cmp r4, 1
		sub r4, r4, 1
		bne reset_loop

	pop {lr}
	bx lr

display:
	push {lr}


	mov r5, 0
	ldr r3, =fibnum
	ldr r4, [r3]
	cmp r4, -1
	beq NEGATIVE
		display_push_loop:

			mov r3, r4
			mov r2, 10
			udiv r4, r4, r2
			mul r4, r4, r2
			sub r3, r3, r4
			push {r3}

			add r5, 1

			cmp r4, 0
			mov r3, 10
			udiv r4, r4, r3
			bne display_push_loop

		mov r4, r5
		display_main_loop:

			mov r0, r4
			pop {r1}
			bl max7219_send

			cmp r4, 1
			sub r4, r4, 1
			bne display_main_loop
		b DISP_EXIT
	NEGATIVE:
		mov r0, 2
		mov r1, 10
		bl max7219_send

		mov r0, 1
		mov r1, 1
		bl max7219_send
	DISP_EXIT:
	pop {lr}
	bx lr

max7219_send:
	// r0 address 	(4 bits)
	// r1 data		(8 bits)

	// PB3 - DIN
	// PB4 - CLK
	// PB5 - CS

	push {r0-r6}

	lsl r0, r0, 8
	add r1, r0, r1

	ldr r0, =GPIOB_BRR
	ldr r3, =(1<<5)
	str r3, [r0]

	mov r2, 15 // 16 times
	send_loop:
		ldr r0, =GPIOB_BRR
		ldr r3, =(1<<3)
		str r3, [r0]
		ldr r3, =(1<<4)
		str r3, [r0]

		mov r3, 1
		lsl r3, r3, r2

		and r3, r3, r1
		lsr r3, r3, r2

		ldr r0, =GPIOB_BSRR
		lsl r3, r3, 3
		str r3, [r0]

		ldr r3, =(1<<4)
		str r3, [r0]

		cmp r2, 0
		sub r2, r2, 1
		bne send_loop

	ldr r0, =GPIOB_BSRR
	ldr r3, =(1<<5)
	str r3, [r0]

	ldr r0, =GPIOB_BRR
	ldr r3, =(1<<5)
	str r3, [r0]
	ldr r3, =(1<<4)
	str r3, [r0]

	pop {r0-r6}

	bx lr



fibonacci:
	push  {r1-r3}
	mov   r1,  #0
	mov   r2,  #1
	fibloop:
		mov   r3,  r2
		adds   r2,  r1,  r2
		bvs overflow
		mov   r1,  r3
		subs   r0,  r0,  #1
		cmp   r0,  #1
		bne   fibloop
	mov   r0,  r2
	b FIBONACCI_EXIT

	overflow:
	mov r0, #-1

	FIBONACCI_EXIT:
	pop   {r1-r3}
	bx lr

fib:
	push {lr}

	cmp r0, #0
	beq fib_0
	cmp r0, #1
	beq fib_1
	cmp r0, #39
	bgt out_of_bounds

		bl fibonacci
		mov r4, r0
		b FIB_EXIT

	out_of_bounds:
		mov r4, #-1
		b FIB_EXIT
	fib_0:
		mov r4, #0
		b FIB_EXIT
	fib_1:
		mov r4, #1
		b FIB_EXIT
	FIB_EXIT:

	ldr r0, =fibnum
	str r4, [r0]


	pop {lr}
	bx lr



GPIO_init:
	// Enable AHB2 clock
	ldr 	r0, =RCC_AHB2ENR
	movs 	r1, #0b00000000000000000000000000000110 // active B and C
	str 	r1, [r0]

	// PB_3, PB_4, PB_5
	ldr		r0, =GPIOB_MODER
	ldr		r1, =0b11111111111111111111010101111111 // output
	str		r1, [r0]

	ldr		r0, =GPIOB_OSPEEDR
	ldr		r1, =0b00000000000000000000101010000000 // high
	str		r1, [r0]

	//-----------

	// PC_13
	ldr		r0, =GPIOC_MODER
	ldr		r1, =0b11110011111111111111111111111111
	str		r1, [r0]

	ldr		r0, =GPIOC_OSPEEDR
	ldr		r1, =0b00001000000000000000000000000000
	str		r1, [r0]

	ldr		r0, =GPIOC_PUPDR
	ldr		r1, =0b00000100000000000000000000000000
	str		r1, [r0]

	bx lr

max7219_init:
	push {lr}

	ldr r0, =0b1100 //shutdown register
	ldr r1, =0b00000001 // normal operation
	bl max7219_send

	ldr r0, =0b1111 // display test register
	ldr r1, =0b00000000 // no
	bl max7219_send

	ldr r0, =0b1011 // scan limit register
	ldr r1, =0b00000111 // 0-6
	bl max7219_send

	ldr r0, =0b1001 // decode mode register
	ldr r1, =0b11111111 // all decode
	bl max7219_send

	bl display_reset

	pop {lr}

	bx lr


delay_r1r2:
	// r1, r2 - loops to do
	// r3, r4 - counters
	push {r0-r4}
	mov R3, r1
	L1:
		mov R4,	r2
		L2:
			SUBS R4,	#1
			BNE L2
		SUBS R3,	#1
		BNE L1
	pop {r0-r4}
	BX LR
