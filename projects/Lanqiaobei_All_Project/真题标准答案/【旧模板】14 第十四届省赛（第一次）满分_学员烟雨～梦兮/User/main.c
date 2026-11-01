/* 头文件声明区域 */
#include <STC15F2K60S2.H>
#include "Led.h"
#include "Init.h"
#include "Key.h"
#include "Seg.h"
#include "Ds1302.h"
#include "Ds18b20.h"
#include "I2C.h"


#define UART_RX_SIZE 10

/* 变量声明区域 */
data unsigned char key_Val, key_Down, key_Up, key_Old, key_Scan_Slow;
data unsigned char seg_Scan_Slow, seg_Pos, seg_Buf[] = {10, 10, 10, 10, 10, 10, 10, 10};
data unsigned char led_Buf[] = {0, 0, 0, 0, 0, 0, 0, 0};
data unsigned int timer1000ms, freq; // 频率为 1 秒内上升沿的数量，Hz。频率 = 事件数 ÷ 时间

unsigned char seg_display_mode = 0; // 0-时间界面，1-回显界面，2-参数界面，3-温湿度界面
unsigned char echo_display_mode = 0; // 回显界面子菜单：0-温度回显，1-湿度回显，2-时间回显
unsigned char rtc_time[3] = {13, 3, 5};
float temperature;
float temperature_sum;
unsigned char max_temperature, temperature_par = 30;
float humidity;
float humidity_sum;
unsigned char max_humidity;
unsigned char ff_count;
unsigned char ff_time[2]; // 最近一次的触发时间
unsigned int timer2000ms;
float voltage; // 大于 4 = 亮，小于为挡光
bit gather_flag; // 判断是否进行采集
bit freq_error_flag; // 频率数据无效标志位
unsigned char old_seg_display_mode[2]; // 保存触发前的显示状态
unsigned int timer3000ms;
unsigned char timer100ms;
float old_gather_data[2]; // 上一次采集到的数据

/* 按键处理函数 */
void Key_Proc()
{
	if(key_Scan_Slow || seg_display_mode == 3) return;
	key_Scan_Slow = 1;
	
	key_Val = Key_Read();
	key_Down = key_Val & (key_Val ^ key_Old);
	key_Up = ~key_Val & (key_Val ^ key_Old);
	key_Old = key_Val;
 
	switch(key_Down)
	{
    case 4: // 界面切换
      if(seg_display_mode == 0) 
        echo_display_mode = 0;
      if(++seg_display_mode == 3) seg_display_mode = 0;
      break;
    case 5: // 回显界面子界面切换
      if(seg_display_mode == 1 && ++echo_display_mode == 3) echo_display_mode = 0;
      break;
    case 8: // 参数 +
      if(seg_display_mode == 2 && ++temperature_par == 100) 
        temperature_par = 99;
      break;
    case 9: // 参数 -，清空数据
      if(seg_display_mode == 2 && --temperature_par == 255) 
        temperature_par = 0;
      else if(echo_display_mode == 2)
        timer2000ms = 0;
      break;
	}
  
    
  if(echo_display_mode == 2 && key_Up == 9 && timer2000ms == 2000) // 长按判断
  {
      ff_count = ff_time[0] = ff_time[1] = 0;
      humidity = humidity_sum = max_humidity = 0;
      temperature = temperature_sum = max_temperature = 0;
      old_gather_data[0] = old_gather_data[1] = 0;
  }
}

