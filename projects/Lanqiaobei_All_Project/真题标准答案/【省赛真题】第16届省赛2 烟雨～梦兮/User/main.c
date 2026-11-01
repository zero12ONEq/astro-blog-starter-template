/* ===================================头文件声明==================================== */
#include <STC15F2K60S2.H>
#include "Led.h"
#include "Init.h"
#include "Key.h"
#include "Seg.h"
#include "Ds1302.h"
#include "Ds18b20.h"
#include "I2C.h"
#include "Sonic.h"
#include <stdio.h>
#include <string.h>
#include "Uart.h"

//#define UART_RX_SIZE 10
/* ====================================变量定义==================================== */
/* 核心变量 */
data unsigned char key_scan_slow, key_val, key_down, key_up, key_old;
data unsigned char seg_scan_slow, seg_pos, seg_buf[] = {10, 10, 10, 10, 10, 10, 10, 10};
data unsigned char led_buf[] = {0, 0, 0, 0, 0, 0, 0, 0};
//data unsigned char uart_recv_tick, uart_rx_data[UART_RX_SIZE], uart_rx_data_index;
//data unsigned int freq_timer1000ms, freq; // 频率为 1 秒内上升沿的数量，Hz。频率 = 事件数 ÷ 时间

/* 状态标志 */
unsigned char seg_display_mode = 1; // 1-测距界面，2-时间间隔设置界面，3-累加次数设置界面

bit measure_mode = 0; // 测量模式：0-单次测量，1-累加测量
bit measure_start_flag = 0; // 是否开始进行累加测量

bit quench_flag = 1; // 数码管是否熄灭(上电时熄灭)

/* 相关变量 */
float temperature; // DS18B20 测量到的温度
unsigned int distance; // 超声波测距
unsigned char measure_count; // 累加测量计数

unsigned char time_interval_par = 2; // 时间间隔参数（s）
unsigned char set_time_par[4] = {2, 4, 6, 4}, set_time_par_index; // 保存时间间隔参数的所有可能

unsigned char times_count_par = 3; // 累加次数参数
unsigned char set_times_par[4] = {3, 5, 7, 5}, set_times_par_index; // 保存累加次数参数的所有可能

/* 计时变量 */
unsigned int measure_timer; // 累加测量计时变量
unsigned char led_timer100ms; // 累加测量进行中时，指示灯 L3 以 0.1 秒为间隔切换亮灭状态

/* ====================================用户函数==================================== */
void DAC_Transition(unsigned int distance) // 将超声波测距的结果转换为 DAC 输出
{
  if(distance <= 20)
    DA_Output(1);
  else if(distance >= 100)
    DA_Output(5);
  else
    DA_Output(((distance - 20) / 20.0) + 1);
}

void DAC_Transition_IRQ(unsigned int distance) // 因为一个函数在中断内和中断外都有调用的话，有可能会导致函数内部的数据出现错误，所以重复定义一个函数
{
  if(distance <= 20)
    DA_Output(1);
  else if(distance >= 100)
    DA_Output(5);
  else
    DA_Output(((distance - 20) / 20.0) + 1);
}

