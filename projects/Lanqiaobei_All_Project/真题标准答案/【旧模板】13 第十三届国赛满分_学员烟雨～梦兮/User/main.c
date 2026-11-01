/* ===================================头文件声明==================================== */
#include <STC15F2K60S2.H>
#include "Led.h"
#include "Init.h"
#include "Key.h"
#include "Seg.h"
#include "I2C.h"
#include "Sonic.h"

/* ====================================变量定义==================================== */
/* 核心变量 */
data unsigned char key_Val, key_Down, key_Up, key_Old, key_Scan_Slow;
data unsigned char seg_Scan_Slow, seg_Pos, seg_Buf[] = {10, 10, 10, 10, 10, 10, 10, 10};
data unsigned char led_Buf[] = {0, 0, 0, 0, 0, 0, 0, 0};

/* 状态标志 */
unsigned char seg_display_mode = 0; // 数码管显示界面控制：0-频率界面，1-湿度界面，2-测距界面，3-参数界面
unsigned char par_mode = 0; // 参数子界面：0-频率，1-湿度，2-距离
bit freq_units_mode = 0; // 频率单位控制：0-Hz，1-KHz
bit distance_units_mode = 0; // 距离单位控制：0-CM，1-M
bit ff_flag; // 触发器
bit pwm_mode; // PWM 占空比模式控制：0-20%，1-80%
bit relay_flag; // 继电器状态：0-断开，1-吸合

/* 相关变量 */
unsigned char i; // 公共循环变量
unsigned int freq; // 测量到的频率值
unsigned char humidity; // 测量到的湿度值
unsigned char distance; // 测量到的距离值
unsigned char old_distance;
unsigned char par_data[3] = {90, 40, 6}; // 频率 * 10，湿度，距离 * 10
unsigned char relay_count; // 记录继电器的开关次数，开和关各一次

/* 计时变量 */
unsigned int timer1000ms; // 频率为 1 秒内上升沿的数量，Hz。频率 = 事件数 ÷ 时间
unsigned int key7_down_1000ms; // s7 按下超过 1 秒后松开
unsigned char timer200ms; // 每 200ms 输出进行一次 DA 输出，不然会使数码管显示延迟
unsigned char timer1000us; // 1KHz -> 周期为 1ms = 1000us
unsigned char timer100ms; // L1~L3 以 0.1s 为间隔闪烁

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
      par_mode = 0;
      if(seg_display_mode == 3)
        led_Buf[0] = led_Buf[1] = led_Buf[2] = timer100ms = 0;
      break;
    case 5: // 参数选择
      if(seg_display_mode == 3 && ++par_mode == 3) par_mode = 0;
      led_Buf[0] = led_Buf[1] = led_Buf[2] = timer100ms = 0;
      break;
    case 6: // 切换距离单位，参数 +
      if(seg_display_mode == 2)
        distance_units_mode ^= 1;
      else if(seg_display_mode == 3)
      {
        switch(par_mode)
        {
          case 0: // 频率参数
            par_data[par_mode] += 5;
            if(par_data[par_mode] == 125) par_data[par_mode] = 10;
            break;
          case 1: // 湿度参数
            par_data[par_mode] += 10;
            if(par_data[par_mode] == 70) par_data[par_mode] = 10;
            break;
          case 2: // 距离参数
            if(++par_data[par_mode] == 13) par_data[par_mode] = 1;
            break;
        }
      }
      break;
    case 7: // 切换频率单位，参数 -
      if(seg_display_mode == 0)
        freq_units_mode ^= 1;
      else if(seg_display_mode == 1)
        key7_down_1000ms = 0;
      else if(seg_display_mode == 3)
      {
        switch(par_mode)
        {
          case 0: // 频率参数
            par_data[par_mode] -= 5;
            if(par_data[par_mode] == 5) par_data[par_mode] = 120;
            break;
          case 1: // 湿度参数
            par_data[par_mode] -= 10;
            if(par_data[par_mode] == 0) par_data[par_mode] = 60;
            break;
          case 2: // 距离参数
            if(--par_data[par_mode] == 0) par_data[par_mode] = 12;
            break;
        }
      }
      break;
	}
  
  if(key_Up == 7 && key7_down_1000ms == 1000)
  {
    relay_count = 0;
    EEPROM_Write(&relay_count, 0, 1);
  }
}

