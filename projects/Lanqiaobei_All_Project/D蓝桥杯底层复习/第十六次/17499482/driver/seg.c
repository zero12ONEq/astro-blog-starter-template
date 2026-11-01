#include "seg.h"
code unsigned char seg_dula[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff};

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