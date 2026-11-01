/* ===================================头文件声明==================================== */
#include <STC15F2K60S2.H>
#include "Led.h"
#include "Init.h"
#include "Key.h"
#include "Seg.h"
#include "I2C.h"
#include <stdio.h>
#include <string.h>
#include "Uart.h"
#include <math.h>

#define UART_RX_SIZE 40
/* ====================================变量定义==================================== */
/* 核心变量 */
data unsigned char key_Val, key_Down, key_Up, key_Old, key_Scan_Slow;
data unsigned char seg_Scan_Slow, seg_Pos, seg_Buf[] = {10, 10, 10, 10, 10, 10, 10, 10};
data unsigned char led_Buf[] = {0, 0, 0, 0, 0, 0, 0, 0};
data unsigned char uart_recv_tick, uart_rx_data[UART_RX_SIZE], uart_rx_data_index;
//data unsigned int timer1000ms, freq; // 频率为 1 秒内上升沿的数量，Hz。频率 = 事件数 ÷ 时间

/* 状态标志 */
unsigned char da_mode = 0; // DA 输出模式：0-正弦波，1-方波，2-三角波

unsigned char work_mode = 0; // 0-波形发生，1-频率测量

bit error_flag; // 指令异常标志位

bit par_index; // 0-频率，1-幅值

/* 相关变量 */
unsigned char da_freq = 10; // 输出频率
unsigned int da_t = 100; // 输出周期（1000ms ~ 10ms）-> 1Hz - 100Hz

unsigned char da_fangbo;
unsigned char pwm_size = 50; // 方波的占空比
unsigned int pwm_max; // 高电平时间
  
float da_sanjiao = 0;
float sanjiao_count; // 每次增加的步长
float sanjiao_max; // 三角波的峰值
bit sanjiao_flag; // 上升和下降切换

float angle = 0; //角度
float radian = 0;	//弧度
float da_sin; // 正弦波输出值
float sin_count; // 每次增加的步长

unsigned int t_count; // 周期计数


unsigned int da_max_x100 = 300;

unsigned int freq;

/* 计时变量 */
unsigned int freq_timer1000ms;
unsigned char led1_timer100ms;
unsigned char led5_timer100ms;

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
    case 4: // 工作模式切换
      work_mode ^= 1;
      break;
    case 5: // 输出波形切换
      if(work_mode == 0 && ++da_mode == 3) da_mode = 0;
      sin_count = 360.0 / da_t;
      pwm_max = da_t * (pwm_size / 100.0);
      sanjiao_max = (da_max_x100 / 100.0) * 51.0;
      sanjiao_count = sanjiao_max / (da_t / 2.0);
      t_count = 0;
      sanjiao_flag = 0;
      break;
    case 8: // 参数选择
      par_index ^= 1;
      break;
    case 12: // 参数-
      if(par_index == 0)
      {
        if(--da_freq == 255) da_freq = 100;
        da_t = 1.0 / da_freq * 1000;
        
        if(da_mode == 0)
          sin_count = 360.0 / da_t;
        else if(da_mode == 1)
          pwm_max = da_t * (pwm_size / 100.0);
        else if(da_mode == 2)
          sanjiao_count = ((da_max_x100 / 100.0) * 51.0) / (da_t / 2.0);
      }
      else
      {
        da_max_x100 -= 10;
        if(da_max_x100 > 65000) da_max_x100 = 500;
          sanjiao_max = (da_max_x100 / 100.0) * 51.0;
          sanjiao_count = sanjiao_max / (da_t / 2.0);
      }
      break;
    case 13: // 参数+
      if(par_index == 0)
      {
        if(++da_freq == 101) da_freq = 1;
        da_t = 1.0 / da_freq * 1000;
        
        if(da_mode == 0)
          sin_count = 360.0 / da_t;
        else if(da_mode == 1)
          pwm_max = da_t * (pwm_size / 100.0);
        else if(da_mode == 2)
          sanjiao_count = sanjiao_max / (da_t / 2.0);
      }
      else
      {
        da_max_x100 += 10;
        
        if(da_max_x100 > 500) da_max_x100 = 0;
        sanjiao_max = (da_max_x100 / 100.0) * 51.0;
        sanjiao_count = sanjiao_max / (da_t / 2.0);
      }
      break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	if(seg_Scan_Slow) return;
	seg_Scan_Slow = 1;
	
	/* 信息读取处理区域 */
  
	/* 数据显示区域 */
  switch(work_mode)
  {
    case 0: // 波形发生模式
      seg_Buf[0] = 11;
      seg_Buf[1] = da_mode + 1;
      seg_Buf[2] = 10;
      seg_Buf[3] = da_freq >= 100 ? da_freq / 100 % 10 : 10;
      seg_Buf[4] = da_freq >= 10 ? da_freq / 10 % 10 : 10;
      seg_Buf[5] = da_freq % 10;
      seg_Buf[6] = da_max_x100 / 100 % 10 + '.';
      seg_Buf[7] = da_max_x100 / 10 % 10;
      break;
    case 1: // 频率测量模式
      seg_Buf[0] = 12;
      seg_Buf[1] = freq >= 1000000 ? freq / 1000000 % 10 : 10;;
      seg_Buf[2] = freq >= 100000 ? freq / 100000 % 10 : 10;;
      seg_Buf[3] = freq >= 10000 ? freq / 10000 % 10 : 10;
      seg_Buf[4] = freq >= 1000 ? freq / 1000 % 10 : 10;
      seg_Buf[5] = freq >= 100 ? freq / 100 % 10 : 10;
      seg_Buf[6] = freq >= 10 ? freq / 10 % 10 : 10;
      seg_Buf[7] = freq % 10;
      break;
  }
}

