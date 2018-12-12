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

int seconds = 0;
uint32_t time_ms = 100;

void GPIO_init(void)
{
	// Enable AHB2 clock
	RCC->AHB2ENR = 		0b00000000000000000000000000000001; // A, B, C

	// PA5 - output
	GPIOA->MODER &= 	0b11111111111111111111011111111111;
	GPIOA->OSPEEDR &=	0b11111111111111111111101111111111;
}

void SystemClockInit()
{
	RCC->CR |= RCC_CR_HSION;
	while((RCC->CR & RCC_CR_HSION) == 0)
		;

	SET_BITS(RCC->CFGR, 0b1111<<4, 0b1011<<4); 	// sysclk/16

	SET_BITS(RCC->CFGR, 0b11, 0b01);			// HSI16 as system clock
}

void SysTick_Handler()
{
	if(time_ms == 0)
	{
		time_ms = 100;
		seconds++;
	}
	else
		time_ms--;

	if(seconds == 1)
	{
		GPIOA->BRR |= 1<<5;
	}
	if(seconds == 2)
	{
		GPIOA->BSRR |= 1<<5;
		seconds = 0;
	}
}

int main(void)
{
	SystemClockInit();
	GPIO_init();

	GPIOA->BSRR |= 1<<5;

	SysTick_Config(1E6/100); // 1MHz/10

	while(1)
		;
}
