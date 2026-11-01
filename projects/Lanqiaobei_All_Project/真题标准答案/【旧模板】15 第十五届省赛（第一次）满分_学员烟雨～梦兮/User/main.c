/* ===================================头文件声明==================================== */
#include <STC15F2K60S2.H>
#include "Led.h"
#include "Init.h"
#include "Key.h"
#include "Seg.h"
#include "Ds1302.h"
#include "I2C.h"

/* ====================================变量定义==================================== */
/* 核心变量 */
data unsigned char key_Val, key_Down, key_Up, key_Old, key_Scan_Slow;
data unsigned char seg_Scan_Slow, seg_Pos, seg_Buf[] = {10, 10, 10, 10, 10, 10, 10, 10};
data unsigned char led_Buf[] = {0, 0, 0, 0, 0, 0, 0, 0};

/* 界面控制 */
unsigned char seg_display_mode = 0; // 主菜单：0-频率界面，1-参数界面，2-时间界面，3-回显界面
unsigned char par_display_mode = 0; // 参数界面子菜单：0-超限参数界面，1-校准值参数界面
unsigned char echo_display_mode = 0; // 回显界面子菜单：0-最大频率，1-最大频率时间  

/* 相关变量 */
unsigned char i;
unsigned int freq = 0; // 频率为 1 秒内上升沿的数量，Hz。频率 = 事件数 ÷ 时间
unsigned int max_freq = 0; // 记录最大频率值
unsigned char max_freq_time[3]; // 记录最大频率值产生的时间
unsigned char freq_par = 2; // 将超限参数除 1000 进行存储，1000 ~ 9000 = 1 ~ 9，每次加减 1000 = 1；
char calibration_value = 0; // 将校准值除 100 进行存储，-900 ~ 900 = -9 ~ 9，每次加减 100 = 1;
unsigned char rtc_time[3] = {13, 03, 05}; // 存储从 DS1302 获取的时、分、秒数据

/* 计时变量 */
unsigned int timer1000ms; // 每 1s 进行一次频率采集
unsigned char led1_timer200ms; // L1 以 0.2s 为间隔亮灭
unsigned char freq_timer200ms;
unsigned char timer200ms; // 每 200ms 进行一次 DA 输出

