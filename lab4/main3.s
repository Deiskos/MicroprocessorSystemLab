	.syntax unified
	.cpu cortex-m4
	.thumb
.data
	leds: .byte 0b0000
	button_ignore: .byte 0x0 // only 1 when button is pressed
	LOOPS: .word 1000
	password: .byte 0b1001

.text
	.global main

	.equ RCC_AHB2ENR, 	0x4002104C
	.equ GPIOB_MODER, 	0x48000400
	.equ GPIOB_OTYPER, 	0x48000404
	.equ GPIOB_OSPEEDR, 0x48000408
	.equ GPIOB_PUPDR, 	0x4800040C
	.equ GPIOB_ODR,		0x48000414

	.equ GPIOC_MODER, 	0x48000800
	.equ GPIOC_OTYPER, 	0x48000804
	.equ GPIOC_OSPEEDR, 0x48000808
	.equ GPIOC_PUPDR, 	0x4800080C
	.equ GPIOC_IDR,		0x48000810
	.equ GPIOC_ODR,		0x48000814

	.equ IOC_MODES, 0b11110011111100000000001111111111 // 6~9 & 13
	.equ IOC_MODER, 0b11110011111100000000001111111111 // 6~9 & 13
	.equ IOC_SPD,	0b00001000000010101010100000000000

	.align 4
main:
	BL GPIO_init
Loop:

	BL DisplayLED

	BL ButtonState

	//BL Delay
	nop

	B Loop

GPIO_init:
	// Enable AHB2 clock
	ldr 	r0, =RCC_AHB2ENR
	movs 	r1, #0b00000000000000000000000000000110 // active B and C
	str 	r1, [r0]

	movs 	r0, #0b11111111111111111101010101111111 // output
	ldr 	r1, =GPIOB_MODER
	ldr 	r2, [r1]
	and 	r2, #0b11111111111111111100000000111111 // MODER3-6
	orrs 	r2, r2, r0
	str 	r2, [r1]

	ldr 	r0, =IOC_MODES // input
	ldr 	r1, =GPIOC_MODER
	ldr 	r2, [r1]
	ldr 	r3, =IOC_MODER
	and 	r2, r3 // MODER 6~9 & 13
	orrs 	r2, r2, r0
	str 	r2, [r1]


	movs 	r0, #0b00000000000000000010101010000000 // pins 3-6
	ldr		r1, =GPIOB_OSPEEDR
	str	r0, [r1]

	ldr 	r0, =IOC_SPD // pins 6~9 & 13
	ldr		r1, =GPIOC_OSPEEDR
	str	r0, [r1]




	BX LR

DisplayLED:
	//TODO: Display LED by leds

	movs 	r1, #0xFF

	ldr 	r0, =leds
	ldrb	r2, [r0]
	lsl		r2, r2, 3
	and 	r1, r1, r2

	ldr		r0, =GPIOB_ODR
	eor		r1, r1, 0xFFFFFFFF
	strh	r1, [r0]
	

	BX LR

ButtonState:
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
			mov r2, 1
			strb r2, [r0]

			// check passwd
			ldr r0, =GPIOC_IDR
			ldr r1, [r0]
			and r1, r1, #0b0000000000000000000001111000000
			lsr r1, 6

			ldr r0, =password
			ldrb r2, [r0]

			cmp r1, r2
			bne NOT_EQUAL
				// equal

				mov r5, 2

				SUCCESS_LOOP:

					ldr r0, =leds
					mov r1, 0b1111
					strb r1, [r0]

					push {lr}
					bl DisplayLED
					pop {lr}

					ldr r0, =LOOPS
					ldr r1, [r0] // saving original value
					mov r2, 700  // shorter delay to debounce
					str r2, [r0]
					push {lr}
						BL Delay
					pop {lr}
					str r1, [r0] // restoring

					ldr r0, =leds
					mov r1, 0b0000
					strb r1, [r0]

					push {lr}
					bl DisplayLED
					pop {lr}

					ldr r0, =LOOPS
					ldr r1, [r0] // saving original value
					mov r2, 700  // shorter delay to debounce
					str r2, [r0]
					push {lr}
						BL Delay
					pop {lr}
					str r1, [r0] // restoring

					sub r5, r5, 1
					cmp r5, 0
					bne SUCCESS_LOOP

			NOT_EQUAL:


				ldr r0, =leds
				mov r1, 0b1111
				strb r1, [r0]

				push {lr}
				bl DisplayLED
				pop {lr}

				ldr r0, =LOOPS
				ldr r1, [r0] // saving original value
				mov r2, 700  // shorter delay to debounce
				str r2, [r0]
				push {lr}
					BL Delay
				pop {lr}
				str r1, [r0] // restoring

				ldr r0, =leds
				mov r1, 0b0000
				strb r1, [r0]

				push {lr}
				bl DisplayLED
				pop {lr}

		BTN_IGNORE:
		b BTN_STATE_EXIT
	BUTTON_1:
	ldr r0, =button_ignore
	mov r2, 0
	strb r2, [r0]




	BTN_STATE_EXIT:

	ldr r0, =LOOPS
	ldr r1, [r0] // saving original value
	mov r2, 10  // shorter delay to debounce
	str r2, [r0]
	push {lr}
		BL Delay
	pop {lr}
	str r1, [r0] // restoring

	BX LR

Delay:
	push {r0-r4}
	ldr r0, =LOOPS
	LDR R3, [r0]
	L1:
		LDR R4,	[r0]
		L2:
			SUBS R4,	#1
			BNE L2
		SUBS R3,	#1
		BNE L1
	pop {r0-r4}
	BX LR
