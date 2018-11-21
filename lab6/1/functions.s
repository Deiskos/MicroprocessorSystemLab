	.syntax unified
	.cpu cortex-m4
	.thumb
.data

.text
	.equ RCC_AHB2ENR, 	0x4002104C
	.equ GPIOB_MODER, 	0x48000400
	.equ GPIOB_OTYPER, 	0x48000404
	.equ GPIOB_OSPEEDR, 0x48000408
	.equ GPIOB_PUPDR, 	0x4800040C
	.equ GPIOB_IDR,		0x48000410
	.equ GPIOB_ODR,		0x48000414
	.equ GPIOB_BSRR, 	0x48000418
	.equ GPIOB_BRR,		0x48000428

	// .equ GPIOC_MODER, 	0x48000800
	// .equ GPIOC_OTYPER, 	0x48000804
	// .equ GPIOC_OSPEEDR, 0x48000808
	// .equ GPIOC_PUPDR, 	0x4800080C
	// .equ GPIOC_IDR,		0x48000810
	// .equ GPIOC_ODR,		0x48000814

	.global GPIO_init
	.global max7219_init

	.global display_reset
	.global max7219_send


display_reset:
	push {r4-r11, lr}
	mov r4, 8
	reset_loop:

		mov r0, r4
		mov r1, 0
		bl max7219_send

		cmp r4, 1
		sub r4, r4, 1
		bne reset_loop

	pop {r4-r11, lr}
	bx lr

max7219_send:
	// r0 address 	(4 bits)
	// r1 data		(8 bits)

	// PB3 - DIN
	// PB4 - CLK
	// PB5 - CS

	push {r4-r11, lr}

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

	pop {r4-r11, lr}

	bx lr

GPIO_init:
	push {r4-r11, lr}

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
	// ldr		r0, =GPIOC_MODER
	// ldr		r1, =0b11110011111111111111111111111111
	// str		r1, [r0]

	// ldr		r0, =GPIOC_OSPEEDR
	// ldr		r1, =0b00001000000000000000000000000000
	// str		r1, [r0]

	// ldr		r0, =GPIOC_PUPDR
	// ldr		r1, =0b00000100000000000000000000000000
	// str		r1, [r0]

	pop {r4-r11, lr}

	bx lr

max7219_init:
	push {r4-r11, lr}

	ldr r0, =0b1100 //shutdown register
	ldr r1, =0b00000001 // normal operation
	bl max7219_send

	ldr r0, =0b1111 // display test register
	ldr r1, =0b00000000 // no
	bl max7219_send

	ldr r0, =0b1011 // scan limit register
	ldr r1, =0b00000110 // 0-6
	bl max7219_send

	ldr r0, =0b1001 // decode mode register
	ldr r1, =0b11111111 // all decode
	bl max7219_send

	bl display_reset

	pop {r4-r11, pc}

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