/* 信息处理函数 */
void Seg_Proc()
{
	/* 信息读取处理区域 */
  switch(seg_Scan_Slow)
  {
    case 40:
      humidity = AD_Read(3) * 20;
      break;
    case 70:
      distance = Sonic_Read();
      break;
  }
  
  if(distance <= (par_data[2] * 10))
  {
    ff_flag = 1;
    if(relay_flag == 1)
    {
      relay_count++;
      EEPROM_Write(&relay_count, 0, 1);
    }
    relay_flag = 0;
    Relay(0);
  }
  else if(ff_flag == 1)
  {
    ff_flag = 0;
    relay_flag = 1;
    relay_count++;
    EEPROM_Write(&relay_count, 0, 1);
  }
  
	/* 数据显示区域 */
  if(seg_Scan_Slow) return;
	seg_Scan_Slow = 1;

  switch(seg_display_mode)
  {
    case 0: // 0-频率界面
      seg_Buf[0] = 11; // F
      seg_Buf[1] = seg_Buf[2] = 10;

      seg_Buf[3] = freq_units_mode ? 10 : freq / 10000 % 10;
      seg_Buf[4] = freq_units_mode ? 10 : freq / 1000 % 10;
      seg_Buf[5] = freq_units_mode ? freq / 10000 % 10 : freq / 100 % 10;
      seg_Buf[6] = freq_units_mode ? freq / 1000 % 10 + '.' : freq / 10 % 10;
      seg_Buf[7] = freq_units_mode ? freq / 100 % 10 : freq % 10;
      
      for(i = freq_units_mode ? 5 : 3; seg_Buf[i] == 0 && i < 7; i++) // 高位熄灭
        seg_Buf[i] = 10;
      break;
    case 1: // 1-湿度界面
      seg_Buf[0] = 12; // H
      seg_Buf[1] = seg_Buf[2] = seg_Buf[3] = seg_Buf[4] = seg_Buf[5] = 10;
      seg_Buf[6] = humidity / 10 % 10;
      seg_Buf[7] = humidity % 10;
      break;
    case 2: // 2-测距界面
      seg_Buf[0] = 13; // A
      seg_Buf[1] = seg_Buf[2] = seg_Buf[3] = seg_Buf[4] = 10;
    
      seg_Buf[5] = distance_units_mode ? distance / 100 % 10 + '.' : distance / 100 % 10;
      seg_Buf[6] = distance / 10 % 10;
      seg_Buf[7] = distance % 10;
    
      for(i = 5; seg_Buf[i] == 0 && i < 7; i++) // 高位熄灭
        seg_Buf[i] = 10;
      break;
    case 3: // 3-参数界面
      seg_Buf[0] = 14; // P
      seg_Buf[1] = par_mode + 1;
      seg_Buf[2] = seg_Buf[3] = seg_Buf[4] = seg_Buf[5] = 10;
      switch(par_mode)
      {
        case 0: // 频率参数
          seg_Buf[5] = par_data[par_mode] / 100 % 10 == 0 ? 10 : par_data[par_mode] / 100 % 10;
          seg_Buf[6] = par_data[par_mode] / 10 % 10 + '.';
          seg_Buf[7] = par_data[par_mode] % 10;
          break;
        case 1: // 湿度参数
          seg_Buf[6] = par_data[par_mode] / 10 % 10;
          seg_Buf[7] = par_data[par_mode] % 10;
          break;
        case 2: // 距离参数
          seg_Buf[6] = par_data[par_mode] / 10 % 10 + '.';
          seg_Buf[7] = par_data[par_mode] % 10;
          break;
      }
      break;
  }
}

/* 其他处理函数 */
void Led_Proc()
{
  if(seg_display_mode < 3)
  {
    for(i = 0; i < 3; i++)
      led_Buf[i] = (i == seg_display_mode);
  }
  
  led_Buf[3] = (freq > (float)par_data[0] / 10 * 1000);
  led_Buf[4] = (humidity > par_data[1]);
  led_Buf[5] = (distance > (float)par_data[2] / 10 * 100);
  
  if(timer200ms == 0)
  {
    if(humidity >= 80)
      DA_Output(5);
    else if(humidity <= par_data[1])
      DA_Output(1);
    else
      DA_Output((4.0 * (humidity - (int)par_data[1])) / (80.0 - (float)par_data[1]) + 1);
  }
  
  Relay(relay_flag);
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

/* 定时器 2 初始化函数 */
void Timer2_Init(void)		//100微秒@12.000MHz
{
	AUXR &= 0xFB;			//定时器时钟12T模式
	T2L = 0x9C;				//设置定时初始值
	T2H = 0xFF;				//设置定时初始值
	AUXR |= 0x10;			//定时器2开始计时
  IE2 |= 0x04;			//使能定时器2中断
}

/* 定时器 1 中断处理函数 */
void Timer1_Server() interrupt 3
{
	if(++key_Scan_Slow == 10) key_Scan_Slow = 0;
	if(++seg_Scan_Slow == 100) seg_Scan_Slow = 0;
  if(++timer200ms == 200) timer200ms = 0;
  
  if(++timer1000ms == 1000)
	{
		timer1000ms = 0;
		freq = ((TH0 << 8) | TL0);
		TH0 = TL0 = 0x00;
	}
  
  
  if(seg_display_mode == 3 && ++timer100ms == 100)
  {
    timer100ms = 0;
    led_Buf[par_mode] ^= 1;
  }
  
  if(seg_display_mode == 1 && key_Old == 7 && ++key7_down_1000ms >= 1000)
    key7_down_1000ms = 1000;
  
	if(++seg_Pos == 8) seg_Pos = 0;
	if(seg_Buf[seg_Pos] > 20) // 判断有无小数点，ASCII 为 46
		Seg_Display(seg_Pos, seg_Buf[seg_Pos] - '.', 1);
	else
		Seg_Display(seg_Pos, seg_Buf[seg_Pos], 0);
	
	Led_Display(led_Buf, 1);
}

void Timer2_Server() interrupt 12
{ 
  pwm_mode = (freq > (par_data[0] * 100));
  if(timer1000us < (pwm_mode ? 8 : 2))
  {
    Mot(1);
  }
  else
  {
    Mot(0);
  }
  
  if(++timer1000us == 10) timer1000us = 0;
}

/* ====================================主循环函数=================================== */
void main(void)
{
  
	/* 初始化区域 */
	System_Init();
  AD_Read(3);
  Timer0_Init();
	Timer1_Init();
	Timer2_Init();
  
	/* 主循环 */
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}
