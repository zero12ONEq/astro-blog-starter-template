#include "Init.h"

void System_Init()
{
	P0 = 0xFF;
	P2 = P2 & 0x1F | 0x80;
	P2 &= 0x1F;
	 
	P0 = 0x00; // 关闭蜂鸣器
	P2 = P2 & 0x1F | 0xA0;
	P2 &= 0x1F;
}


unsigned char temp = 0x00;
unsigned char temp_old = 0xFF;

void Buzz(unsigned char enable) // 蜂鸣器
{
	if(enable)
		temp |= 0x40;
	else
		temp &= ~0x40;
	
	if(temp != temp_old)
	{
		P0 = temp;
		P2 = P2 & 0x1F | 0xA0;
		P2 &= 0x1F;
		temp_old = temp;
	}
}

void Relay(unsigned char enable) // 继电器
{
	if(enable)
		temp |= 0x10;
	else
		temp &= ~0x10;
	
	if(temp != temp_old)
	{
		P0 = temp;
		P2 = P2 & 0x1F | 0xA0;
		P2 &= 0x1F;
		temp_old = temp;
	}
}

/* ======================================= NE555 计数器配置 ===================================================================================== */

///* 计数器 0 初始化函数 */
//void Timer0_Init(void)
//{
//	AUXR &= 0x7F;			//定时器时钟12T模式
//	TMOD &= 0xF0;			//设置定时器模式
//	TMOD |= 0x05;			//设置为计数器，不自动重装载模式
//	TH0 = TL0 = 0x00;		//设置定时初始值
//	TF0 = 0;				//清除TF0标志
//	TR0 = 1;				//计数器0开始计数
//}

// 使用 NE555 时要注意将按键的 P34 部分以及最后一列屏蔽，不然会导致频率出现误差 

///* 定时器 1 中断处理函数 */
//void Timer1_Server() interrupt 3
//{
//	if(++key_Scan_Slow == 10) key_Scan_Slow = 0;
//	if(++seg_Scan_Slow == 100) seg_Scan_Slow = 0;
//	
//	if(++timer1000ms == 1000)
//	{
//		timer1000ms = 0;
//		freq = (TH0 << 8) | TL0;
//		TH0 = TL0 = 0x00;
//	}
//	
//	if(++seg_Pos == 8) seg_Pos = 0;
//	if(seg_Buf[seg_Pos] > 20) // 判断有无小数点，ASCII 为 46
//		Seg_Display(seg_Pos, seg_Buf[seg_Pos] - '.', 1);
//	else
//		Seg_Display(seg_Pos, seg_Buf[seg_Pos], 0);
//	
//	Led_Display(led_Buf, 1);
//	uart_recv_tick++;		
//}


	
