/* 头文件声明区域 */
#include <STC15F2K60S2.H>
#include "Led.h"
#include "Init.h"
#include "Key.h"
#include "Seg.h"
#include "Ds18b20.h"
#include "I2C.h"

/* 变量声明区域 */
unsigned char key_Val, key_Down, key_Up, key_Old, key_Scan_Slow;
unsigned char seg_Scan_Slow, seg_Buf[] = {10, 10, 10, 10, 10, 10, 10, 10}, points[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned char led_Buf[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned char display_Pos;
unsigned char seg_display_mode = 0; // 0-温度显示，1-参数设置，2-DAC输出
float display_t; // 实际显示温度
unsigned char display_set_t = 25, set_t; // 温度参数
float dac_voltage; // DAC 输出电压
bit sys_mode;// 0-DAC输出电压与温度相关，1-DAC 按照图示关系输出电压
unsigned char timer200ms;

/* 按键处理函数 */
void Key_Proc()
{

	if(key_Scan_Slow) return;
	key_Scan_Slow = 1;
	
	key_Val = Key_Read();
	key_Down = key_Val & (key_Val ^ key_Old);
	key_Up = ~key_Val & (key_Val ^ key_Old);
	key_Old = key_Val;
	
	switch(key_Down)
	{
		case 4: // 界面切换
			if(seg_display_mode == 0) set_t = display_set_t; // 进入
			if(seg_display_mode == 1) display_set_t = set_t; // 退出
			if(++seg_display_mode == 3) seg_display_mode = 0;
			break;
		case 8: // 参数减 1
			if(seg_display_mode == 1 && --set_t == 255) set_t = 0; 
			break;
		case 9: // 参数加 1
			if(seg_display_mode == 1 && ++set_t == 100) set_t = 99; 
			break;
		case 5: // 模式切换
			sys_mode ^= 1;
			break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	if(seg_Scan_Slow) return;
	seg_Scan_Slow = 1;
	
	/* 信息读取区域 */
	display_t = Read_Temperature();
	
	/* 数据处理区域 */
	switch(seg_display_mode)
	{
		case 0: // 0-温度显示
			seg_Buf[0] = 11;
			seg_Buf[1] = seg_Buf[2] = seg_Buf[3] = 10;
			seg_Buf[4] = (unsigned char)display_t / 10 % 10;
			seg_Buf[5] = (unsigned char)display_t / 1 % 10;
			seg_Buf[6] = (unsigned int)(display_t * 10) % 10;
			seg_Buf[7] = (unsigned int)(display_t * 100) % 10;
			points[5] = 1;
			break;
		case 1: // 1-参数设置
			seg_Buf[0] = 12;
			seg_Buf[1] = seg_Buf[2] = seg_Buf[3] = seg_Buf[4] = seg_Buf[5]= 10;
			seg_Buf[6] = set_t / 10 % 10;
			seg_Buf[7] = set_t % 10;
			points[5] = 0;
			break;
		case 2: // 2-DAC输出
			
			seg_Buf[0] = 13;
			seg_Buf[1] = seg_Buf[2] = seg_Buf[3] = seg_Buf[4] = 10;
			seg_Buf[5] = (unsigned char)dac_voltage / 1 % 10;
			seg_Buf[6] = (unsigned int)(dac_voltage * 10) % 10;
			seg_Buf[7] = (unsigned int)(dac_voltage * 100) % 10;
			points[5] = 1;
			break;
	}
}

/* 其他处理函数 */
void Led_Proc()
{
	unsigned char i;
	led_Buf[0] = ~sys_mode;
	for(i = 1; i <= 3; i++)
		led_Buf[i] = (i == seg_display_mode + 1);
  
  if(timer200ms) return;
	
	if(sys_mode == 0) // 模式 1
    dac_voltage = display_t < display_set_t ? 0 : 5;
  else
  {
    if(display_t <= 20)
      dac_voltage = 1;
    else if(display_t >= 40)
      dac_voltage = 4;
    else
      dac_voltage = display_t * 0.15 - 2;
  }
  DA_Output(dac_voltage);
}

/* 定时器 0 初始化函数 */
void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TL0 = 0x18;				//设置定时初始值
	TH0 = 0xFC;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	EA = 1;
	ET0 = 1;
}

/* 定时器 0 中断处理函数 */
void Timer0_Server() interrupt 1
{
	if(++key_Scan_Slow == 10) key_Scan_Slow = 0;
	if(++seg_Scan_Slow == 200) seg_Scan_Slow = 0;
  
  if(++timer200ms == 200)
    timer200ms = 0;
	
	if(++display_Pos == 8) display_Pos = 0;
	Seg_Display(display_Pos, seg_Buf[display_Pos], points[display_Pos]);
	Led_Display(display_Pos, led_Buf[display_Pos]);
}

/* Main */
void main(void)
{
	/* 初始化区域 */
	while(Read_Temperature() == 85);
	Timer0_Init();
	System_Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}