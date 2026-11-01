#include "Led.h"

void Led_Display(unsigned char* ucled, unsigned char enable)
{
	unsigned char temp = 0x00;
	static unsigned char temp_Old = 0xFF;

	temp = (ucled[0] << 0) | (ucled[1] << 1) | (ucled[2] << 2) | (ucled[3] << 3) |
		   (ucled[4] << 4) | (ucled[5] << 5) | (ucled[6] << 6) | (ucled[7] << 7);
	
	if(!enable) temp = 0x00;

	if(temp != temp_Old)
	{
		P0 = ~temp;
		P2 = P2 & 0x1F | 0x80;
		P2 &= 0x1F;
		temp_Old = temp;
	}
}