/* 其他处理函数 */
void Led_Proc()
{
  if(work_mode == 1)
    led_Buf[0] = 1;
  
  led_Buf[1] = (da_mode == 0);
  led_Buf[2] = (da_mode == 1);
  led_Buf[3] = (da_mode == 2);
  
  if(error_flag == 0)
    led_Buf[4] = 1;
  
  if(da_mode == 0)
    DA_Output(da_sin);
  
  if(da_mode == 1)
    DA_Output(da_fangbo);
    
  if(da_mode == 2)
    DA_Output(da_sanjiao);
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
	if(++key_Scan_Slow == 6) key_Scan_Slow = 0;
	if(++seg_Scan_Slow == 20) seg_Scan_Slow = 0;
  
  if(work_mode == 0)
  {
    if(++led1_timer100ms == 0)
    {
      led1_timer100ms = 0;
      led_Buf[0] ^= 1;
    }
  }
  else
    led1_timer100ms = 0;
  
  if(error_flag == 1)
  {
    if(++led5_timer100ms == 0)
    {
      led5_timer100ms = 0;
      led_Buf[4] ^= 1;
    }
  }
  else
    led5_timer100ms = 0;
  
	if(++freq_timer1000ms >= 1000)
	{
		freq_timer1000ms = 0;
		freq = (TH0 << 8) | TL0;
		TH0 = TL0 = 0x00;
	}
	
	if(++seg_Pos == 8) seg_Pos = 0;
	if(seg_Buf[seg_Pos] > 20) // 判断有无小数点，ASCII 为 46
		Seg_Display(seg_Pos, seg_Buf[seg_Pos] - '.', 1);
	else
		Seg_Display(seg_Pos, seg_Buf[seg_Pos], 0);
	    
  if(da_mode == 0) // 正弦波
  {
    angle += sin_count;		
    if(angle >= 360)			//一个周期360度
    {
      angle = 0;
    }
    radian = angle * 0.0055; //角度转弧度
    da_sin = 128 + (sin(radian * 3.1415) * ((da_max_x100 / 100.0) * 51.0 / 2.0 + 0.5));//标度变换，1~-1转换到 0~255
  }
  
  if(da_mode == 1)
  {
    if(t_count <= pwm_max)
      da_fangbo = (da_max_x100 / 100.0) * 51;
    else
      da_fangbo = 0;
    
    if(++t_count == da_t) t_count = 0;  
  }
  
  if(da_mode == 2) // 三角波
  {
    if(sanjiao_flag == 0)
    {
      da_sanjiao += sanjiao_count;
      if(da_sanjiao >= sanjiao_max)
      {
        da_sanjiao = sanjiao_max;
        sanjiao_flag = 1;
      }
    }
    else
    {
      da_sanjiao -= sanjiao_count;
      if(da_sanjiao <= 0)
      {
        da_sanjiao = 0;
        sanjiao_flag = 0;
      }
    }
  }
  
	Led_Display(led_Buf, 1);
	uart_recv_tick++;		
}

