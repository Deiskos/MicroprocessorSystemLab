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

int buzzz = 50;
int held = 0;
int output = 0; int last_output = 0;
int map[4][4] = {
		{261, 293, 329, 0},
		{349, 392, 440, 0},
		{493, 523, 0, 0},
		{'-', 0, '+', 0}
};


void GPIO_init(void)
{
	// Enable AHB2 clock
	RCC->AHB2ENR |= 0b00000000000000000000000000000111; // A, B, C

	// PA5 - output, PA0 - alternative
	GPIOA->MODER &= 	0b11111111111111111111011111111110;
	SET_BITS(GPIOA->AFR[0], 0b1111, 0b0010); // AF2
	GPIOA->OSPEEDR &=	0b11111111111111111111101111111110;

	// PC0~PC3 - horizontal - input
	// PC13 - input
	// PC5~PC8 - vertical - output
	GPIOC->MODER =		0b11110011111111010101011100000000;
	GPIOC->OSPEEDR = 	0b00001000000000101010100010101010;
	GPIOC->PUPDR =		0b00000000000000000000000010101010;

	// PB3~PB5 - MAX7219 - output
	// PB10 - alternate - buzzzzzer
	GPIOB->MODER = 		0b11111111111011111111010101111111;
	GPIOB->MODER = 		0b11111111111011111111010101111111;
	GPIOB->OSPEEDR = 	0b00000000001000000000101010000000;

	SET_BITS(GPIOB->AFR[1], 0b1111<<8, 0b0001<<8);
}

int cur_active_led = 0;

void keypad()
{
	for(int i = 0; i < 4; i++)
		GPIOC->BRR = (0b1 << (5+i));
	for(int col = 0; col < 4; col++)
	{
		GPIOC->BSRR = (0b1 << (5+col));
		for(int row = 0; row < 4; row++)
		{
			int found = (GPIOC->IDR >> (0+row)) & 0b1;
			if(found == 0b1)
			{
				last_output = output;
				output = map[row][col];



				if(output == '-')
				{
					if(output == last_output)
						return;
					if(buzzz < 90)
						buzzz += 5;
					output = 0;
				}
				if(output == '+')
				{
					if(output == last_output)
						return;
					if(buzzz > 10)
						buzzz -= 5;
					output = 0;
				}




				return;
			}
			else
				output = 0;
		}
		GPIOC->BRR = (0b1 << (5+col));
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

	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM5EN;
	TIM5->ARR = 100;
	TIM5->PSC = 153;
	TIM5->CCER |= 0b01;				// Capture/Compare 1 complimentary output enabled
	TIM5->CCMR1 |= 0b01100100;		// CC1 as output, combined PWM mode 1
	TIM5->CCR1 = 0;

	TIM5->EGR = TIM_EGR_UG;		// re-init
	TIM5->CR1 |= TIM_CR1_CEN;	// enable

	while(1)
	{
		keypad();
		if(output != 0)
		{
			TIM5->CCR1 = buzzz;
			TIM5->PSC = 4000000/(output*100);
		}
		else
		{
			TIM5->CCR1 = 0;
		}

		busy_sleep(1000);
	}
}
