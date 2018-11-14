	.syntax unified
	.cpu cortex-m4
	.thumb
.data
	arr: .byte 	0b01111110, 0b00110000,	0b01101101, 0b01111001, 0b00110011, 0b01011011, 0b01011111, 0b01110000, 0b01111111, 0b01111011, 0b01110111, 0b00011111, 0b01001110, 0b00111101, 0b01001111, 0b01000111
	counter: .byte 0

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

main:
	bl GPIO_init
	bl max7219_init

	MAIN_LOOP:

		bl display

		mov r1, 1000
		mov r2, 1000
		bl delay_r1r2

	b MAIN_LOOP

L:	B L

display:
	ldr r2, =arr
	ldr r3, =counter
	ldrb r4, [r3]
	add r2, r2, r4

	push {lr}
	ldr r0, =0b0001
	ldrb r1, [r2]
	bl max7219_send
	pop {lr}

	add r4, r4, 1
	cmp r4, 16
	bne counter_no_loop
		mov r4, #0
	counter_no_loop:
	ldr r3, =counter
	strb r4, [r3]

	bx lr

max7219_send:
	// r0 address 	(4 bits)
	// r1 data		(8 bits)

	// PB3 - DIN
	// PB4 - CLK
	// PB5 - CS
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

	bx lr

GPIO_init:
	// Enable AHB2 clock
	ldr 	r0, =RCC_AHB2ENR
	movs 	r1, #0b00000000000000000000000000000111 // active A and C
	str 	r1, [r0]

	// PB_3, PB_4, PB_5
	ldr		r0, =GPIOB_MODER
	ldr		r1, =0b11111111111111111111010101111111 // output
	str		r1, [r0]

	ldr		r0, =GPIOB_OSPEEDR
	ldr		r1, =0b00000000000000000000101010000000 // high
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
	ldr r1, =0b00000000 // only first digit
	bl max7219_send

	ldr r0, =0b1001 // decode mode register
	ldr r1, =0b00000000 // no decode
	bl max7219_send

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
