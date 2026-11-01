#include "seg.h"
//11-A-0X88 12-C-0XC6 13-E-0X86 14-P-0X8C
code unsigned char seg_dula[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0xff,
																0x88,0xc6,0x86,0x8c};

void seg_disp(unsigned char wela,unsigned char dula)
{
	unsigned char temp;
	P0=0xff;
	temp=P2&0x1f;
	temp=temp|0xe0;
	P2=temp;
	temp=P2&0x1f;
	P2=temp;

	P0=0x01<<wela;
	temp=P2&0x1f;
	temp=temp|0xc0;
	P2=temp;
	temp=P2&0x1f;
	P2=temp;	
	
	P0=seg_dula[dula];
	temp=P2&0x1f;
	temp=temp|0xe0;
	P2=temp;
	temp=P2&0x1f;
	P2=temp;
}