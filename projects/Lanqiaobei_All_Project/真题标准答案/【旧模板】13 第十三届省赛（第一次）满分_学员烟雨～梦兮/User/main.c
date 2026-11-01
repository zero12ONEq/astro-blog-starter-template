/* ===================================头文件声明==================================== */
#include <STC15F2K60S2.H>
#include "Led.h"
#include "Init.h"
#include "Key.h"
#include "Seg.h"
#include "Ds1302.h"
#include "Ds18b20.h"

/* ====================================变量定义==================================== */
/* 核心变量 */
data unsigned char key_Val, key_Down, key_Up, key_Old, key_Scan_Slow;
data unsigned char seg_Scan_Slow, seg_Pos, seg_Buf[] = {10, 10, 10, 10, 10, 10, 10, 10};
data unsigned char led_Buf[] = {0, 0, 0, 0, 0, 0, 0, 0};

/* 状态标志 */
unsigned char seg_display_mode = 0; // 0-温度显示，1-时间显示，2-参数设置
bit work_mode = 0; // 0-温度控制，1-时间控制
bit work0_relay_flag;
bit work1_relay_flag;

/* 相关变量 */
float temperature; // 温度值
unsigned char temperature_par = 23; // 温度参数
unsigned char rtc_time[3] = {12, 59, 55};

/* 计时变量 */
unsigned char timer100ms;
unsigned int relay_timer5000ms;
unsigned int led_timer5000ms;

/* ====================================任务函数==================================== */
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
    case 12: // 界面切换
      if(++seg_display_mode == 3) seg_display_mode = 0;
      break;
    case 13: // 工作模式切换
      work_mode ^= 1;
      break;
    case 16: // 参数 +
      if(seg_display_mode == 2 && ++temperature_par == 100) temperature_par = 99;
      break;
    case 17: // 参数-，长按切换界面
      if(seg_display_mode == 2 && --temperature_par == 9) temperature_par = 10;
      break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	/* 信息读取处理区域 */
  switch(seg_Scan_Slow)
  {
    case 30:
      temperature = Read_Temperature();
      break;
    case 80:
      Read_Rtc(rtc_time);
      break;
  }
  
	/* 数据显示区域 */
  if(seg_Scan_Slow) return;
	seg_Scan_Slow = 1;
  
  seg_Buf[0] = 12; // U
  seg_Buf[1] = seg_display_mode + 1;
  switch(seg_display_mode)
  {
    case 0: // 温度显示
      seg_Buf[3] = seg_Buf[4] = 10;
      seg_Buf[5] = temperature >= 10 ? (unsigned char)temperature / 10 % 10 : 10;
      seg_Buf[6] = (unsigned char)temperature % 10 + '.';
      seg_Buf[7] = (unsigned int)(temperature * 10) % 10;
      break;
    case 1: // 时间显示
      seg_Buf[3] = key_Old == 17 ? rtc_time[1] / 10 % 10 : rtc_time[0] / 10 % 10;
      seg_Buf[4] = key_Old == 17 ? rtc_time[1] % 10 : rtc_time[0] % 10;
    
      seg_Buf[5] = 11; // -
    
      seg_Buf[6] = key_Old == 17 ? rtc_time[2] / 10 % 10 : rtc_time[1] / 10 % 10;
      seg_Buf[7] = key_Old == 17 ? rtc_time[2] % 10 : rtc_time[1] % 10;
      break;
    case 2: // 参数设置
      seg_Buf[3] = seg_Buf[4] = seg_Buf[5] = 10;
      seg_Buf[6] = temperature_par >= 10 ? temperature_par / 10 % 10 : 10;
      seg_Buf[7] = temperature_par % 10;
      break;
  }
}

/* 其他处理函数 */
void Led_Proc()
{
	if(work_mode == 0) // 温度控制
  {
    work0_relay_flag = temperature > temperature_par;
    Relay(work0_relay_flag);
  }
  else // 时间控制
  {
    if(rtc_time[1] == 00 && rtc_time[2] == 00)
    {
      work1_relay_flag = 1;
    }
    
    Relay(work1_relay_flag);
  }
  
  if(rtc_time[1] == 00 && rtc_time[2] == 00)
    led_Buf[0] = 1;
  
  led_Buf[1] = !work_mode;
}

/* 定时器 1 初始化函数 */
void Timer1_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	EA = 1;
	ET1 = 1;
}

/* 定时器 1 中断处理函数 */
void Timer1_Server() interrupt 3
{
	if(++key_Scan_Slow == 10) key_Scan_Slow = 0;
	if(++seg_Scan_Slow == 100) seg_Scan_Slow = 0;
  
  if(work1_relay_flag == 1 && ++relay_timer5000ms == 5000)
  {
    relay_timer5000ms = 0;
    work1_relay_flag = 0;
  }
  
  if(led_Buf[0] == 1 && ++led_timer5000ms == 5000)
  {
    led_timer5000ms = 0;
    led_Buf[0] = 0;
  }
  
  if((work_mode == 0 && work0_relay_flag) || (work_mode == 1 && work1_relay_flag))
  {
    if(++timer100ms == 100)
    {
      timer100ms = 0;
      led_Buf[2] ^= 1;
    }
  }
  else
    led_Buf[2] = timer100ms = 0;
	
	if(++seg_Pos == 8) seg_Pos = 0;
	if(seg_Buf[seg_Pos] > 20) // 判断有无小数点，ASCII 为 46
		Seg_Display(seg_Pos, seg_Buf[seg_Pos] - '.', 1);
	else
		Seg_Display(seg_Pos, seg_Buf[seg_Pos], 0);
	
	Led_Display(led_Buf, 1);
}

/* ====================================主循环函数=================================== */
void main(void)
{
  
	/* 初始化区域 */
	System_Init();
  Set_Rtc(rtc_time);
  while(Read_Temperature() == 85);
	Timer1_Init();
  
	/* 主循环 */
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}
