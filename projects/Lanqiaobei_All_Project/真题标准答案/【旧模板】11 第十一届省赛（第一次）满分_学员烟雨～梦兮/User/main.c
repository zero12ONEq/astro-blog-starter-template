/* 头文件声明区域 */
#include <STC15F2K60S2.H>
#include "Led.h"
#include "Init.h"
#include "Key.h"
#include "Seg.h"
#include "I2C.h"

/* 变量声明区域 */
data unsigned char key_Val, key_Down, key_Up, key_Old, key_Scan_Slow;
data unsigned char seg_Scan_Slow, seg_Pos, seg_Buf[] = {10, 10, 10, 10, 10, 10, 10, 10};
data unsigned char led_Buf[] = {0, 0, 0, 0, 0, 0, 0, 0};

unsigned char seg_display_mode = 0; // 0-数据界面，1-参数界面，2-计数界面
float voltage, voltage_par, set_voltage_par;
unsigned int count = 0;
unsigned char eeprom_data;
unsigned char error_count;

unsigned int timer5000ms;
bit ff_flag;

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
      if(seg_display_mode == 0)
        set_voltage_par = voltage_par;
      else if(seg_display_mode == 1)
      {
        ff_flag = 0;
        voltage_par = set_voltage_par;
        eeprom_data = voltage_par * 10;
        EEPROM_Write(&eeprom_data, 0, 1);
      }
      if(++seg_display_mode == 3) seg_display_mode = 0;
      break;
    case 13: // 计数值清零
      if(seg_display_mode == 2)
        error_count = count = 0;
      else
        error_count++;
      break;
    case 16: // 参数 +
      if(seg_display_mode == 1)
      {
        error_count = 0;
        set_voltage_par += 0.5;
        if(set_voltage_par == 5.5) set_voltage_par = 0;
      }
      else
        error_count++;
      break;
    case 17: // 参数 -
      if(seg_display_mode == 1)
      {
        error_count = 0;
        set_voltage_par -= 0.5;
        if(set_voltage_par == -0.5) set_voltage_par = 5;
      }
      else
        error_count++;
      break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
  unsigned char i;
	if(seg_Scan_Slow) return;
	seg_Scan_Slow = 1;
	
	/* 信息读取区域 */
  voltage = AD_Read(3);
	
	/* 数据处理区域 */
  switch(seg_display_mode)
  {
    case 0: // 0-数据界面
      seg_Buf[0] = 11;
      seg_Buf[5] = (unsigned char)voltage % 10 + '.';
      seg_Buf[6] = (unsigned int)(voltage * 10) % 10;
      seg_Buf[7] = (unsigned int)(voltage * 100) % 10;
      break;
    case 1: // 1-参数界面
      seg_Buf[0] = 12;
      seg_Buf[5] = (unsigned char)set_voltage_par % 10 + '.';
      seg_Buf[6] = (unsigned int)(set_voltage_par * 10) % 10;
      seg_Buf[7] = (unsigned int)(set_voltage_par * 100) % 10;
      break;
    case 2: // 2-计数界面
      seg_Buf[0] = 13;
      seg_Buf[3] = count / 10000 % 10;
      seg_Buf[4] = count / 1000 % 10;
      seg_Buf[5] = count / 100 % 10;
      seg_Buf[6] = count / 10 % 10;
      seg_Buf[7] = count % 10;
    
      for(i = 3; seg_Buf[i] == 0 && i < 7; i++)
        seg_Buf[i] = 10;
      break;
  }
}

/* 其他处理函数 */
void Led_Proc()
{
  if(voltage > voltage_par)
    ff_flag = 1;
  else if(ff_flag == 1)
  {
    ff_flag = 0;
    count++;
  }
  
  led_Buf[1] = (count % 2 != 0);
  led_Buf[2] = (error_count >= 3);
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
  
  if(voltage >= voltage_par)
    led_Buf[0] = timer5000ms = 0;
  else if(++timer5000ms == 5000)
    led_Buf[0] = 1;
	
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
  EEPROM_Read(&eeprom_data, 0, 1);
  voltage_par = eeprom_data / 10.0;
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