/* 串口 1 中断服务函数 */
void Uart1_Server(void) interrupt 4
{
	if (RI)				//检测串口1接收中断
	{
		uart_recv_tick = 0;
		uart_rx_data[uart_rx_data_index] = SBUF;
		RI = 0;			//清除串口1接收中断请求位
		if(++uart_rx_data_index == UART_RX_SIZE) uart_rx_data_index = 0;
	}
}

/* 串口处理函数 */
void Uart_Proc()
{
  int temp1_int, temp2_int;
  float temp_float;
  
	if(uart_rx_data_index == 0) return; // 没有数据则退出
	if(uart_recv_tick >= 10) // 超时解析
	{
		uart_recv_tick = 0;
		/* 串口数据解析区域 */
    if(sscanf(uart_rx_data, "WAVE,SIN,%d,%f", &temp1_int, &temp_float) == 2)
    {
      if(temp1_int >= 1 && temp1_int <= 100 && temp_float >= 0.0 && temp_float <= 5.0)
      {
        da_t = 1.0 / temp1_int * 1000;
        da_max_x100 = temp_float * 100;
        sin_count = 360.0 / da_t;
        printf("OK");
      }
      else
      {
        error_flag = 1;
        printf("ERROR");
      }
    }
    else if(sscanf(uart_rx_data, "WAVE,SQR,%d,%f,%d", &temp1_int, &temp_float, &temp2_int) == 3)
    {
      if(temp1_int >= 1 && temp1_int <= 100 && temp_float >= 0.0 && temp_float <= 5.0 && temp2_int >= 10 && temp2_int <= 90)
      {
        da_t = 1.0 / temp1_int * 1000;
        da_max_x100 = temp_float * 100;
        pwm_size = temp2_int;
        pwm_max = da_t * (pwm_size / 100.0);
        printf("OK");
      }
      else
      {
        error_flag = 1;
        printf("ERROR");
      }
    }
    else if(sscanf(uart_rx_data, "WAVE,TRI,%d,%f", &temp1_int, &temp_float) == 2)
    {
      if(temp1_int >= 1 && temp1_int <= 100 && temp_float >= 0.0 && temp_float <= 5.0)
      {
        da_t = 1.0 / temp1_int * 1000;
        da_max_x100 = temp_float * 100;
        sanjiao_max = (da_max_x100 / 100.0) * 51.0;
        sanjiao_count = sanjiao_max / (da_t / 2.0);
        printf("OK");
      }
      else
      {
        error_flag = 1;
        printf("ERROR");
      }
    }
    else if(strcmp(uart_rx_data, "FREQ?") == 0)
    {
      printf("FREQ,%u", freq);
    }
    else
    {
      error_flag = 1;
      printf("ERROR"); 
    }
    
		memset(uart_rx_data, 0, uart_rx_data_index); // 清空接收数据
		uart_rx_data_index = 0;
	}
}

/* ====================================主循环函数=================================== */
void main(void)
{
  
	/* 初始化区域 */
	System_Init();
  sin_count = 360.0 / da_t;
  pwm_max = da_t * (pwm_size / 100.0);
  sanjiao_max = (da_max_x100 / 100.0) * 51.0;
  sanjiao_count = sanjiao_max / (da_t / 2.0);
  
  Timer0_Init();
	Uart1_Init();
	Timer1_Init();

  
	/* 主循环 */
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Uart_Proc(); 
	}
}
