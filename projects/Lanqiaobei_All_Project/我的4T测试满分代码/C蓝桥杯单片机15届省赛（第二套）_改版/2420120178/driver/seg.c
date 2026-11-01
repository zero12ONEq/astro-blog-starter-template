#include "seg.h"
//11-C-0xC6 12-A-0x88 13-P-0X8C 14-'-'-0xbf
code unsigned char seg_dula[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff,
																0XC6,0X88,0X8C,0XBF};

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
