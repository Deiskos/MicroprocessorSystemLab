	.syntax unified
	.cpu cortex-m4
	.thumb
.data
	leds: .byte 0b000011
	stahp: .byte 0b0
	mvdir: .byte 0b0 // 0 left, 1 right
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

	ldr r0, =stahp
	ldrb r1, [r0]
	cmp r1, 0b1
	beq GO_ON
		BL MoveLights
		BL Delay
	GO_ON:
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

	ldr 	r0, =leds
	ldrb	r2, [r0]
	and		r2, 0b011110
	lsl		r2, r2, 2

	ldr		r0, =GPIOB_ODR
	eor		r2, r2, 0xFFFFFFFF
	strh	r2, [r0]

	BX LR

MoveLights:
	ldr r0, =leds
	ldrb r1, [r0]

	and r1, r1, 0b1

	cmp r1, 0b1
	bne NOT_RIGHTMOST
		ldr r0, =mvdir
		mov r2, 0b0
		strb r2, [r0]
	NOT_RIGHTMOST:

	ldr r0, =leds
	ldr r1, [r0]
	and r1, r1, 0b100000
	cmp r1, 0b100000
	bne NOT_LEFTMOST
		ldr r0, =mvdir
		mov r2, 0b1
		strb r2, [r0]
	NOT_LEFTMOST:

	ldr r0, =leds
	ldrb r1, [r0]
	ldr r0, =mvdir
	ldrb r2, [r0]

	cmp r2, 0b0
	beq LEFT
		// right here
		lsr r1, r1, 1
		ldr r0, =leds
		strb r1, [r0]
		b MOVE_EXIT
	LEFT:
		// left here
		lsl r1, r1, 1
		ldr r0, =leds
		strb r1, [r0]

	MOVE_EXIT:
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

			// stahp
			ldr r0, =stahp
			ldrb r1, [r0]
			eor r1, r1, 0b1
			strb r1, [r0]

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
