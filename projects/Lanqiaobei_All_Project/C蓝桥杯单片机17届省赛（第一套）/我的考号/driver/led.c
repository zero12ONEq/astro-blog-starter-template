#include "led.h"
idata unsigned char temp_1=0x00;
idata unsigned char temp_1_old=0xff;

void led_disp(unsigned char*ucled)
{
	unsigned char temp;
	temp_1=0x00;
	temp_1=(ucled[0]<<0)|(ucled[1]<<1)|(ucled[2]<<2)|(ucled[3]<<3)|
					(ucled[4]<<4)|(ucled[5]<<5)|(ucled[6]<<6)|(ucled[7]<<7);
	if(temp_1!=temp_1_old)
	{
		P0=~temp_1;
		temp=P2&0x1f;
		temp=temp|0x80;
		P2=temp;
		temp=P2&0x1f;
		P2=temp;		
		
		temp_1_old=temp_1;
	}
}


	