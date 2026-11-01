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
#include "seg.h"		  // 数码管显示模块
#include "key.h"		  // 按键处理模块
#include "led.h"
#include "init.h" // 系统初始化模块

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

// PWM控制相关变量
idata unsigned char pwm_period = 10; // PWM周期(1ms)
idata unsigned char pwm_compare = 1; // PWM占空比(10%)
void Key_Proc()
{
	// 检查延时是否到达
	if (Key_Slow_Down < 10)
		return;
	Key_Slow_Down = 0;

	// 读取按键值并计算按下/松开事件
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Val ^ Key_Old); // 检测按键按下事件
	Key_Up = ~Key_Val & (Key_Val ^ Key_Old);  // 检测按键松开事件
	Key_Old = Key_Val;						  // 保存当前按键值作为下次检测的旧值

	// 按键8处理(调整PWM占空比)
	if (Key_Down == 8)
		pwm_compare = (++pwm_compare) % 10; // 循环调整PWM占空比(0-9)
}

/* 数码管处理函数 */
void Seg_Proc()
{
	if (Seg_Slow_Down < 50)
		return;
	Seg_Slow_Down = 0;
	Seg_Buf[0] = 10;
	Seg_Buf[1] = 10;
	Seg_Buf[2] = 10;
	Seg_Buf[3] = 10;
	Seg_Buf[4] = 10;
	Seg_Buf[5] = 10;
	Seg_Buf[6] = 10;
	Seg_Buf[7] = pwm_compare;
}
void Led_Proc()
{
	ucLed[0] = 1;
	ucLed[1] = 1;
	ucLed[2] = 1;
	ucLed[3] = 1;
	ucLed[4] = 1;
	ucLed[5] = 1;
	ucLed[6] = 1;
	ucLed[7] = 1;
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

	// PWM输出处理
	if (++pwm_period >= 10)
		pwm_period = 0; // 周期计数(0-9)
	if (pwm_period < pwm_compare)
		Led_Disp(ucLed); // PWM高电平期间点亮LED
	else
		Led_Off(); // PWM低电平期间熄灭LED
}

/*主函数*/
void main()
{
	System_Init(); // 系统初始化
	Timer0_Init(); // 定时器0初始化（系统计时用）
	while (1)
	{
		Led_Proc();
		Key_Proc();
		Seg_Proc();
	}
}