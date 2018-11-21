#include "stm32l476xx.h"

// PB3~PB5 - MAX7219

// PA6~PA9 - horizontal
// PC5~PC8 - vertical

int map[4][4] = {
		{1, 2, 3, 99},
		{4, 5, 6, 11},
		{7, 8, 9, 12},
		{15, 0, 14, 13}
};
int cur_active_led = 0;

void GPIO_init(void)
{
	// Enable AHB2 clock
	RCC->AHB2ENR = 0b00000000000000000000000000000110; // A, B, C


	// PC0~PC3 - horizontal - input
	// PC5~PC8 - vertical - output
	GPIOC->MODER =		0b11111111111111010101011100000000;
	GPIOC->OSPEEDR = 	0b00000000000000101010100010101010;
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
		max7219_send(i+1, 15); // clear
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

		for(int i = nsymb; i > 0; i--)
		{
			max7219_send(i, stack[i-1]);
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

// PC0~PC3 - horizontal - input
// PC5~PC8 - vertical - output
void keypad()
{
	for(int i = 0; i < 4; i++)
		GPIOC->BRR = 0b1 << (5+i);
	for(int col = 0; col < 4; col++)
	{
		GPIOC->BSRR = 0b1 << (5+col);
		for(int row = 0; row < 4; row++)
		{
			int found = (GPIOC->IDR >> (0+row)) & 0b1;
			if(found == 0b1)
			{
				screen(map[row][col]);
				return;
			}
		}
		GPIOC->BRR = 0b1 << (5+col);
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

int main(void)
{
	GPIO_init();
	//max7219_init();

	max7219_send(0b1100, 0b00000001); // shutdown register | normal operation
	max7219_send(0b1111, 0b00000000); // test | no
	max7219_send(0b1011, 0b00000111); // scan limit | 1-7
	max7219_send(0b1001, 0b11111111); // decode | 1-7

	clear_screen();

	while(1)
	{
		keypad();

		busy_sleep(10000);
	}

	return 0;
}
