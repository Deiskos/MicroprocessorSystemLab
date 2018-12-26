#include <stdint.h>
#include "stm32l476xx.h"

#include "cmsis_gcc.h"
#include "core_cmFunc.h"
#include "core_cmSimd.h"
#include "system_stm32l4xx.h"
#include "core_cm4.h"
#include "core_cmInstr.h"

#define BIT_MASK(bit)				(1 << (bit))
#define SET_BIT(value,bit)			((value) |= BIT_MASK(bit))
#define CLEAR_BIT(value,bit)		((value) &= ~BIT_MASK(bit))
#define TEST_BIT(value,bit)			(((value) & BIT_MASK(bit)) ? 1 : 0)
#define SET_BITS(value, mask, newvalue)		(value = (value & ~mask) | (newvalue & mask))

int D0_D7_mode_val = -1;
int busy = 0;
enum
{
	D0_D7_read = 1,
	D0_D7_write = 2,
	D0_D7_readable = 3, // pins as input
	D0_D7_writeable = 4 // pins as output (default)
};
enum
{
	SET,
	RESET,
	READ,
	INVERT
};
enum EDisplayMode
{
	CLEAR_DISPLAY = 0,
	RETURN_HOME,
	ENTRY_MODE_SET,
	DISPLAY_ON_OFF,
	CURSOR_OR_DISPLAY_SHIFT,
	FUNCTION_SET,
	SET_CGRAM_ADDRESS,
	SET_DDRAM_ADDRESS,
	READ_BUSY_FLAG_AND_ADDRESS,
	WRITE_DATA_TO_RAM,
	READ_DATA_FROM_RAM
};

void GPIO_init(void)
{
	// Enable AHB2 clock
	RCC->AHB2ENR |= 0b00000000000000000000000000000111; // A, B, C

	// PA5~PA9 - D0~D4
						//15141312111009080706050403020100
	GPIOA->MODER = 		0b10101011110101010101011101011111;
	GPIOA->OSPEEDR =	0b11000000001010101010100010100000;
	//GPIOA->PUPDR =		0b11001000000000010101010001010100;

	// PB3~PB5 - D5~D7
	// PB6 - RS
	// PB10 - E
						//15141312111009080706050403020100
	GPIOB->MODER = 		0b11111111110111111101010101111111;
	GPIOB->OSPEEDR = 	0b00000000001000000010101010000000;


	// PC7 - RW
						//15141312111009080706050403020100
	GPIOC->MODER = 		0b11111111111111110111111111111111;
	GPIOC->OSPEEDR = 	0b00000000000000001000000000000000;
}

void D0_D7_mode(int mode)
{
	switch(mode)
	{
		case D0_D7_readable:
			// PA5~PA9 - D0~D4
			// PB3~PB5 - D5~D7
							//16151413121110090807060504030201
			GPIOA->MODER &= 0b11111111111111000000000011111111;
			GPIOB->MODER &= 0b11111111111111111111110000001111;
			D0_D7_mode_val = D0_D7_readable;
		break;
		case D0_D7_writeable:
			GPIO_init();
			D0_D7_mode_val = D0_D7_writeable;
		break;
		default:

		break;
	}
}

int D0_D7_rw(int rw, int data)
{
	// PA5~PA9 - D0~D4
	// PB3~PB5 - D5~D7
	switch(rw)
	{
		case D0_D7_read:
			if(D0_D7_mode_val != D0_D7_readable)
				D0_D7_mode(D0_D7_readable);
			int rval = 0;
			rval |= (GPIOA->IDR & (0b11111 << 5)) >> 5; // lower bits
			rval |= (GPIOB->IDR & (0b111 << 3)) << 1;   // higher bits
			return rval;
		break;
		case D0_D7_write:
			if(D0_D7_mode_val != D0_D7_writeable)
				D0_D7_mode(D0_D7_writeable);
			GPIOA->BSRR |= (data & 0b11111) << 5;
			GPIOB->BSRR |= ((data & 0b11100000) >> 5) << 3;
			return 0;
		break;
	}
	return -1;
}

int PIN(GPIO_TypeDef * PORT, int op, int pin)
{
//	if(pin < 0 || pin > 16)
//		return -1;

	switch(op)
	{
		case SET:
			PORT->BSRR |= (1<<pin);
			return 0;
		break;
		case RESET:
			PORT->BRR |= (1<<pin);
			return 0;
		break;
		case READ:
			return ((PORT->IDR & ~(1<<pin)) >> pin);
		break;
		case INVERT:
			if(PIN(PORT, READ, pin) == 1)
				PIN(PORT, RESET, pin);
			else
				PIN(PORT, SET, pin);
		break;
		default:
			return -1;
		break;
	}
	return -1;
}

