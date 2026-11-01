#include "Ds1302.h"

void Write_Ds1302(unsigned char temp) 
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


void Write_Ds1302_Byte(unsigned char address, unsigned char dat)     
{
 	RST=0;	_nop_();
 	SCK=0;	_nop_();
 	RST=1; 	_nop_();  
 	Write_Ds1302(address);	
 	Write_Ds1302(dat);		
 	RST=0; 
}


unsigned char Read_Ds1302_Byte (unsigned char address)
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

void Set_Rtc(unsigned char* ucTime)
{
	Write_Ds1302_Byte(0x8E, 0x00); // 关闭写保护
	Write_Ds1302_Byte(0x80, 0x80); // 停止计时（秒寄存器的第7位就是启动/停止位）
	Write_Ds1302_Byte(0x84, (ucTime[0] / 10) * 16 + ucTime[0] % 10);
	Write_Ds1302_Byte(0x82, (ucTime[1] / 10) * 16 + ucTime[1] % 10);
	Write_Ds1302_Byte(0x80, (ucTime[2] / 10) * 16 + ucTime[2] % 10);
	Write_Ds1302_Byte(0x8E, 0x80);
}

void Read_Rtc(unsigned char* ucTime) // 读取系统时间，并将其转换成十进制
{
	unsigned char temp = 0;
	EA=0; //关闭总中断
	temp = Read_Ds1302_Byte(0x85);
	ucTime[0] = (temp / 16) * 10 + temp % 16;
	temp = Read_Ds1302_Byte(0x83);
	ucTime[1] = (temp / 16) * 10 + temp % 16;
	temp = Read_Ds1302_Byte(0x81);
	ucTime[2] = (temp / 16) * 10 + temp % 16;
	EA=1; //打开总中断
}
