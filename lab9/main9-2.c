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

char bitmap1[8] = {	0b10101,
					0b01010,
					0b10101,
					0b01010,
					0b10101,
					0b01010,
					0b10101,
					0b01010};
int b1_addr = 0x01;
int b1_dir = 1;	// right

char bitmap2[8] = {	0b00000,
					0b00000,
					0b00000,
					0b01010,
					0b00100,
					0b10011,
					0b10101,
					0b11001};

int b2_addr = 0x4E;
int b2_dir = 0;	// left

int mode = 0;

int second_mode_pos = 0;
int second_mode_dir = 1;
char * second_mode_str = "Pickle Rick!";

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
	// PC13 - input
						//15141312111009080706050403020100
	GPIOC->MODER = 		0b11110011111111110111111111111111;
	GPIOC->OSPEEDR = 	0b00001000000000001000000000000000;
}

void EXTI_config()
{
	// EXTI13 - PC13
	EXTI->IMR1 |= 1 << 13;
	EXTI->EMR1 |= 1 << 13;
	EXTI->FTSR1 |= 1 << 13;

	RCC->APB2ENR |= 0b1; 				// enable SYSCFG
	SYSCFG->EXTICR[3] |= 0b00100000;	// configure EXTI13 to use PC13
}

void NVIC_config()
{
	NVIC->ISER[1] |= 1 << 8; // interrupt 40 - EXTI10_15
}

void EXTI15_10_IRQHandler()
{
	if((EXTI->PR1 & (1 << 13)) != 0)
	{
		EXTI->PR1 = 1 << 13;
		mode = !mode;
		LCD_command(CLEAR_DISPLAY, 0);
	}
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

void LCD_Write_str(char * ptr)
{
	int len  = strlen(ptr);
	if(len <= 16)
		for(int i = 0; i < len; i++)
		{
			LCD_command(WRITE_DATA_TO_RAM, ptr[i]);
		}
}

void LCD_Write_at(int addr, char c)
{
	int AC = LCD_command(READ_BUSY_FLAG_AND_ADDRESS, 0);
	AC &= 0b01111111; // discard busy flag

	LCD_command(SET_DDRAM_ADDRESS, addr);
	LCD_command(WRITE_DATA_TO_RAM, c);

	LCD_command(SET_DDRAM_ADDRESS, AC);
}

void LCD_create_char(int CGRAM_N, char * bitmap)
{
	CGRAM_N--;
	int AC = LCD_command(READ_BUSY_FLAG_AND_ADDRESS, 0);
	AC &= 0b01111111; // discard busy flag
	for(int i = 0; i < 8; i++)
	{
		LCD_command(SET_CGRAM_ADDRESS, (CGRAM_N << 3) | i);
		LCD_command(WRITE_DATA_TO_RAM, bitmap[i]);
	}

	LCD_command(SET_DDRAM_ADDRESS, AC);
}

void SysTick_Handler()
{
	if(mode == 0)
	{
		LCD_Write_at(b1_addr, ' ');
		if(b1_addr == 0x0F || b1_addr == 0x00)
			b1_dir = !b1_dir;
		if(b1_dir)
			b1_addr++;
		else
			b1_addr--;
		LCD_Write_at(b1_addr, 0);

		LCD_Write_at(b2_addr, ' ');
		if(b2_addr == 0x4F || b2_addr == 0x40)
			b2_dir = !b2_dir;
		if(b2_dir)
			b2_addr++;
		else
			b2_addr--;
		LCD_Write_at(b2_addr, 1);
	}
	else
	{
		LCD_command(CLEAR_DISPLAY, 0);
		int len = strlen(second_mode_str);
		for(int i = 0; i < len; i++)
		{
			if(!(((second_mode_pos+i) > 16) || ((second_mode_pos+i) < 0)))
			{
				LCD_Write_at(second_mode_pos+i, second_mode_str[i]);
			}
		}
		if(second_mode_pos == 0x0F || (second_mode_pos == 0x00-len))
			second_mode_dir = !second_mode_dir;
		if(second_mode_dir)
			second_mode_pos++;
		else
			second_mode_pos--;


	}
}

int main()
{
	GPIO_init();
	D0_D7_mode(D0_D7_writeable);

	NVIC_config();
	EXTI_config();


	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM5EN;
	TIM5->PSC = 4; // 40/4MHz = 10ns each tim5 increment
	TIM5->EGR = TIM_EGR_UG;		// re-init
	TIM5->CR1 |= TIM_CR1_CEN;	// enable

	LCD_command(FUNCTION_SET, 0b11100);
	LCD_command(ENTRY_MODE_SET, 0b10);
	LCD_command(DISPLAY_ON_OFF, 0b100);
	LCD_command(CLEAR_DISPLAY, 0);
	LCD_command(RETURN_HOME, 0);

	LCD_create_char(1, bitmap1);
	LCD_create_char(2, bitmap2);

	SysTick_Config(1200000); // every 0.3s assuming sysclk is 4MHz

	while(1)
		;
}
