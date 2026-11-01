#include <STC15F2K60S2.H>
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "led.h"
#include "init.h"
#include "seg.h"
#include "key.h"
#include "ds1302.h"
#include "onewire.h"
#include "iic.h"
#include "ultrasound.h"
#include "uart.h"
//频率
idata unsigned int freq;
idata unsigned int freq_time_1s;
//串口
bit uart_rx_flag;
idata unsigned char uart_rx_tick;
pdata unsigned char uart_rx_buf[]={0,0,0,0,0,0,0,0,0,0};
idata unsigned char uart_rx_index;
//按键
idata unsigned char key_val,key_old,key_down,key_up;
idata unsigned char key_slow_down;
//数码管
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down;
idata unsigned char state_disp_mode;
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
//ad
idata unsigned char ad_slow_down;
//温度
idata unsigned int T_slow_down;
//dist
idata unsigned char dist_slow_down;
idata unsigned int speed_10x;
idata unsigned char R_10x;//1.0-2.0(10-20)
idata char B_para;//-90 ~90
pdata unsigned int start_location[2]={0};
pdata unsigned int current_location[2]={0};
pdata unsigned int destination[2]={0};
idata unsigned int move_time;
idata unsigned char cnt_move_time;
idata unsigned char ctrl_slow_down;
idata unsigned char barrier_distance;
idata float speed_x;
idata float speed_y;
bit reach_flag;
bit got_freq_flag;
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


void T_proc()
{
	if(T_slow_down<300) return;
	T_slow_down=0;

}

void refresh_speed()
{
	float temp_speed_10x;
	temp_speed_10x=((3.14*(R_10x/10.0)*freq)/100.0+B_para)*10;
	speed_10x=(temp_speed_10x<0)?0:(unsigned int)temp_speed_10x;
}

void reset_movement()
{
	float dist_x,dist_y,dist;
	start_location[0]=current_location[0];
	start_location[1]=current_location[1];
	cnt_move_time=0;
	move_time=0;
	dist_x=(float)destination[0]-(float)current_location[0];
	dist_y=(float)destination[1]-(float)current_location[1];
	dist=sqrt(dist_x*dist_x+dist_y*dist_y);
	if(dist>0)
	{
		speed_x=(speed_10x/10.0)*dist_x/dist;
		speed_y=(speed_10x/10.0)*dist_y/dist;
	}
	else
	{
		speed_x=0;
		speed_y=0;
	}
}

void ctrl_proc()
{
	float new_x,new_y;
	float time;
	if(ctrl_slow_down<200) return;
	ctrl_slow_down=0;
	if(state_disp_mode==1)
	{
		if(barrier_distance<30)
		{
			state_disp_mode=2;
			return;
		}
		time=move_time*0.1;
		new_x=(float)start_location[0]+speed_x*time;
		new_y=(float)start_location[1]+speed_y*time;
		if(speed_x>=0)
		{
			if(new_x>=(float)destination[0])
				new_x=(float)destination[0];	
		}
		else
		{
			if(new_x<=(float)destination[0])
				new_x=(float)destination[0];				
		}
		if(speed_y>=0)
		{
			if(new_y>=(float)destination[1])
				new_y=(float)destination[1];	
		}
		else
		{
			if(new_y<=(float)destination[1])
				new_y=(float)destination[1];				
		}		
		current_location[0]=(unsigned int)(new_x+0.5f);
		current_location[1]=(unsigned int)(new_y+0.5f);
		if(current_location[0]==destination[0]&&current_location[1]==destination[1])
		{
			state_disp_mode=0;
			reach_flag=1;
		}
	}
}

void speed_update_proc()
{
	unsigned int speed_old_10x;
	if(got_freq_flag==0) return;
	got_freq_flag=0;
	speed_old_10x=speed_10x;
	refresh_speed();
	if(state_disp_mode==1&&speed_10x!=speed_old_10x)
	{
		reset_movement();
	}

}
void dist_proc()
{
	float temp;
	if(dist_slow_down<100) return;
	dist_slow_down=0;
	temp=speed_10x/10.0;
	barrier_distance=ut_wave_data(temp);
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
		result=sscanf(uart_rx_buf,"(%u,%u)%n",&x,&y,&num);
		if(result==2&&num==strlen(uart_rx_buf))
		{
			printf("(%u,%u)",x,y);
		}		
		else
		{
			printf("Error");
		}
		uart_rx_index=0;
		memset(uart_rx_buf,0,10);
	}
}

void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			//设置定时器模式为P34引脚读取外部脉冲模式
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
	T_slow_down++;
	dist_slow_down++;
	ctrl_slow_down++;
	if(seg_buf[seg_pos]>=20)
		seg_disp(seg_pos,seg_buf[seg_pos]-',',1);
	else
		seg_disp(seg_pos,seg_buf[seg_pos],0);
	seg_pos=(++seg_pos)%8;
	if(++freq_time_1s==1000)
	{
		freq_time_1s=0;
		freq=TH0<<8|TL0;
		got_freq_flag=1;
		TH0=TL0=0;
	}
	if(state_disp_mode==1)
	{
		if(++cnt_move_time==100)
		{
			cnt_move_time=0;
			move_time++;
		}
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
			uart_rx_buf[uart_rx_index++]=SBUF;
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
	Timer0_Init();
	Uart1_Init();
	Timer1_Init();
	while(1)
	{
		key_proc();
		seg_proc();
		led_proc();
		ad_proc();
		T_proc();
		dist_proc();
		uart_proc();
	}
}