/* 状态标志 */
bit freq_error_flag; // 频率测量错误标志

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
    case 4: // 主界面切换
      if(++seg_display_mode == 4) seg_display_mode = 0;  
      par_display_mode = echo_display_mode = 0; // 每次切换时，重置子菜单的选择
      led_Buf[0] = led1_timer200ms = 0;
      break;
    case 5: // 子菜单选择
      if(seg_display_mode == 1) // 参数子菜单选择
        par_display_mode ^= 1;
      else if(seg_display_mode == 3) // 回显子菜单选择
        echo_display_mode ^= 1;
      break;
    case 8: // 参数 +
      if(seg_display_mode == 1)
      {
        if(par_display_mode == 0 && ++freq_par == 10) freq_par = 9;
        else if(par_display_mode == 1 && ++calibration_value == 10) calibration_value = 9;
      }
      break;
    case 9: // 参数 -
      if(seg_display_mode == 1)
      {
        if(par_display_mode == 0 && --freq_par == 0) freq_par = 1;
        else if(par_display_mode == 1 && --calibration_value == -10) calibration_value = -9;
      }
      break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	if(seg_Scan_Slow) return;
	seg_Scan_Slow = 1;
	
	/* 信息读取区域 */
  Read_Rtc(rtc_time);
	
	/* 数据处理区域 */
  switch(seg_display_mode)
  {
    case 0: // 频率界面
      seg_Buf[0] = 11;
      seg_Buf[1] = seg_Buf[2] = 10;
      seg_Buf[3] = freq_error_flag ? 10 : freq / 10000 % 10;
      seg_Buf[4] = freq_error_flag ? 10 : freq / 1000 % 10;
      seg_Buf[5] = freq_error_flag ? 10 : freq / 100 % 10;
      seg_Buf[6] = freq_error_flag ? 16 : freq / 10 % 10;
      seg_Buf[7] = freq_error_flag ? 16 : freq % 10;
    
      for(i = 3; seg_Buf[i] == 0 && i < 7; i++) // 高位熄灭
        seg_Buf[i] = 10;
    
      break;
    case 1: // 参数界面
      seg_Buf[0] = 12;
      seg_Buf[1] = par_display_mode + 1;
      seg_Buf[2] = seg_Buf[3] = 10;
      switch(par_display_mode)
      {
        case 0: // 超限参数界面
          seg_Buf[4] = freq_par;
          seg_Buf[5] = 0;
          seg_Buf[6] = 0;
          seg_Buf[7] = 0;
          break;
        case 1: // 校准值参数界面
          seg_Buf[4] = calibration_value < 0 ? 13 : 10;
          seg_Buf[5] = calibration_value == 0 ? 10 : (calibration_value < 0 ? -calibration_value: calibration_value);
          seg_Buf[6] = calibration_value == 0 ? 10 : 0;
          seg_Buf[7] = 0;
          break;
      }
      break;
    case 2: // 时间界面
      seg_Buf[2] = seg_Buf[5] = 13;
      seg_Buf[0] = rtc_time[0] / 10 % 10;
      seg_Buf[1] = rtc_time[0] % 10;
      seg_Buf[3] = rtc_time[1] / 10 % 10;
      seg_Buf[4] = rtc_time[1] % 10;
      seg_Buf[6] = rtc_time[2] / 10 % 10;
      seg_Buf[7] = rtc_time[2] % 10;
      break;
    case 3: // 回显界面 
      seg_Buf[0] = 14;
      switch(echo_display_mode)
      {
        case 0: // 0-最大频率
          seg_Buf[1] = 11;
          seg_Buf[2] = 10;
          seg_Buf[3] = max_freq / 10000 % 10;
          seg_Buf[4] = max_freq / 1000 % 10;
          seg_Buf[5] = max_freq / 100 % 10;
          seg_Buf[6] = max_freq / 10 % 10;
          seg_Buf[7] = max_freq % 10;
        
          for(i = 3; seg_Buf[i] == 0 && i < 7; i++) // 高位熄灭
            seg_Buf[i] = 10;
          break;
        case 1: // 1-最大频率时间  
          seg_Buf[1] = 15;
          seg_Buf[2] = max_freq_time[0] / 10 % 10;
          seg_Buf[3] = max_freq_time[0] % 10;
          seg_Buf[4] = max_freq_time[1] / 10 % 10;
          seg_Buf[5] = max_freq_time[1] % 10;
          seg_Buf[6] = max_freq_time[2] / 10 % 10;
          seg_Buf[7] = max_freq_time[2] % 10;
          break;
      }
      break;
  }
}

/* 其他处理函数 */
void Led_Proc()
{
  freq_error_flag = (freq > 64000); // 频率为无符号整数，范围为 0~65535，所以当大于 64000，即判断出现了负数
  if(max_freq < freq) // 判断是否更新频率最大值
  {
    max_freq = freq;
    for(i = 0; i < 3; i++)
      max_freq_time[i] = rtc_time[i];
  }
  
  if(timer200ms) return;
  
  if(freq_error_flag == 0)
  {
    if(freq >= (freq_par * 1000))
      DA_Output(5);
    else if(freq <= 500)
      DA_Output(1);
    else
      DA_Output((4.0 / (freq_par * 1000 - 500)) * (freq - 500) + 1);
  }
  else
  {
    DA_Output(0);
    led_Buf[1] = 1;
  }
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
  
  if(seg_display_mode == 0 && ++led1_timer200ms == 200)
  {
    led1_timer200ms = 0;
    led_Buf[0] ^= 1;
  }
  
	if(++timer1000ms == 1000)
	{
		timer1000ms = 0;
		freq = ((TH0 << 8) | TL0);
    freq = freq + (calibration_value * 100); 
		TH0 = TL0 = 0x00;
	}
  
  if(++timer200ms == 200)
  {
    timer200ms = 0;
  }
	
  if(freq_error_flag == 0)
  {
    if(freq > (freq_par * 1000))
    {
      if(++freq_timer200ms == 200)
      {
        freq_timer200ms = 0;
        led_Buf[1] ^= 1;
      }
    }
    else
      led_Buf[1] = freq_timer200ms = 0;
  }
  
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
  Timer0_Init();
	Timer1_Init();
  
	/* 主循环 */
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}
