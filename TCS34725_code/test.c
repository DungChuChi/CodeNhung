// Nguyễn Mạnh Dũng - 22146287
// Nguyễn Lê Duy - 22146285
#include <stdio.h>
#include <stdlib.h>
#include "tcs34725_library.h"

int r,g,b,c;
int main()
{
	turn_On_Sensor();
	Init_Enable(0x1B);
	Init_Atime(atime_24ms);
	Init_Control(gain_x4);
	printf("Turn on success a\n");
	while (1)
	{
		c = Read_CLEAR_data();
		r = Read_RED_data();
		g = Read_GREEN_data();
		b = Read_BLUE_data();
		printf("c: %d, r: %d, g: %d, b: %d\n", c,r,g,b); 
		
	}
	return 0;
	
}
