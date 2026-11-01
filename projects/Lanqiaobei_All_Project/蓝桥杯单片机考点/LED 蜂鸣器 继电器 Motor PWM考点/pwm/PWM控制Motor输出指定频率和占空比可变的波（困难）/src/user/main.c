/***************************************************************************************
 * Copyright (C) 2025 米醋电子工作室
 *
 * 版本         ：V4T
 * 作者         ：米醋电子工作室
 * 描述         ：该版本为V2版本的改进版，针对4T平台做了专项优化，修复了此前版本的若干Bug
 *
 * 版权归米醋电子工作室所有，并保留所有权力。
 * 未经明确书面许可，任何个人或机构不得擅自用于商业用途。
 ***********************************************************************************************/

/***********************************************************************
 * 文件名: main.c
 * 功能描述: STC15F2K60S2单片机多功能控制程序，包含以下功能模块:
 *   - LED显示控制
 *   - 数码管显示(时间、温度、距离、AD值、频率)
 *   - 按键处理
 *   - I2C通信(用于AD/DA转换)
 *   - 单总线通信(用于温度传感器)
 *   - DS1302实时时钟
 *   - 串口通信
 *   - 超声波测距
 *   - PWM控制
 ***********************************************************************/

#include <STC15F2K60S2.H> // STC15F2K60S2单片机头文件
#include "led.h"		  // LED显示模块
#include "seg.h"		  // 数码管显示模块
#include "key.h"		  // 按键处理模块
#include "uart.h"		  // 串口通信模块
#include "init.h"		  // 系统初始化模块

#include "stdio.h"	 // 标准输入输出
#include "string.h"	 // 字符串处理
#include "intrins.h" // 单片机内部函数

/* ==================== 全局变量定义区 ==================== */

// LED显示缓冲区，存储8个LED的状态(0:亮，1:灭)
pdata unsigned char ucLed[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// 数码管显示缓冲区，存储8位数码管显示的值(0-9为数字，10为不显示)
pdata unsigned char Seg_Buf[8] = {10, 10, 10, 10, 10, 10, 10, 10};
idata unsigned char Seg_Pos = 0;   // 当前数码管扫描位置(0-7)
idata unsigned char Seg_Slow_Down; // 数码管显示刷新延时计数器

// 按键相关变量
idata unsigned char Key_Val, Key_Old, Key_Down, Key_Up; // 按键当前值、旧值、按下标志、松开标志
idata unsigned char Key_Slow_Down;						// 按键扫描延时计数器

/* 数据 */
idata unsigned int Sent_Freq = 1000;  // 发送频率1000~10000
idata unsigned char Duty_Cycle = 10;  // 10~90
idata unsigned int Timer_Count = 0;	  // 用于计时的计数器
idata unsigned int High_Time = 0;	  // 高电平持续时间
idata unsigned int Low_Time = 0;	  // 低电平持续时间
idata unsigned char signal_level = 0; // 当前信号电平

void Update_PWM_Timing(void);

void Key_Proc()
{
	if (Key_Slow_Down < 10)
		return;
	Key_Slow_Down = 0;
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Old ^ Key_Val); // 按键按下检测
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);  // 按键抬起检测
	Key_Old = Key_Val;

	if (Key_Down == 10) // 频率调整按键
	{
		Sent_Freq = (Sent_Freq == 10000) ? 1000 : Sent_Freq + 1000;
		Update_PWM_Timing();
	}

	if (Key_Down == 11) // 占空比调整按键
	{
		Duty_Cycle = (Duty_Cycle == 90) ? 10 : Duty_Cycle + 10;
		printf("%bu%%", Duty_Cycle);
		Update_PWM_Timing();
	}
}

/* 数码管处理函数 */
void Seg_Proc()
{
	if (Seg_Slow_Down < 50)
		return;
	Seg_Slow_Down = 0;
	Seg_Buf[0] = 10;
	Seg_Buf[1] = 10;
	Seg_Buf[2] = 10; // 空白
	Seg_Buf[3] = (Sent_Freq / 10000 == 0) ? 10 : 1;
	Seg_Buf[4] = Sent_Freq / 1000 % 10;
	Seg_Buf[5] = 0;
	Seg_Buf[6] = 0;
	Seg_Buf[7] = 0;
}

/* 更新PWM时序参数 */
void Update_PWM_Timing(void)
{
	unsigned long period_us = 1000000UL / Sent_Freq;	   // 周期(微秒)
	High_Time = ((period_us * Duty_Cycle) / 100 + 5) / 10; // 高电平时间(微秒)
	Low_Time = (period_us - High_Time * 10) / 10;		   // 低电平时间(微秒)

	// 立即更新状态，确保平滑过渡
	TR1 = 0;		  // 暂停定时器1
	Motor(0);		  // 先将输出置为低电平
	signal_level = 0; // 初始状态为低电平
	Timer_Count = 0;  // 计数器清零
	TR1 = 1;		  // 重新启动定时器1
}

void Timer0_Init(void) // 1毫秒@12.000MHz
{
	AUXR &= 0x7F; // 定时器时钟12T模式
	TMOD &= 0xF0; // 设置定时器模式
	TL0 = 0x18;	  // 设置定时初始值
	TH0 = 0xFC;	  // 设置定时初始值
	TF0 = 0;	  // 清除TF0标志
	TR0 = 1;	  // 定时器0开始计时
	ET0 = 1;	  // 使能定时器0中断
	EA = 1;		  // 全局中断使能
}

void Timer0_Isr(void) interrupt 1
{
	// 各模块减速计数器递增
	Key_Slow_Down++;
	Seg_Slow_Down++;
	Seg_Pos = (++Seg_Pos) % 8; // 更新数码管位置

	// 数码管显示处理
	if (Seg_Buf[Seg_Pos] > 20)
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1); // 带小数点
	else
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0); // 无小数点
}

/* 定时器1初始化，10微秒@12.000MHz */
void Timer1_Init(void) // 10微秒@12.000MHz
{
	AUXR &= 0xBF; // 定时器时钟12T模式
	TMOD &= 0x0F; // 设置定时器模式
	TL1 = 0xF6;	  // 设置定时初始值
	TH1 = 0xFF;	  // 设置定时初始值
	TF1 = 0;	  // 清除TF1标志
	TR1 = 1;	  // 定时器1开始计时
	ET1 = 1;	  // 使能定时器1中断
}

void Timer1_Isr(void) interrupt 3
{
	// 每10us计时一次
	Timer_Count++;
	if (signal_level == 0)
	{ // 当前为低电平阶段
		if (Timer_Count >= Low_Time)
		{
			Motor(1);		  // 输出高电平
			signal_level = 1; // 切换到高电平阶段
			Timer_Count = 0;  // 重置计时器
		}
	}
	else
	{ // 当前为高电平阶段
		if (Timer_Count >= High_Time)
		{
			Motor(0);		  // 输出低电平
			signal_level = 0; // 切换到低电平阶段
			Timer_Count = 0;  // 重置计时器
		}
	}
}

/*主函数*/
void main()
{
	System_Init(); // 系统初始化
	Uart1_Init();  // 串口1初始化
	// 初始化PWM参数
	Update_PWM_Timing();

	Timer1_Init(); // 定时器1初始化（方波生成用）
	Timer0_Init(); // 定时器0初始化（系统计时用）

	while (1)
	{
		Key_Proc();
		Seg_Proc();
	}
}