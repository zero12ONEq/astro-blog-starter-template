#include "seg.h"
code unsigned char Seg_Dula[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0x88,0x83,0xc6,0xa1,0x86,0x8e,0xff};//段码

//数码管显示处理函数
//无返回值 传入参数 wela：位码值 0-7表示1-8个数码管点亮 duta：段码值 point：1-显示小数点 0-不显示小数点
void Seg_Disp(unsigned char wela,unsigned char dula,bit point)
{
	unsigned char temp;
	//#1、消影
	P0=0xff;
	//开门
	temp=P2&0x1f;
	temp=temp|0xe0;
	P2=temp;
	//关门
	temp=P2&0x1f;
	P2=temp;
	
	//#2、位选
	P0=0x01<<wela;
	//开门
	temp=P2&0x1f;
	temp=temp|0xc0;
	P2=temp;
	//关门
	temp=P2&0x1f;
	P2=temp;
	
	//#3、段选
	P0=Seg_Dula[dula];
	if(point)
		P0&=0x7f;//点亮小数点
	//开门
	temp=P2&0x1f;
	temp=temp|0xe0;
	P2=temp;
	//关门
	temp=P2&0x1f;
	P2=temp;
	

}