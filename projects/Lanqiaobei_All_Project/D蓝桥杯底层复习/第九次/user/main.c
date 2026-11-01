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
idata bit freq_danwei;//0-hz 1-khz
//串口
idata bit uart_rx_flag;
idata unsigned char uart_rx_tick;
idata unsigned char uart_rx_buf[]={0,0,0,0,0,0,0,0,0,0};
idata unsigned char uart_rx_index;
//距离
idata unsigned int current_location[2];
idata unsigned int destination[2];
idata bit got_it_flag;
idata unsigned int obstacle_distance;
idata unsigned char range_disp_mode;//0-空闲 1-运行 2-等待
idata unsigned char dist_val;
idata unsigned char motion_state;//0-静止 1-徘徊 2-跑动
//按键
idata unsigned char key_val,key_old,key_down,key_up;
idata unsigned char key_slow_down;
//数码管
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down;
idata unsigned char seg_disp_mode;
//电压
idata unsigned int V_val_100x;
//dac
idata unsigned char dac_data_10x;//0.1-2.0 (1-20)
//校准值
idata unsigned int jiaozhun_para;//-900 - 900
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
	switch(seg_disp_mode)
	{
		case 0://常规高位熄灭 15届国赛
			seg_buf[0]=11;
			seg_buf[1]=3;
			seg_buf[2]=10;
			seg_buf[3]=(obstacle_distance>=10000)?obstacle_distance/10000%10:10;
			seg_buf[4]=(obstacle_distance>=1000)?obstacle_distance/1000%10:10;
			seg_buf[5]=(obstacle_distance>=100)?obstacle_distance/100%10:10;
			seg_buf[6]=(obstacle_distance>=10)?obstacle_distance/10%10:10;
			seg_buf[7]=obstacle_distance%10;
			break;
		case 1://没说高位熄灭直接显示 16届省赛（第一套）
			seg_buf[0]=12;
			seg_buf[1]=motion_state+1;
			seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
			seg_buf[5]=dist_val/100%10;
			seg_buf[6]=dist_val/10%10;
			seg_buf[7]=dist_val%10;
			break;
		case 2://单位转换 13届国赛
			seg_buf[0]=13;
			seg_buf[1]=0;
			if(freq_danwei==0)
			{
				seg_buf[2]=(freq>=100000)?freq/100000%10:10;
				seg_buf[3]=(freq>=10000)?freq/10000%10:10;
				seg_buf[4]=(freq>=1000)?freq/1000%10:10;
				seg_buf[5]=(freq>=100)?freq/100%10:10;
				seg_buf[6]=(freq>=10)?freq/10%10:10;
				seg_buf[7]=freq%10;
			}
			else
			{
				seg_buf[2]=10;
				seg_buf[3]=10;			
				seg_buf[4]=(freq>=100000)?freq/100000%10:10;
				seg_buf[5]=(freq>=10000)?freq/10000%10:10;
				seg_buf[6]=freq/1000%10+',';
				seg_buf[7]=freq/100%10;
			}
			break;
		case 3://小数点 两位 一位 11届省赛（第一套）14届国赛
			seg_buf[0]=14;
			seg_buf[1]=V_val_100x/100%10+',';
			seg_buf[2]=V_val_100x/10%10;
			seg_buf[3]=V_val_100x%10;
			seg_buf[4]=15;
			seg_buf[5]=15;
			seg_buf[6]=dac_data_10x/10%10+',';
			seg_buf[7]=dac_data_10x%10;
			break;
		case 4: //负数 15届省赛第一套
			seg_buf[0]=16;
			seg_buf[1]=2;
			seg_buf[2]=seg_buf[3]=10;
			if(jiaozhun_para>=0)
			{
				seg_buf[4]=10;
				seg_buf[5]=(jiaozhun_para>=100)?jiaozhun_para/100%10:10;
				seg_buf[6]=(jiaozhun_para>=10)?jiaozhun_para/10%10:10;
				seg_buf[7]=jiaozhun_para%10;
			}
			else
			{
				if((-jiaozhun_para)>=100)
				{
					seg_buf[4]=15;
					seg_buf[5]=(-jiaozhun_para)/100%10;
					seg_buf[6]=(-jiaozhun_para)/10%10;
					seg_buf[7]=(-jiaozhun_para)%10;					
				}
				else if((-jiaozhun_para)>=10)
				{
					seg_buf[4]=10;
					seg_buf[5]=15;
					seg_buf[6]=(-jiaozhun_para)/10%10;
					seg_buf[7]=(-jiaozhun_para)%10;						
				}
				else
				{
					seg_buf[4]=10;
					seg_buf[5]=10;
					seg_buf[6]=15;
					seg_buf[7]=(-jiaozhun_para)%10;						
				}
			}
			break;
	}

}


void led_proc()
{


}

void ad_proc()
{


}

void da_proc()
{


}

void temperature_proc()
{

}

void dist_proc()
{

}

void uart_proc()
{
	unsigned int x,y;
	int result,num;
	if(uart_rx_index==0) return;
	if(uart_rx_tick>=10)
	{
		uart_rx_tick=0;
		uart_rx_flag=0;
		if(uart_rx_buf[0]=='(' && uart_rx_buf[uart_rx_index-1]==')')
		{
			result=sscanf(uart_rx_buf,"(%u,%u)%n",&x,&y,&num);
			if(result==2&&num==strlen(uart_rx_buf))
			{
				if(range_disp_mode==0)
				{
					destination[0]=x;
					destination[1]=y;
					got_it_flag=1;
					printf("Got it");
				}
				else if(range_disp_mode==1)
				{
					printf("Busy");
				}
				else
				{
					printf("Wait");
				}
			}
			else
			{
				printf("Error");
			}
		}
		else if(strcmp(uart_rx_buf,"?")==0)
		{
				if(range_disp_mode==0)
					printf("Idle");
				else if(range_disp_mode==1)
					printf("Busy");
				else
					printf("Wait");		
		}
		else if(strcmp(uart_rx_buf,"#")==0)
		{
			printf("(%u,%u)",current_location[0],current_location[1]);
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
	TMOD |= 0x05;			//设置定时器模式
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
	EA = 1;
}

void Timer1_Isr(void) interrupt 3
{
	key_slow_down++;
	seg_slow_down++;
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
			memset(uart_rx_buf,0,10);
			uart_rx_index=0;
			uart_rx_flag=0;
			uart_rx_tick=0;
		}
		RI = 0;			//清除串口1接收中断请求位
	}
}
void main()
{
	system_init();
	Timer0_Init();
	Uart1_Init();
	Timer1_Init();
	while(1)
	{
		key_proc();
		seg_proc();
		led_proc();
		ad_proc();
		da_proc();
		temperature_proc();
		dist_proc();
		uart_proc();
	}
}
