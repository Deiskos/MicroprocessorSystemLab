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

int ALARM = 0;
int timer_initial = 0;

int map[4][4] = {
		{1, 2, 3, 10},
		{4, 5, 6, 11},
		{7, 8, 9, 12},
		{15, 0, 14, 13}
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
	GPIOB->MODER = 		0b11111111111011111111010101111111;
	GPIOB->MODER = 		0b11111111111011111111010101111111;
	GPIOB->OSPEEDR = 	0b00000000001000000000101010000000;

	SET_BITS(GPIOB->AFR[1], 0b1111<<8, 0b0001<<8);
}

void EXTI_config()
{
	// EXTI0~EXTI3 - PC0~PC3
	EXTI->IMR1 |= 0b1111;
	EXTI->RTSR1 |= 0b1111;

	RCC->APB2ENR |= 0b1; 						// enable SYSCFG
	SYSCFG->EXTICR[0] |= 0b0010001000100010;	// configure EXTI0~EXTI3 to use PC0~PC3
}

void NVIC_config()
{
	NVIC->ISER[0] |= 0b1111<<6; // interrupts 6~9 - EXTI0~EXTI3
}

int keypad(int triggered)
{
	int output = 0;

	for(int i = 0; i < 4; i++)
		GPIOC->BRR = (0b1 << (5+i));

	for(int col = 0; col < 4; col++)
	{
		GPIOC->BSRR = (0b1 << (5+col));

		int found = (GPIOC->IDR >> (0+triggered)) & 0b1;
		if(found == 0b1)
		{
			output = map[triggered][col];
			break;
		}
		else
			output = 0;

		GPIOC->BRR = (0b1 << (5+col));
	}

	for(int i = 0; i < 4; i++)
			GPIOC->BSRR = (0b1 << (5+i));

	return 2*output;
}

void EXTI0_IRQHandler()
{
	if((EXTI->PR1 & 0b0001) != 0)
	{
		EXTI->PR1 = 1 << 0;
		if(timer_initial == 0)
			timer_initial = keypad(0);
	}
}

void EXTI1_IRQHandler()
{
	if((EXTI->PR1 & 0b0010) != 0)
	{
		EXTI->PR1 = 1 << 1;
		if(timer_initial == 0)
			timer_initial = keypad(1);
	}
}

void EXTI2_IRQHandler()
{
	if((EXTI->PR1 & 0b0100) != 0)
	{
		EXTI->PR1 = 1 << 2;
		if(timer_initial == 0)
			timer_initial = keypad(2);
	}
}

void EXTI3_IRQHandler()
{
	if((EXTI->PR1 & 0b1000) != 0)
	{
		EXTI->PR1 = 1 << 3;
		if(timer_initial == 0)
			timer_initial = keypad(3);
	}
}

void SysTick_Handler()
{
	if(timer_initial == 0)
	{
		return;
	}
	else
	{
		if(timer_initial == 1)
		{
			ALARM = 1;
			timer_initial = 0;
		}
		else
			timer_initial--;
	}
}





int main(void)
{
	GPIO_init();
	NVIC_config();
	EXTI_config();


	GPIOC->BSRR |= 0b1111<<5;

	SysTick_Config(1E6); // every half second assuming SYSCLK is 4MHz

	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM5EN;
	TIM5->ARR = 100;
	TIM5->PSC = 153;
	TIM5->CCER |= 0b01;				// Capture/Compare 1 complimentary output enabled
	TIM5->CCMR1 |= 0b01100100;		// CC1 as output, combined PWM mode 1
	TIM5->CCR1 = 50;

	TIM5->EGR = TIM_EGR_UG;		// re-init
	//TIM5->CR1 |= TIM_CR1_CEN;	// enable

	while(1)
	{
		if(ALARM)
			TIM5->CR1 |= TIM_CR1_CEN;	// enable

		if((GPIOC->IDR & 1<<13) == 0)
		{
			ALARM = 0;
			TIM5->CR1 &= ~TIM_CR1_CEN;	// disable
		}

	}
}