/* 信息处理函数 */
void Seg_Proc()
{
	if(seg_Scan_Slow) return;
	seg_Scan_Slow = 1;
	
	/* 信息读取区域 */
	Read_Rtc(rtc_time);
  voltage = AD_Read(1);
  
  if(seg_display_mode != 3 && voltage > 4.0)
    gather_flag = 1;
  else if(gather_flag)
  {
    gather_flag = 0;
    
    old_gather_data[0] = temperature;
    temperature = Read_Temperature();
    if(max_temperature < temperature) max_temperature = temperature;
    
    freq_error_flag = (freq < 200 || freq > 2000);
    if(freq_error_flag == 0) 
    {
      temperature_sum += temperature;
      
      old_gather_data[1] = humidity;
      humidity = (2.0 * (float)freq + 50.0) / 45.0 - 5.6;
      humidity_sum += humidity;
      if(max_humidity < humidity) max_humidity = humidity;
      ff_count++;
    }
    
    ff_time[0] = rtc_time[0];
    ff_time[1] = rtc_time[1];
    
    old_seg_display_mode[0] = seg_display_mode;
    old_seg_display_mode[1] = echo_display_mode;
    
    seg_display_mode = 3;
    timer3000ms = 0;
  }
  
	/* 数据处理区域 */
  switch(seg_display_mode)
  {
    case 0: // 0-时间界面
      seg_Buf[2] = seg_Buf[5] = 11;
      seg_Buf[0] = rtc_time[0] / 10 % 10;
      seg_Buf[1] = rtc_time[0] % 10;
      seg_Buf[3] = rtc_time[1] / 10 % 10;
      seg_Buf[4] = rtc_time[1] % 10;
      seg_Buf[6] = rtc_time[2] / 10 % 10;
      seg_Buf[7] = rtc_time[2] % 10;
      break;
    case 1: // 1-回显界面
      switch(echo_display_mode)
      {
        case 0: // 0-温度回显
          seg_Buf[0] = 12;
          seg_Buf[1] = 10;
        
          if(ff_count == 0)
            seg_Buf[2] = seg_Buf[3] = seg_Buf[4] = seg_Buf[5] = seg_Buf[6] = seg_Buf[7] = 10;
          else
          {
            seg_Buf[4] = 11;
            seg_Buf[2] = max_temperature / 10 % 10;
            seg_Buf[3] = max_temperature % 10;
            seg_Buf[5] = (unsigned char)(temperature_sum / ff_count) / 10 % 10;
            seg_Buf[6] = (unsigned char)(temperature_sum / ff_count) % 10 + '.';
            seg_Buf[7] = (unsigned int)((temperature_sum / ff_count) * 10) % 10;
          }
          break;
        case 1: // 1-湿度回显
          seg_Buf[0] = 13;
          seg_Buf[1] = 10;
          
          if(ff_count == 0)
            seg_Buf[2] = seg_Buf[3] = seg_Buf[4] = seg_Buf[5] = seg_Buf[6] = seg_Buf[7] = 10;
          else
          {
            seg_Buf[4] = 11;
            seg_Buf[2] = max_humidity / 10 % 10;
            seg_Buf[3] = max_humidity % 10;
            seg_Buf[5] = (unsigned char)(humidity_sum / ff_count) / 10 % 10;
            seg_Buf[6] = (unsigned char)(humidity_sum / ff_count) % 10 + '.';
            seg_Buf[7] = (unsigned int)(humidity_sum / ff_count * 10) % 10;
          }
          break;
        case 2: // 2-时间回显
          seg_Buf[0] = 14;
          seg_Buf[1] = ff_count / 10 % 10;
          seg_Buf[2] = ff_count % 10;
          seg_Buf[5] = ff_count == 0 ? 10 : 11;
          seg_Buf[3] = ff_count == 0 ? 10 : ff_time[0] / 10 % 10;
          seg_Buf[4] = ff_count == 0 ? 10 : ff_time[0] % 10;
          seg_Buf[6] = ff_count == 0 ? 10 : ff_time[1] / 10 % 10;
          seg_Buf[7] = ff_count == 0 ? 10 : ff_time[1] % 10;
          break;
      }
      break;
    case 2: // 2-参数界面
      seg_Buf[0] = 15;
      seg_Buf[1] = seg_Buf[2] = seg_Buf[3] = seg_Buf[4] = seg_Buf[5] = 10;
      seg_Buf[6] = temperature_par / 10 % 10;
      seg_Buf[7] = temperature_par % 10;
    break;
    case 3: // 3-温湿度界面
      seg_Buf[0] = 16;
      seg_Buf[1] = seg_Buf[2] = 10;
      seg_Buf[5] = 11;
      seg_Buf[3] = (unsigned char)temperature / 10 % 10;
      seg_Buf[4] = (unsigned char)temperature % 10;
      seg_Buf[6] = freq_error_flag ? 17 : (unsigned char)humidity / 10 % 10;
      seg_Buf[7] = freq_error_flag ? 17 : (unsigned char)humidity % 10;
      break;
    case 4:
      seg_Buf[3] = freq / 10000 % 10;
      seg_Buf[4] = freq / 1000 % 10;
      seg_Buf[5] = freq / 100 % 10;
      seg_Buf[6] = freq / 10 % 10;
      seg_Buf[7] = freq % 10;
      break;
  }
}

/* 其他处理函数 */
void Led_Proc()
{
  unsigned char i;
	for(i = 0; i < 3; i++)
    led_Buf[i] = (i == seg_display_mode);
  
  led_Buf[2] = (seg_display_mode == 3);
  
  led_Buf[4] = freq_error_flag;
  
  led_Buf[5] = (ff_count >= 2 && temperature > old_gather_data[0] && humidity > old_gather_data[1]);
}

/* 计数器 0 初始化函数 */
void Timer0_Init(void)
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			//设置为计数器，不自动重装载模式
	TH0 = TL0 = 0x00;		//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//计数器0开始计数
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
  
	if(++timer1000ms == 1000)
	{
		timer1000ms = 0;
		freq = (TH0 << 8) | TL0;
		TH0 = TL0 = 0x00;
	}
  
  if(echo_display_mode == 2 && key_Old == 9 && ++timer2000ms >= 2000) timer2000ms = 2000;
	
  if(seg_display_mode == 3 && ++timer3000ms == 3000)
  {
    seg_display_mode = old_seg_display_mode[0];
    echo_display_mode = old_seg_display_mode[1];
  }
  
  if(temperature > temperature_par && ++timer100ms == 100)
  {
    timer100ms = 0;
    led_Buf[3] ^= 1;
  }
  
	if(++seg_Pos == 8) seg_Pos = 0;
	if(seg_Buf[seg_Pos] > 20) // 判断有无小数点，ASCII 为 46
		Seg_Display(seg_Pos, seg_Buf[seg_Pos] - '.', 1);
	else
		Seg_Display(seg_Pos, seg_Buf[seg_Pos], 0);
	
	Led_Display(led_Buf, 1);
//	uart_recv_tick++;		
}

/* Main */
void main(void)
{
	/* 初始化区域 */
	while(Read_Temperature() == 85);
  Set_Rtc(rtc_time);
  AD_Read(1);
	System_Init();
  Timer0_Init();
	Timer1_Init();
//	Uart1_Init();
	
	/* 主循环 */
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
//		Uart_Proc(); 
	}
}