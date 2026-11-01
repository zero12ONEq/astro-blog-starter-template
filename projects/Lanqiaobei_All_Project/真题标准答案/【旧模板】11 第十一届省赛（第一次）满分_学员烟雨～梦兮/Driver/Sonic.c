#include "Sonic.h"

void Delay12us(void)	//@12.000MHz
{
	unsigned char data i;

	_nop_();
	_nop_();
	i = 33;
	while (--i);
}


void Sonic_Init(void) //生成 8 个 40KHZ 的超声波信号
{
	unsigned char i;
	for(i = 0; i < 8; i++)
	{
		TX = 1;
		Delay12us();
		TX = 0;
		Delay12us();
	}
}

unsigned char Sonic_Read(void)
{
	unsigned int time;
	CMOD = 0x00; // 配置 PCA 工作模式
	CH = CL = 0;
	Sonic_Init();
	CR = 1; // 开始计时
	while((RX == 1) && (CF == 0));
	CR = 0; // 停止计时
	if(CF == 0) // 没有溢出
	{
		time = CH << 8 | CL;
		return (time * 0.017);
	}
	else
	{
		CF = 0;
		return 0;
	}
}
