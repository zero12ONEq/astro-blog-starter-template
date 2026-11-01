#include "Init.h"

void System_Init()
{
	P0 = 0xFF;
	P2 = P2 & 0x1F | 0x80;
	P2 &= 0x1F;
	 
	P0 = 0x00; // ¹Ø±Õ·äÃùÆ÷
	P2 = P2 & 0x1F | 0xA0;
	P2 &= 0x1F;
}

void Buzz(unsigned char enable) // ·äÃùÆ÷
{
	static unsigned char temp = 0x00;
	static unsigned char temp_old = 0xFF;
	
	if(enable)
		temp |= 0x40;
	else
		temp &= ~0x40;
	
	if(temp != temp_old)
	{
		P0 = temp;
		P2 = P2 & 0x1F | 0xA0;
		P2 &= 0x1F;
		temp_old = temp;
	}
}

void Relay(unsigned char enable) // ¼ÌµçÆ÷
{
	static unsigned char temp = 0x00;
	static unsigned char temp_old = 0xFF;
	
	if(enable)
		temp |= 0x10;
	else
		temp &= ~0x10;
	
	if(temp != temp_old)
	{
		P0 = temp;
		P2 = P2 & 0x1F | 0xA0;
		P2 &= 0x1F;
		temp_old = temp;
	}
}