/* 头文件声明区域 */
#include <STC15F2K60S2.H>
#include "Led.h"
#include "Init.h"
#include "Key.h"
#include "Seg.h"
#include "Ds1302.h"
#include "Ds18b20.h"
#include "I2C.h"
#include "Sonic.h"

/* 变量声明区域 */
data unsigned char key_Val, key_Down, key_Up, key_Old, key_Scan_Slow;
data unsigned char seg_Scan_Slow, seg_Pos, seg_Buf[] = {10, 10, 10, 10, 10, 10, 10, 10};
data unsigned char led_Buf[] = {0, 0, 0, 0, 0, 0, 0, 0};

unsigned char seg_display_mode; // 0-电压界面，1-测距界面，2-参数界面，
unsigned char sonic;
bit sonic_mode;// 0-未启动，1-连续测量
float voltage;
unsigned char par_max = 45, par_min = 5, set_par_max, set_par_min;
bit par_index;
unsigned char* par_data[2] = {&set_par_max, &set_par_min};
unsigned char timer100ms;
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
      if(seg_display_mode == 1) // 准备切换至参数界面时
      {
        set_par_max = par_max;
        set_par_min = par_min;
        par_index = 0;
      }
      else if(seg_display_mode == 2 && set_par_max >= set_par_min) // 准备从参数界面腿粗时
      {
        par_max = set_par_max;
        par_min = set_par_min;
      }
      if(++seg_display_mode == 3) seg_display_mode = 0;
      break;
    case 5: // 参数选择
      if(seg_display_mode == 2)
        par_index ^= 1;
      break;
    case 6: // 参数 +
      if(seg_display_mode == 2)
      {
        *(par_data[par_index]) += 5;
        if(*(par_data[par_index]) == 55) *(par_data[par_index]) = 5;
      }
      break;
    case 7: // 参数 -
      if(seg_display_mode == 2)
      {
        *(par_data[par_index]) -= 5;
        if(*(par_data[par_index]) == 0) *(par_data[par_index]) = 50;
      }
      break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
  unsigned char i = 5;
	if(seg_Scan_Slow) return;
	seg_Scan_Slow = 1;
	
	/* 信息读取区域 */
  voltage = AD_Read(3);
  sonic_mode = ((par_min / 10.0) < voltage && voltage < (par_max / 10.0));
  if(sonic_mode)
    sonic = Sonic_Read();
  
	/* 数据处理区域 */
  switch(seg_display_mode)
  {
    case 0: // 0-电压界面
      seg_Buf[0] = 11;
      seg_Buf[3] = seg_Buf[4] = 10;
      seg_Buf[5] = (unsigned char)voltage % 10 + '.';
      seg_Buf[6] = (unsigned int)(voltage * 10) % 10;
      seg_Buf[7] = (unsigned int)(voltage * 100) % 10;
      break;
    case 1: // 1-测距界面
      seg_Buf[0] = 13;
      seg_Buf[3] = seg_Buf[4] = 10;
      seg_Buf[5] = sonic_mode ? sonic / 100 % 10 : 14;
      seg_Buf[6] = sonic_mode ? sonic / 10 % 10 : 14;
      seg_Buf[7] = sonic_mode ? sonic % 10 : 14;
      
      while(sonic_mode && seg_Buf[i] == 0)
      {
        seg_Buf[i] = 10;
        if(++i == 7) break;
      }
      break;
    case 2: // 2-参数界面
      seg_Buf[0] = 12;
      seg_Buf[5] = 10;
      seg_Buf[3] = set_par_max / 10 % 10 + '.';
      seg_Buf[4] = set_par_max % 10;
      seg_Buf[6] = set_par_min / 10 % 10 + '.';
      seg_Buf[7] = set_par_min % 10;
      break;
  }
}

/* 其他处理函数 */
void Led_Proc()
{
  unsigned char i = 0;
  for(i = 0; i < 3; i++)
    led_Buf[i] = (i == seg_display_mode);
  
  if(timer200ms == 0)
  {
    if(sonic_mode)
    {
      if(sonic >= 80)
        DA_Output(5);
      else if(sonic <= 20)
        DA_Output(1);
      else
        DA_Output((sonic - 5.0) / 15.0);
    }
    else
      DA_Output(0);
  }
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
	
  if(sonic_mode && ++timer100ms == 100)
  {
    timer100ms = 0;
    led_Buf[7] ^= 1;
  }
  else if(sonic_mode == 0)
    led_Buf[7] = 0;
  
  if(++timer200ms == 200)
  {
    timer200ms = 0;
  }
  
	if(++seg_Pos == 8) seg_Pos = 0;
	if(seg_Buf[seg_Pos] > 20) // 判断有无小数点，ASCII 为 46
		Seg_Display(seg_Pos, seg_Buf[seg_Pos] - '.', 1);
	else
		Seg_Display(seg_Pos, seg_Buf[seg_Pos], 0);
	
	Led_Display(led_Buf, 1);
}

/* Main */
void main(void)
{
	/* 初始化区域 */
	System_Init();
  AD_Read(3);
	Timer1_Init();
	
	/* 主循环 */
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}