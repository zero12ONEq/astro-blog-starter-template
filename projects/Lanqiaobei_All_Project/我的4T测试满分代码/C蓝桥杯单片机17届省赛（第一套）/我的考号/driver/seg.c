#include "seg.h"
//11-A-0X88 12-E-0X86 13-C-0XC6 14-P-0X8C 15-H-0X89 16-L-0XC7 17-'-'-0xbf
code unsigned char seg_dula[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff,
																0X88,0X86,0XC6,0X8C,0X89,0XC7,0XBF};

void seg_disp(unsigned char wela,unsigned char dula,bit point)
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
	if(point)
		P0&=0x7f;
	temp=P2&0x1f;
	temp=temp|0xe0;
	P2=temp;
	temp=P2&0x1f;
	P2=temp;	
}