#include "seg.h"
//0~9 Ãð
pdata unsigned char seg_dula[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xff};

void Seg_Disp(unsigned char wela,unsigned char dula,unsigned char point)
{
	unsigned char temp;
	
	P0 = 0xff;
	
	/*
	P2 =(P2&0x1f)|0xe0;
	P2 &= 0x1f;
	*/
	temp = P2 & 0x1f;
	temp = temp | 0xe0;
	P2 = temp;
	temp = P2 & 0x1f;
	P2 = temp;
	
	P0 = 0x01 << wela;
	
	/*
	P2 =(P2&0x1f)|0xc0;
	P2 &= 0x1f;
	*/
	temp = P2 & 0x1f;
	temp = temp | 0xc0;
	P2 = temp;
	temp = P2 & 0x1f;
	P2 = temp;
	
	P0 = seg_dula[dula];
	
	if(point) P0 &= 0x7f;
	
	/*
	P2 =(P2&0x1f)|0xe0;
	P2 &= 0x1f;
	*/
	temp = P2 & 0x1f;
	temp = temp | 0xe0;
	P2 = temp;
	temp = P2 & 0x1f;
	P2 = temp;
}
	