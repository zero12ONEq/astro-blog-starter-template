#include "bsp_led.h"

unsigned char temp=0x00;
unsigned char temp_old=0xff;

void led_disp(unsigned char *ucled)
{
	unsigned char i=0;
	temp=0x00;
	
	for(i=0;i<8;i++)
	{
		temp|=ucled[i]<<i;
	}
	
	if(temp!=temp_old)
	{
		P0=~temp;
		P2=P2&0X1F|0X80;
	  P2=P2&0X1F; 		
		temp_old=temp;
	}
}

unsigned char temp1=0x00;
unsigned char temp1_old=0xff;

void relay(bit enable)
{
	if(enable)
	temp1|=0x10;
	else
	temp1&=~0x10;
	
	if(temp1!=temp1_old)
	{
		P0=temp1;
		P2=P2&0X1F|0XA0;
		P2=P2&0X1F;
		temp1_old=temp1;
	}
	
}

