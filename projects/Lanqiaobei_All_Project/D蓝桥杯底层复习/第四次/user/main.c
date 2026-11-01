#include <STC15F2K60S2.H>
#include "stdio.h"
#include "string.h"
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "ds1302.h"
#include "onewire.h"
#include "iic.h"
#include "ultrasound.h"
#include "uart.h"
//频率
idata unsigned int freq;
idata unsigned int freq_time_1s;
//串口
idata bit uart_rx_flag;
idata unsigned char uart_rx_tick;
pdata unsigned char uart_rx_buf[]={0,0,0,0,0,0,0,0,0,0};
idata unsigned char uart_rx_index;
//按键
idata unsigned char key_val,key_old,key_down,key_up;
idata unsigned char key_slow_down;
//数码管
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_pos;
idata unsigned char seg_slow_down;
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
//ad
idata unsigned char ad_slow_down;
//测距
idata unsigned char dist_slow_down;
//温度
idata unsigned int T_slow_down;
void key_proc()
{
	if(key_slow_down<10) return;
	key_slow_down=0;
	key_val=key_read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
}

void seg_proc()
{
	if(seg_slow_down<80) return;
	seg_slow_down=0;
}

void led_proc()
{


}

void ad_proc()
{
	if(ad_slow_down<100) return;
	ad_slow_down=0;

}

void temperature_proc()
{
	if(T_slow_down<300) return;
	T_slow_down=0;
}

void distance_proc()
{
	if(dist_slow_down<100) return;
	dist_slow_down=0;
}

void uart_proc()
{
	unsigned char x,y;
	if(uart_rx_index==0) return;
	if(uart_rx_tick>=10)
	{
		uart_rx_tick=0;
		uart_rx_flag=0;
		if(sscanf(uart_rx_buf,"(%bu,%bu)",&x,&y)==2)
		{
			printf("Got it!");
		}
		else
		{
			printf("Error");
		}
		memset(uart_rx_buf,0,10);
		uart_rx_index=0;
	}

}
void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			//设置定时器模式 为P34引脚读取外部脉冲模式
	TL0 = 0x00;				//设置定时初始值
	TH0 = 0x00;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
}



void Timer1_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	ET1 = 1;				//使能定时器1中断
	EA = 1;         //使能全局中断
}

void Timer1_Isr(void) interrupt 3
{
	key_slow_down++;
	seg_slow_down++;
	ad_slow_down++;
	dist_slow_down++;
	T_slow_down++;
	if(seg_buf[seg_pos]>20)
		seg_disp(seg_pos,seg_buf[seg_pos]-',',1);
	else
		seg_disp(seg_pos,seg_buf[seg_pos],0);
	seg_pos=(++seg_pos)%8;
	if(++freq_time_1s==1000)
	{
		freq_time_1s=0;
		freq=TH0<<8|TL0;
		TH0=TL0=0;
	}
	if(uart_rx_flag) uart_rx_tick++;
	led_disp(ucled);
}

void Uart1_Isr(void) interrupt 4
{
	if (RI)				//检测串口1接收中断
	{
		uart_rx_flag=1;
		uart_rx_tick=0;
		if(uart_rx_index<10)
		{
			uart_rx_buf[uart_rx_index++]=SBUF;
		}
		else
		{
			uart_rx_index=0;
			memset(uart_rx_buf,0,10);
			uart_rx_flag=0;
			uart_rx_tick=0;
		}
		RI = 0;			//清除串口1接收中断请求位
	}
}
void main()
{
	system_init();
	while(rd_temperature()==85.0);
	ad_read(0x01);
	Timer0_Init();
	Uart1_Init();
	Timer1_Init();
	while(1)
	{
		key_proc();
		seg_proc();
		led_proc();
		ad_proc();
		temperature_proc();
		distance_proc();
		uart_proc();
	}

}
