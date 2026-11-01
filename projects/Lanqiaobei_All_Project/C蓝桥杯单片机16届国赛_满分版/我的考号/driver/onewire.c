/*	# 	单总线代码片段说明
	1. 	本文件夹中提供的驱动代码供参赛选手完成程序设计参考。
	2. 	参赛选手可以自行编写相关代码或以该代码为基础，根据所选单片机类型、运行速度和试题
		中对单片机时钟频率的要求，进行代码调试和修改。
*/
#include "onewire.h"
sbit DQ=P1^4;
//
void Delay_OneWire(unsigned int t)  
{
	unsigned char i;
	while(t--){
		for(i=0;i<12;i++);
	}
}

//
void Write_DS18B20(unsigned char dat)
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		DQ = 0;
		DQ = dat&0x01;
		Delay_OneWire(5);
		DQ = 1;
		dat >>= 1;
	}
	Delay_OneWire(5);
}

//
unsigned char Read_DS18B20(void)
{
	unsigned char i;
	unsigned char dat;
  
	for(i=0;i<8;i++)
	{
		DQ = 0;
		dat >>= 1;
		DQ = 1;
		if(DQ)
		{
			dat |= 0x80;
		}	    
		Delay_OneWire(5);
	}
	return dat;
}

//
bit init_ds18b20(void)
{
  	bit initflag = 0;
  	
  	DQ = 1;
  	Delay_OneWire(12);
  	DQ = 0;
  	Delay_OneWire(80);
  	DQ = 1;
  	Delay_OneWire(10); 
    initflag = DQ;     
  	Delay_OneWire(5);
  
  	return initflag;
}

float rd_temperature()
{
		unsigned char low, high;
		unsigned int raw_u;
		int raw;

		init_ds18b20();
		Write_DS18B20(0xcc);
		Write_DS18B20(0x44);
		Delay_OneWire(200);

		init_ds18b20();
		Write_DS18B20(0xcc);
		Write_DS18B20(0xbe);

		low = Read_DS18B20();
		high = Read_DS18B20();

		raw_u = ((unsigned int)high << 8) | low; /* 将 DS18B20 读出的高低字节拼成 16 位原始温度数据 */

		if(raw_u & 0x8000) /* DS18B20 温度数据是 16 位有符号补码，最高位为 1 表示负温度 */
				raw = (int)(raw_u - 65536UL); /* 负温度：把无符号原始值转换成真正的有符号数 */
		else
				raw = (int)raw_u;/* 正温度：原始值可直接作为温度计数值 */

		return raw * 0.0625f;
}
