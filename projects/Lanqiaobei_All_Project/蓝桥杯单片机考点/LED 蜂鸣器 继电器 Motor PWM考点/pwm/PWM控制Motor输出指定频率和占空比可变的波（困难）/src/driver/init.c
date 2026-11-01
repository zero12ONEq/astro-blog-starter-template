#include "init.h"
void System_Init()
{
	unsigned char temp;
	//关闭LED
	P0=0xff;

	/*
		P2 = (P2&0x1f) | 0x80;
		P2 &= 0x1f;
	*/
	temp = P2 & 0x1f;
	temp = temp | 0x80;
	P2 = temp;
	temp = P2 & 0x1f;
	P2 = temp;
	
	//关闭蜂鸣器，继电器，电机
	P0=0x00;
	
	/*
		P2 = (P2&0x1f) | 0xa0;
		P2 &= 0x1f;
	*/
	temp = P2 & 0x1f;
	temp = temp | 0xa0;
	P2 = temp;
	temp = P2 & 0x1f;
	P2 = temp;
	
}