int LCD_command(int command, int data)
{
	// PA2 - RS, PA3 - RW, PA10 - E
	// PA5~PA9 - D0~D4
	// PB3~PB5 - D5~D7


	while(command != READ_BUSY_FLAG_AND_ADDRESS)
	{
		if(!((LCD_command(READ_BUSY_FLAG_AND_ADDRESS, 0) & (1<<7)) >> 7))
			break;
	}

	switch(command)
	{
		case CLEAR_DISPLAY:
		case RETURN_HOME:
		case ENTRY_MODE_SET:
		case DISPLAY_ON_OFF:
		case FUNCTION_SET:
		case CURSOR_OR_DISPLAY_SHIFT:
		case SET_CGRAM_ADDRESS:
		case SET_DDRAM_ADDRESS:
			PIN(GPIOB, RESET, 6); 	// RS 0
			PIN(GPIOC, RESET, 7); 	// RW 0
			break;
		case READ_BUSY_FLAG_AND_ADDRESS:
			PIN(GPIOB, RESET, 6); 	// RS 0
			PIN(GPIOC, SET, 7); 	// RW 1
			break;
		case WRITE_DATA_TO_RAM:
			PIN(GPIOB, SET, 6); 	// RS 1
			PIN(GPIOC, RESET, 7); 	// RW 0
			break;
		case READ_DATA_FROM_RAM:
			PIN(GPIOB, SET, 6); 	// RS 1
			PIN(GPIOC, SET, 7); 	// RW 1
			break;
		default:

			break;
	}

	TIM5->CNT = 0;
	while(TIM5->CNT < 120) // tsu1 ~120 nanoseconds
		;

	PIN(GPIOB, SET, 10); // E

	int rvalue = 0;
	switch(command)
	{
		case CLEAR_DISPLAY:
		case RETURN_HOME:
		case ENTRY_MODE_SET:
		case DISPLAY_ON_OFF:
		case FUNCTION_SET:
		case CURSOR_OR_DISPLAY_SHIFT:
		case SET_CGRAM_ADDRESS:
		case SET_DDRAM_ADDRESS:
			D0_D7_rw(D0_D7_write, (1<<command) | data);
			break;
		case READ_BUSY_FLAG_AND_ADDRESS:
			rvalue = D0_D7_rw(D0_D7_read, 0);
			break;
		case WRITE_DATA_TO_RAM:
			D0_D7_rw(D0_D7_write, data);
			break;
		case READ_DATA_FROM_RAM:
			rvalue = D0_D7_rw(D0_D7_read, 0);
			break;
		default:
			;
			break;
	}

	TIM5->CNT = 0;
	while(TIM5->CNT < 80) // tsu2 ~80 ns
		;

	PIN(GPIOB, RESET, 10); // E

	TIM5->CNT = 0;
	while(TIM5->CNT < 20) // th1 ~20 ns
		;

	GPIOA->BRR |= 0b11111 << 5;
	GPIOB->BRR |= 0b111 << 3;

	TIM5->CNT = 0;
	while(TIM5->CNT < 400) // tc ~400 ns
		;

	return rvalue;
}

void LCD_Write(char * ptr)
{
	int len  = strlen(ptr);
	if(len <= 16)
		for(int i = 0; i < len; i++)
		{
			//LCD_command(CURSOR_OR_DISPLAY_SHIFT, 0b0100);
			LCD_command(WRITE_DATA_TO_RAM, ptr[i]);
		}
}

int main()
{
	GPIO_init();
	D0_D7_mode(D0_D7_writeable);

	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM5EN;
	TIM5->PSC = 4; // 40/4MHz = 10ns each tim5 increment
	TIM5->EGR = TIM_EGR_UG;		// re-init
	TIM5->CR1 |= TIM_CR1_CEN;	// enable

	LCD_command(FUNCTION_SET, 0b11100);
	LCD_command(ENTRY_MODE_SET, 0b10);
	LCD_command(DISPLAY_ON_OFF, 0b111);
	LCD_command(CLEAR_DISPLAY, 0);
	LCD_command(RETURN_HOME, 0);

	while(1)
	{
		LCD_Write("Group 32");
		TIM5->CNT = 0;
		while(TIM5->CNT < 1000000)
			;
		LCD_command(RETURN_HOME, 0);
		LCD_Write("Tu tu ru");
		TIM5->CNT = 0;
		while(TIM5->CNT < 1000000)
			;
		LCD_command(RETURN_HOME, 0);
	}

	while(1)
		;
}
