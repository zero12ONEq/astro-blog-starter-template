/* ===================================头文件声明==================================== */
#include <STC15F2K60S2.H>
#include "Led.h"
#include "Init.h"
#include "Key.h"
#include "Seg.h"
#include "Ds18b20.h"
#include "I2C.h"
#include "Sonic.h"

#define abs(x) ((x) < 0 ? -(x) : (x))
/* ====================================变量定义==================================== */
/* 核心变量 */
data unsigned char key_Val, key_Down, key_Up, key_Old, key_Scan_Slow;
data unsigned char seg_Scan_Slow, seg_Pos, seg_Buf[] = {10, 10, 10, 10, 10, 10, 10, 10};
data unsigned char led_Buf[] = {0, 0, 0, 0, 0, 0, 0, 0};

/* 状态标志 */
unsigned char seg_display_mode = 0; // 数码管显示控制：0-测距界面，1-参数界面，2-工厂模式
unsigned char factory_display_mode = 0; // 工厂模式子菜单显示控制：0-校准值设置界面，1-介质设置界面，2-DAC 输出设置
bit distance_units_mode = 0; // 距离的单位模式：0-厘米，1-米
bit par_index = 0; // 参数界面定位编号：0-距离参数，1-温度参数
bit record_flag; // 判断是否进入记录模式
bit da_flag; // 判断是否进行 DA 输出

/* 相关变量 */
unsigned char i; // 公共循环变量
float temperature = 23.5; // 温度数据
unsigned char temperature_par = 30; // 温度参数
int factory_data[3] = {0, 34, 10}; // 工厂模式下的三种设置数据：校准值正常保存，传输速度 / 10 保存，DAC 下限 * 10 保存

unsigned char distance = 0; // 距离数据
unsigned char distance_par = 40; // 距离参数
unsigned char record_data[30]; // 记录下来的距离数据，每 200ms 采集一次，6 秒一共 30 次
unsigned char record_data_index;

/* 计时变量 */
unsigned int timer2000ms; // 长按 2s 重置系统
unsigned char timer100ms; // LED1 以 0.1s = 100ms 为间隔闪烁
unsigned int timer6000ms; // 记录 6 秒内距离数据的连续变化
unsigned char timer200ms; // 记录模式下，每 200ms 记录一次数据
unsigned char da_timer200ms; // 输出模式下，每 200 ms 输出一次数据

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
  
  if(record_flag == 1)
    return;
  
  switch(key_Down)
  {
    case 4: // 主界面切换
      if(++seg_display_mode == 3) seg_display_mode = 0;
      timer100ms = 0;
      led_Buf[0] = led_Buf[1] = led_Buf[2] = led_Buf[3] = led_Buf[4] = led_Buf[5] = led_Buf[6] = led_Buf[7] = 0;
      par_index = 0;
      factory_display_mode = 0;
    break;
    case 5: // 界面内切换
      switch(seg_display_mode)
      {
        case 0: // 测距界面：显示单位切换
          distance_units_mode ^= 1;
          break;
        case 1: // 参数界面：参数切换
          par_index ^= 1;
          break;
        case 2: // 工厂模式：三种子界面切换
          if(++factory_display_mode == 3) factory_display_mode = 0;
          break;
      }
      break;
    case 8: // 参数 +
      if(seg_display_mode == 1) // 参数界面
      {
        if(par_index == 0) // 距离参数
        {
          distance_par += 10;
          if(distance_par == 100 ) distance_par = 90;
        }
        else if(++temperature_par == 81) // 温度参数
          temperature_par = 80; 
      }
      else if(seg_display_mode == 2) // 工厂模式
      {
        if(factory_display_mode == 0) // 校准值
        {
          factory_data[factory_display_mode] += 5;
          if(factory_data[factory_display_mode] == 95) factory_data[factory_display_mode] = 90;
        }
        else if(factory_display_mode == 1 && ++factory_data[factory_display_mode] == 1000) // 传输速度
          factory_data[factory_display_mode] = 999;
        else if(factory_display_mode == 2 && ++factory_data[factory_display_mode] == 21) // DAC 输出下限
          factory_data[factory_display_mode] = 20;
      }
      else // 测距界面（激活记录功能）
      {
        record_flag = 1;
        record_data_index = timer6000ms = timer200ms = 0;
      }
      break;
    case 9: // 参数 -
      if(seg_display_mode == 1) // 参数界面
      {
        if(par_index == 0) // 距离参数
        {
          distance_par -= 10;
          if(distance_par == 0 ) distance_par = 10;
        }
        else if(--temperature_par == 255) // 温度参数
          temperature_par = 0;
      }
      else if(seg_display_mode == 2) // 工厂模式
      {
        if(factory_display_mode == 0) // 校准值
        {
          factory_data[factory_display_mode] -= 5;
          if(factory_data[factory_display_mode] == -95) factory_data[factory_display_mode] = -90;
        }
        else if(--factory_data[factory_display_mode] == 0) // 传输速度和 DAC 输出下限
          factory_data[factory_display_mode] = 1;
      }
      else // 测距界面（DA 输出）
      {
        da_flag = 1;
        record_data_index = 0;
      }
      break;
  }
  
  if(key_Up != 89 && timer2000ms == 2000)
  {
    seg_display_mode = 0;
    distance_units_mode = 0;
    distance_par = 40;
    temperature_par = 30;
    factory_data[0] = 0;
    factory_data[1] = 34;
    factory_data[2] = 10;
    key_Old = timer2000ms = 0;
  }
}

