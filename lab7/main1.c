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

enum
{
	CLK_1_MHZ,
	CLK_6_MHZ,
	CLK_10_MHZ,
	CLK_16_MHZ,
	CLK_40_MHZ
};
int selected_speed = CLK_1_MHZ;
int user_button_pressed = 0;


void GPIO_init(void)
{
	// Enable AHB2 clock
	RCC->AHB2ENR = 0b00000000000000000000000000000111; // A, B, C

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

void SystemClockInit()
{
//	RCC->CR |= RCC_CR_HSION;
//	RCC->CFGR = 0b1011<<4; // hclk/16 = 1 MHz
//	while((RCC->CR & RCC_CR_HSIRDY) == 0)
//		;
	RCC->CR |= RCC_CR_MSION;
	while((RCC->CR & RCC_CR_MSIRDY) == 0)
		;
	SET_BITS(RCC->CR, 0b1111<<4, 0b0100<<4); // MSI = 1 MHz

	// PLL initial configuration
	// output@PLLR = (input*(PLLN/PLLM))/PLLR;
	// 1 = (1*(8/1))/8;
	RCC->PLLCFGR |= (0b11<<25);		// PLLR = 8
	RCC->PLLCFGR |= (0b0001000<<8); // PLLN = 8
	RCC->PLLCFGR |= (0b000<<4); 	// PLLM = 1
//	RCC->PLLCFGR |= (0b10<<0); 		// PLLSRC = 10 (HSI16)
	RCC->PLLCFGR |= (0b01<<0); 		// PLLSRC = 01 (MSI)

	//FLASH->ACR = FLASH_ACR_ACC64;
	//FLASH->ACR |= 0b001;
	//FLASH->ACR |= FLASH_ACR_PRFTEN;  // prefetch enable

	SET_BIT(RCC->CR, 24);

	RCC->PLLCFGR |= (0b1<<24); 		// PLLR Enable
	RCC->PLLCFGR |= (0b0<<20); 		// PLLQ Disable
	RCC->PLLCFGR |= (0b0<<16); 		// PLLP Disable

	while((RCC->CR & RCC_CR_PLLRDY) == 0)
			;

	RCC->CFGR |= 0b11; // PLL
}

void busy_sleep(int loops) // pass 1000 for ~1 second
{
	for(int i = 0; i < loops; i++)
	{
		for(int j = 0; j < loops; j++)
		{
			i += 1;
			i -= 1;
		}
	}
	return;
}

void SystemClock_Config()
{
	selected_speed++;
	if(selected_speed > CLK_40_MHZ)
		selected_speed = CLK_1_MHZ;

	switch(selected_speed)
	{
		case CLK_1_MHZ:
			SET_BITS(RCC->CR, 0b1111<<4, 0b0100<<4);
			SET_BITS(RCC->CFGR, 0b11, 0b00);
			RCC->CR &= ~(1<<24);
			while((RCC->CR & RCC_CR_PLLRDY) != 0) // wait to turn off
				;

			// output@PLLR = (input*(PLLN/PLLM))/PLLR;
			// 1 = (1*(8/1))/8;
			SET_BITS(RCC->PLLCFGR, (0b11<<25), (0b11<<25));				// PLLR = 8
			SET_BITS(RCC->PLLCFGR, (0b1111111<<8), (0b0001000<<8));		// PLLN = 8
			SET_BITS(RCC->PLLCFGR, (0b111<<4), (0b000<<4));				// PLLM = 1
//			SET_BITS(RCC->PLLCFGR, (0b11<<0), (0b10<<0));				// PLLSRC = 10 (HSI16)

			SET_BIT(RCC->CR, 24);
			while((RCC->CR & RCC_CR_PLLRDY) == 0)
					;
			RCC->CFGR |= 0b11; // PLL
		break;
		case CLK_6_MHZ:
			SET_BITS(RCC->CFGR, 0b11, 0b00);
			RCC->CR &= ~(1<<24);
			while((RCC->CR & RCC_CR_PLLRDY) != 0) // wait to turn off
				;

			// output@PLLR = (input*(PLLN/PLLM))/PLLR;
			// 6 = (1*(12/1))/2;
			SET_BITS(RCC->PLLCFGR, (0b11<<25), (0b00<<25));				// PLLR = 2
			SET_BITS(RCC->PLLCFGR, (0b1111111<<8), (0b0001100<<8));		// PLLN = 12
			SET_BITS(RCC->PLLCFGR, (0b111<<4), (0b000<<4));				// PLLM = 1
//			SET_BITS(RCC->PLLCFGR, (0b11<<0), (0b10<<0));				// PLLSRC = 10 (HSI16)

			SET_BIT(RCC->CR, 24);

			RCC->PLLCFGR |= (0b1<<24); 		// PLLR Enable
			RCC->PLLCFGR |= (0b0<<20); 		// PLLQ Disable
			RCC->PLLCFGR |= (0b0<<16); 		// PLLP Disable

			while((RCC->CR & RCC_CR_PLLRDY) == 0)
					;
			RCC->CFGR |= 0b11; // PLL
		break;
		case CLK_10_MHZ:
			SET_BITS(RCC->CFGR, 0b11, 0b00);
			RCC->CR &= ~(1<<24);
			while((RCC->CR & RCC_CR_PLLRDY) != 0) // wait to turn off
				;

			// output@PLLR = (input*(PLLN/PLLM))/PLLR;
			// 10 = (1*(20/1))/2;
			SET_BITS(RCC->PLLCFGR, (0b11<<25), (0b00<<25));				// PLLR = 2
			SET_BITS(RCC->PLLCFGR, (0b1111111<<8), (0b0010100<<8));		// PLLN = 20
			SET_BITS(RCC->PLLCFGR, (0b111<<4), (0b000<<4));				// PLLM = 1
//			SET_BITS(RCC->PLLCFGR, (0b11<<0), (0b10<<0));				// PLLSRC = 10 (HSI16)

			SET_BIT(RCC->CR, 24);
			while((RCC->CR & RCC_CR_PLLRDY) == 0)
					;
			RCC->CFGR |= 0b11; // PLL
		break;
		case CLK_16_MHZ:
			SET_BITS(RCC->CR, 0b1111<<4, 0b0110<<4); // MSI = 4 MHz
			SET_BITS(RCC->CFGR, 0b11, 0b00);
			RCC->CR &= ~(1<<24);
			while((RCC->CR & RCC_CR_PLLRDY) != 0) // wait to turn off
				;

			// output@PLLR = (input*(PLLN/PLLM))/PLLR;
			// 16 = (4*(8/1))/2;
			SET_BITS(RCC->PLLCFGR, (0b11<<25), (0b00<<25));				// PLLR = 2
			SET_BITS(RCC->PLLCFGR, (0b1111111<<8), (0b0001000<<8));		// PLLN = 8
			SET_BITS(RCC->PLLCFGR, (0b111<<4), (0b000<<4));				// PLLM = 1
//			SET_BITS(RCC->PLLCFGR, (0b11<<0), (0b10<<0));				// PLLSRC = 10 (HSI16)

			SET_BIT(RCC->CR, 24);
			while((RCC->CR & RCC_CR_PLLRDY) == 0)
				;
			RCC->CFGR |= 0b11; // PLL
		break;
		case CLK_40_MHZ:
			SET_BITS(RCC->CR, 0b1111<<4, 0b0110<<4); // MSI = 4 MHz
			SET_BITS(RCC->CFGR, 0b11, 0b00);
			while(((RCC->CFGR & 0b1100) >> 2) != 0b00)
				;
			RCC->CR &= ~(1<<24);
			while((RCC->CR & RCC_CR_PLLRDY) != 0) // wait to turn off
				;

			// output@PLLR = (input*(PLLN/PLLM))/PLLR;
			// 40 = (4*(20/1))/2;
			SET_BITS(RCC->PLLCFGR, (0b11<<25), (0b01<<25));				// PLLR = 2
			SET_BITS(RCC->PLLCFGR, (0b1111111<<8), (0b0010100<<8));		// PLLN = 20
			SET_BITS(RCC->PLLCFGR, (0b111<<4), (0b000<<4));				// PLLM = 1
//			SET_BITS(RCC->PLLCFGR, (0b11<<0), (0b10<<0));				// PLLSRC = 10 (HSI16)

			//SET_BITS(FLASH->ACR, 0b111, 0b010);

			SET_BIT(RCC->CR, 24);
			while((RCC->CR & RCC_CR_PLLRDY) == 0)
					;
			RCC->CFGR |= 0b11; // PLL
		break;
	}
}

int user_button()
{
	if((user_button_pressed == 0) && ((GPIOC->IDR & (1<<13)) >> 13 == 0b0))
	{
		user_button_pressed = 1;
		busy_sleep(10); // debounce
		return 1;
	}
	else
	{
		if(((GPIOC->IDR & (1<<13)) >> 13 == 0b1))
		{
			user_button_pressed = 0;
		}
		busy_sleep(10); // debounce
		return 0;
	}

}

int main()
{
	GPIO_init();
	SystemClockInit();

	while(1)
	{

		GPIOA->BSRR = (1<<5);
		for(int i = 0; i < 100; i++)
		{
			if(user_button())
			{
				SystemClock_Config();
			}
			busy_sleep(100);
		}
		GPIOA->BRR = (1<<5);
		for(int i = 0; i < 100; i++)
		{
			if(user_button())
			{
				SystemClock_Config();
			}
			busy_sleep(100);
		}
	}

	return 0;
}
