#include "bsp_key.h"

unsigned char key_read()
{
	unsigned char key_val;
	unsigned int key_num;
	
	P44=0;P42=1;P35=1;
	key_num=P3&0X0F;
	
	P44=1;P42=0;P35=1;
	key_num=((key_num<<4)|(P3&0X0F));
	
	P44=1;P42=1;P35=0;
	key_num=((key_num<<4)|(P3&0X0F));
	
	P44=1;P42=1;P35=1;
	key_num=((key_num<<4)|(P3&0X0F));
	
	switch(~key_num)
	{
		case 0x8000:key_val=4;break;
		case 0x4000:key_val=5;break;
		case 0x2000:key_val=6;break;
		case 0x1000:key_val=7;break;
		
		case 0x0800:key_val=8;break;
		case 0x0400:key_val=9;break;
		case 0x0c00:key_val=20;break;
		case 0x0200:key_val=10;break;
		case 0x0100:key_val=11;break;
		
		case 0x0080:key_val=12;break;
		case 0x0040:key_val=13;break;
		case 0x0020:key_val=14;break;
		case 0x0010:key_val=15;break;
		
		default:key_val=0;break;
		
	}
	return key_val;
}