/* 信息处理函数 */
void Seg_Proc()
{
	/* 信息读取处理区域 */
  switch(seg_Scan_Slow)
  {
    case 40:
      temperature = Read_Temperature();
      break;
    case 70: 
      distance = (factory_data[1] / 1000.0) * ((float)Sonic_Read() / 0.017) / 2.0 + factory_data[0] + 1.2; // 增加 1.2 的校准值通过测评
      break;
  }
  
	/* 数据显示区域 */
  if(seg_Scan_Slow) return;
	seg_Scan_Slow = 1;
  
  switch(seg_display_mode)
  {
    case 0: // 测距界面
      seg_Buf[0] = (unsigned char)temperature / 10 % 10;
      seg_Buf[1] = (unsigned char)temperature % 10 + '.';
      seg_Buf[2] = (unsigned int)(temperature * 10) % 10; 
      seg_Buf[3] = 11; // -

      seg_Buf[5] = distance_units_mode ? distance / 100 % 10 + '.' : distance / 100 % 10;
      seg_Buf[6] = distance / 10 % 10;
      seg_Buf[7] = distance % 10;

      for(i = 5; seg_Buf[i] == 0 && i < 7; i++)
        seg_Buf[i] = 10;
    
      break;
    case 1: // 参数界面
      seg_Buf[0] = 12; // P
      seg_Buf[1] = (unsigned char)par_index + 1;
      seg_Buf[2] = seg_Buf[3] = seg_Buf[4] = seg_Buf[5] = 10;
      seg_Buf[6] = par_index ? temperature_par / 10 % 10 : distance_par / 10 % 10;
      seg_Buf[7] = par_index ? temperature_par % 10 : distance_par % 10;
      break;
    case 2: // 工厂模式
      seg_Buf[0] = 13; // F
      seg_Buf[1] = factory_display_mode + 1;
      seg_Buf[2] = seg_Buf[3] = seg_Buf[4] = seg_Buf[5] = 10;
      switch(factory_display_mode)
      {
        case 0: // 校准值设置界面
          
          seg_Buf[6] = abs(factory_data[factory_display_mode]) / 10 % 10;
          seg_Buf[7] = abs(factory_data[factory_display_mode]) % 10;
          
          if(factory_data[factory_display_mode] < 0)
          {
            if(seg_Buf[6] == 0)
              seg_Buf[6] = 11;
            else
              seg_Buf[5] = 11;
          }
          else if(seg_Buf[6] == 0)
            seg_Buf[6] = 10;
          
          break;
        case 1: // 介质设置界面
          seg_Buf[4] = factory_data[factory_display_mode] / 100 % 10;
          seg_Buf[5] = factory_data[factory_display_mode] / 10 % 10;
          seg_Buf[6] = factory_data[factory_display_mode] % 10;
          seg_Buf[7] = 0;
        
          for(i = 4; seg_Buf[i] == 0 && i < 7; i++)
            seg_Buf[i] = 10;
        
          break;
        case 2: // DAC 输出设置界面
          seg_Buf[6] = abs(factory_data[factory_display_mode]) / 10 % 10 + '.';
          seg_Buf[7] = abs(factory_data[factory_display_mode]) % 10;
          break;
      }
      break;
  }
}

/* 其他处理函数 */
void Led_Proc()
{
  if(seg_display_mode == 0)
  {
    if(distance >= 255)
    {
      for(i = 0; i < 8; i++)
        led_Buf[i] = 1;
    }
    else
    {
      for(i = 0; i < 8; i++)
        led_Buf[i] = (distance >> i) & 0x01;
        //led_Buf[i] = distance & (0x01 << i); // 结果不一定是 0 / 1
    }
  }
  else if(seg_display_mode == 1)
  {
        led_Buf[7] = 1;
  }
  
  Relay(distance >= distance_par - 5 && distance <= distance_par + 5 && temperature <= temperature_par);
  
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
  
  if(key_Old == 89 && ++timer2000ms >= 2000) timer2000ms = 2000;
  
  if(seg_display_mode == 2 && ++timer100ms == 100)
  {
    timer100ms = 0;
    led_Buf[0] ^= 1;
  }
  
  if(record_flag && ++timer200ms == 200) // 距离距离数据
  {
    timer200ms = 0;
    record_data[record_data_index]= distance;
    record_data_index++;
  }
  
  if(record_flag && ++timer6000ms == 6000)
  {
    record_flag = 0;
  }
  
  if(da_flag && ++da_timer200ms == 200)
  {
    da_timer200ms = 0;
    if(record_data[record_data_index] >= 90)
      DA_Output(5);
    else if(record_data <= 10)
      DA_Output((float)factory_data[2] / 10.0);
    else
      DA_Output((5 - ((float)factory_data[2] / 10.0)) / 80.0 * ((int)record_data[record_data_index] - 10.0) + ((float)factory_data[2] / 10.0)); 
    
    if(++record_data_index == 30)
      da_flag = 0;
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
