/*	# 	DS1302代码片段说明
	1. 	本文件夹中提供的驱动代码供参赛选手完成程序设计参考。
	2. 	参赛选手可以自行编写相关代码或以该代码为基础，根据所选单片机类型、运行速度和试题
		中对单片机时钟频率的要求，进行代码调试和修改。
*/								
#include "ds1302.h"
#include "intrins.h"
sbit SCK = P1^7;
sbit SDA = P2^3;
sbit RST = P1^3;
//
void Write_Ds1302(unsigned  char temp) 
{
	unsigned char i;
	for (i=0;i<8;i++)     	
	{ 
		SCK = 0;
		SDA = temp&0x01;
		temp>>=1; 
		SCK=1;
	}
}   

//
void Write_Ds1302_Byte( unsigned char address,unsigned char dat )     
{
 	RST=0;	_nop_();
 	SCK=0;	_nop_();
 	RST=1; 	_nop_();  
 	Write_Ds1302(address);	
 	Write_Ds1302(dat);		
 	RST=0; 
}

//
unsigned char Read_Ds1302_Byte ( unsigned char address )
{
 	unsigned char i,temp=0x00;
 	RST=0;	_nop_();
 	SCK=0;	_nop_();
 	RST=1;	_nop_();
 	Write_Ds1302(address);
 	for (i=0;i<8;i++) 	
 	{		
		SCK=0;
		temp>>=1;	
 		if(SDA)
 		temp|=0x80;	
 		SCK=1;
	} 
 	RST=0;	_nop_();
 	SCK=0;	_nop_();
	SCK=1;	_nop_();
	SDA=0;	_nop_();
	SDA=1;	_nop_();
	return (temp);			
}

//写入时间参数函数 
//无返回值 传入参数为要写入DS1302的内容
void Set_Rtc(unsigned char*ucRtc)
{
	unsigned char i;
	Write_Ds1302_Byte(0x8e,0x00);//WP位 置为0，关闭写保护
	Write_Ds1302_Byte(0x80,0x80);//CH位 置为1，关闭时钟振荡器，停止计时
	for(i=0;i<3;++i)
		Write_Ds1302_Byte(0x84-2*i,(ucRtc[i]/10%10)*16+ucRtc[i]%10);//把时，分，秒依次写入，十进制转化为十六进制BCD码
	Write_Ds1302_Byte(0x8e,0x80);//WP位 置为1，开启写保护
	
}
//读取时间参数函数
//无返回值 传入参数为要读取到的DS1302的内容，用一个数组来接收
void Read_Rtc(unsigned char*ucRtc)
{
	unsigned char i;
	unsigned char temp;
	EA=0;//关闭全局中断，防止影响时序
	for(i=0;i<3;++i)
	{
		temp=Read_Ds1302_Byte(0x85-2*i);
		ucRtc[i]=temp/16*10+temp%16;//十六进制BCD码转十进制
	}
	EA=1;//开启全局中断
}