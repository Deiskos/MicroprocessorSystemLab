#include "stm32l476xx.h"

int student_id[7] = {0, 5, 1, 6, 1, 1, 1};

int main(void)
{
	GPIO_init();
	max7219_init();
	int *ptr = &(student_id[6]);
	for(int i = 0; i < 7; i++)
	{
		max7219_send(i+1, *ptr);
		ptr--;
	}
	while(1)
		;
	return 0;
}
