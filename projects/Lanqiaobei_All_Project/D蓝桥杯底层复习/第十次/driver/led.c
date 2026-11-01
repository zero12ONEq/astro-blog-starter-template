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

void led_off()
{
	unsigned char temp;
	P0=0xff;
	temp=P2&0x1f;
	temp=temp|0x80;
	P2=temp;
	temp=P2&0x1f;
	P2=temp;	
	
	temp_1_old=0x00;
}

idata unsigned char temp_2=0x00;
idata unsigned char temp_2_old=0xff;

void beep(bit enable)
{
	unsigned char temp;
	if(enable)
		temp_2|=0x40;
	else
		temp_2&=(~0x40);
	if(temp_2!=temp_2_old)
	{
		P0=temp_2;
		temp=P2&0x1f;
		temp=temp|0xa0;
		P2=temp;
		temp=P2&0x1f;
		P2=temp;	

		temp_2_old=temp_2;
	}
}

void motor(bit enable)
{
	unsigned char temp;
	if(enable)
		temp_2|=0x20;
	else
		temp_2&=(~0x20);
	if(temp_2!=temp_2_old)
	{
		P0=temp_2;
		temp=P2&0x1f;
		temp=temp|0xa0;
		P2=temp;
		temp=P2&0x1f;
		P2=temp;	

		temp_2_old=temp_2;
	}
}

void relay(bit enable)
{
	unsigned char temp;
	if(enable)
		temp_2|=0x10;
	else
		temp_2&=(~0x10);
	if(temp_2!=temp_2_old)
	{
		P0=temp_2;
		temp=P2&0x1f;
		temp=temp|0xa0;
		P2=temp;
		temp=P2&0x1f;
		P2=temp;

		temp_2_old=temp_2;
	}
}