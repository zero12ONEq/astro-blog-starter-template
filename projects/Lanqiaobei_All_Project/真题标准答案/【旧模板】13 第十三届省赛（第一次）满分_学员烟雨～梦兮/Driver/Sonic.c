#include "Sonic.h"

void Delay12us(void)	//@12.000MHz
{
	unsigned char i;

	_nop_();
	_nop_();
	i = 38; // 特别注意
	while (--i);
}


void Sonic_Init(void) //生成 8 个 40KHZ 的超声波信号
{
	unsigned char i;
  EA = 0;
	for(i = 0; i < 8; i++)
	{
		TX = 1;
		Delay12us();
		TX = 0;
		Delay12us();
	}
  EA = 1;
}

unsigned char Sonic_Read(void)
{
	unsigned int time; // 时间储存变量
	CMOD = 0x00;
	CH = CL = 0;	// 复位计数值 等待超声波信号发出
	CR = 1;			// 开始计时
	Sonic_Init(); // 发送超声波信号
	while ((RX == 1) && (CF == 0));		 // 等待接受返回信号或者定时器溢出
	CR = 0;		 // 停止计时
	if (CF == 0) // 定时器没有溢出
	{
		time = CH << 8 | CL;   // 读取当前时间
		return (time * 0.017); // 返回距离值
	}
	else
	{
		CF = 0; // 清除溢出标志位
		return 0;
	}
}
