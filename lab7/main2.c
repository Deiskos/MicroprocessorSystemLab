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

#define TIME_SEC_TIMES_100 636

short int decode[10] = {0b01111110, 0b00110000,	0b01101101, 0b01111001, 0b00110011, 0b01011011, 0b01011111, 0b01110000, 0b01111111, 0b01111011};

int cur_active_led = 0;


void GPIO_init(void)
{
	// Enable AHB2 clock
	RCC->AHB2ENR |= 0b00000000000000000000000000000111; // A, B, C

	// PA5 - output
	GPIOA->MODER &= 	0b11111111111111111111011111111111;
	GPIOA->OSPEEDR &=	0b11111111111111111111101111111111;

	// PC0~PC3 - horizontal - input
	// PC13 - input
	// PC5~PC8 - vertical - output
	GPIOC->MODER =		0b11110011111111010101011100000000;
	GPIOC->OSPEEDR = 	0b00001000000000101010100010101010;
	GPIOC->PUPDR =		0b00000000000000000000000010101010;

	// PB3~PB5 - MAX7219 - output
	GPIOB->MODER = 		0b11111111111111111111010101111111;
	GPIOB->OSPEEDR = 	0b00000000000000000000101010000000;
}

void max7219_send(int address, int data)
{
	address = address<<8;
	data += address;

	GPIOB->BRR = (1<<5);
	for(int i = 15; i >= 0; i--) // 16 times
	{
		GPIOB->BRR = (1<<3);
		GPIOB->BRR = (1<<4);
		GPIOB->BSRR = ((data>>i)&0b1)<<3;
		GPIOB->BSRR = (1<<4);
	}
	GPIOB->BSRR = (1<<5);
	GPIOB->BRR = (1<<4);
}

void clear_screen()
{
	for(int i = 0; i < 8; i++)
		max7219_send(i+1, 0x0); // clear
}

void screen(int to_print)
{
	if(to_print != 0)
	{
		int tmp = to_print, nsymb = 0;
		short int stack[10] = {0};
		while(tmp != 0)
		{
			stack[nsymb] = tmp%10;
			tmp /= 10;
			nsymb++;
		}

		if(nsymb < cur_active_led)
		{
			cur_active_led = nsymb;
			clear_screen();
		}
		else
			cur_active_led = nsymb;

		if(nsymb == 1)
		{
			stack[1] = 0;
			stack[2] = 0;
			nsymb = 3;
		}
		if(nsymb == 2)
		{
			stack[2] = 0;
			nsymb = 3;
		}

		for(int i = nsymb; i > 0; i--)
		{
			if(i == 3)
				max7219_send(i, (decode[stack[i-1]] | 0b10000000));
			else
				max7219_send(i, decode[stack[i-1]]);
		}
	}
	else
	{
		if(1 < cur_active_led)
		{
			cur_active_led = 1;
			clear_screen();
		}
		else
			cur_active_led = 1;

		max7219_send(1, 0);
	}
}

void busy_sleep(int loops)
{
	for(int i = 0; i < loops; i++)
	{
		i += 1;
		i -= 1;
	}
}


int main()
{
	GPIO_init();
	max7219_send(0b1100, 0b00000001); // shutdown register | normal operation
	max7219_send(0b1111, 0b00000000); // test | no
	max7219_send(0b1011, 0b00000111); // scan limit | 1-7
	max7219_send(0b1001, 0b00000000); // decode | 1-7

	clear_screen();

	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM5EN;
	TIM5->PSC = 39999; //4000
	TIM5->ARR = 99999999;
	TIM5->EGR = TIM_EGR_UG;
	TIM5->CR1 |= TIM_CR1_CEN;


	while(1)
	{
		if(TIM5->CNT <= TIME_SEC_TIMES_100)
			screen(TIM5->CNT);
		busy_sleep(1000);
	}

}