/* ====================================任务函数==================================== */
/* 按键处理函数 */
void Key_Proc()
{
	if(key_scan_slow < 10) return;
  key_scan_slow = 0;
	
	key_val = Key_Read();
  key_down = key_val & (key_val ^ key_old);
  key_up = ~key_val & (key_val ^ key_old);
  key_old = key_val;
  
	switch(key_down)
	{
    case 4: // 启动一次单次测量(可以立即结束正在执行的累加测量过程，并立即启动一次单次测量)
      if(seg_display_mode == 1)
      {
        measure_start_flag = 0; // 结束累加测量
        measure_mode = 0; // 单次测量
        distance = (Sonic_Read() / 340.0) * (330 + 0.6 * temperature);
        DAC_Transition(distance); // 更新 DAC 输出
        quench_flag = 0; // 取消熄灭
        led_buf[1] = (distance < 20 || distance > 100); // 单次测量或累加测量的最终结果小于 20cm 或大于 100cm 时，指示灯 L2 点亮
      }
      break;
    case 5: // 启动累加测量
      if(seg_display_mode == 1)
      {
        measure_timer = 0;
        measure_mode = 1; // 累加测量
        distance = (Sonic_Read() / 340.0) * (330 + 0.6 * temperature); // 立即启动累加测量的首次测距
        quench_flag = 0; // 取消熄灭
        measure_count = times_count_par - 1; // 去掉首次累加测量
        measure_start_flag = 1; // 开始进行累加测量
      }
      break;
    case 8: // 界面循环切换
      if(measure_start_flag == 0 && ++seg_display_mode == 4) seg_display_mode = 1;
      break;
    case 9: // 参数设置
      if(seg_display_mode == 2)
      {
        if(++set_time_par_index == 4) set_time_par_index = 0;
        time_interval_par = set_time_par[set_time_par_index];
      }
      else if(seg_display_mode == 3)
      {
        if(++set_times_par_index == 4) set_times_par_index = 0;
        times_count_par = set_times_par[set_times_par_index];
      }
      break;
    case 12: // 清除(返回图 3-3 所示的界面)
      if(measure_start_flag == 0)
      {
        measure_mode = 0;
        seg_display_mode = 1; // 返回测距界面
        quench_flag = 1; // 熄灭
        distance = 0;
      }
      break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	/* 信息读取处理区域 */
  switch(seg_scan_slow)
  {
    case 30:
      temperature = Read_Temperature();
      break;
  }
  
	/* 数据显示区域 */
  if(seg_scan_slow < 100) return;
  seg_scan_slow = 0;
  
  switch(seg_display_mode)
  {
    case 1: // 1-测距界面
      seg_buf[0] = measure_mode ? 12 : 11; // E : A
      seg_buf[1] = 13; // C
      seg_buf[2] = (unsigned int)temperature / 10 % 10;
      seg_buf[3] = (unsigned int)temperature % 10;
      seg_buf[4] = (quench_flag == 0 && distance >= 1000) ? distance / 1000 % 10 : 10;
      seg_buf[5] = (quench_flag == 0 && distance >= 100) ? distance / 100 % 10 : 10;
      seg_buf[6] = (quench_flag == 0 && distance >= 10) ? distance / 10 % 10 : 10;
      seg_buf[7] = quench_flag == 0 ? distance % 10 : 10;
      break;
    case 2: // 2-时间间隔设置界面
      seg_buf[0] = 14; // P
      seg_buf[1] = 1; 
      seg_buf[2] = seg_buf[3] = seg_buf[4] = seg_buf[5] = 10;
      seg_buf[6] = time_interval_par / 10 % 10;
      seg_buf[7] = time_interval_par % 10;
      break;
    case 3: // 3-累加次数设置界面
      seg_buf[0] = 14; // P
      seg_buf[1] = 2; 
      seg_buf[2] = seg_buf[3] = seg_buf[4] = seg_buf[5] = 10;
      seg_buf[6] = times_count_par / 10 % 10;
      seg_buf[7] = times_count_par % 10;
      break;
  }
}

/* 其他处理函数 */
void Led_Proc()
{
  led_buf[0] = (seg_display_mode == 1); // 测距界面 LED1 点亮
  
  if(quench_flag == 1) // 未完成任何有效测量时,DAC 输出 0V。
    DA_Output(0);
}

/* 定时器 1 中断处理函数 */
void Timer1_Server() interrupt 3
{
  if(measure_start_flag == 1) // 累加测量
  {
    if(++measure_timer >= time_interval_par * 1000)
    {
      measure_timer = 0;
      distance += (Sonic_Read() / 340.0) * (330 + 0.6 * temperature); // 累加测量
      if(--measure_count == 0) // 次数归 0 时结束此次累加测量 
      {
        DAC_Transition_IRQ(distance); // 更新 DAC 输出
        led_buf[1] = (distance < 20 || distance > 100); // 单次测量或累加测量的最终结果小于 20cm 或大于 100cm 时，指示灯 L2 点亮
        measure_start_flag = 0;
      }
    }
    
    if(++led_timer100ms >= 100) // 累加测量进行中时，指示灯 L3 以 0.1 秒为间隔切换亮灭状态
    {
      led_timer100ms = 0;
      led_buf[2] ^= 1;
    }
  }
  else
    led_buf[2] = led_timer100ms = 0;
  
  /* 根据对应的 BUF 刷新 LED 与数码管 */
  if(seg_buf[seg_pos] > 20)
    Seg_Display(seg_pos, seg_buf[seg_pos] - '.', 1);
  else
    Seg_Display(seg_pos, seg_buf[seg_pos], 0);
  
  Led_Display(led_buf, 1);
  
  if(++seg_pos == 8) seg_pos = 0;
  key_scan_slow++;
  seg_scan_slow++;
//  uart_recv_tick++;
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
	ET1 = 1;
}

/* ====================================主循环函数=================================== */
void main(void)
{
  
	/* 初始化区域 */
	System_Init();
  while((temperature = Read_Temperature()) == 85);
//  Set_Rtc(rtc_time);
//	Uart1_Init();
//  Timer0_Init();
	Timer1_Init();
  EA = 1;
	/* 主循环 */
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
//		Uart_Proc(); 
	}
}
