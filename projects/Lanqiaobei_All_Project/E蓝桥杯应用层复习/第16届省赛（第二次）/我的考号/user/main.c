#include <STC15F2K60S2.H>
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "onewire.h"
#include "iic.h"
#include "ultrasound.h"
//按键
idata unsigned char key_val,key_old,key_up,key_down;
idata unsigned char key_slow_down;
//数码管
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down;
idata unsigned char seg_disp_mode;//0-测距 1-参数
idata unsigned char range_disp_mode;//0-首次上电/未执行任何有效测量 1-单次 2-累加
idata bit para_disp_mode;//0-时间参数设置 1-测量次数设置
//温度
idata unsigned char T_val;
idata unsigned int T_slow_down;
//测距
idata unsigned int dist_val;
idata unsigned int dist_sum;
idata bit signal_flag;//单次测距标志位 0-测距完成 1-测距进行中
idata bit summation_flag;//多次测距标志位 0-测距完成 1-测距进行中
idata unsigned char summation_index;//统计测距的次数标志位
//参数
pdata unsigned char time_apart[4]={2,4,6,4};
idata unsigned char time_apart_index;
pdata unsigned char dist_count[4]={3,5,7,5};
idata unsigned char dist_count_index;
idata float speed;
idata unsigned int dist_test_timer;//测距时间计时器
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata bit led_flash_flag;
idata unsigned char led_flash_tick;
void do_distance()
{
	unsigned int temp;
	if(signal_flag)
	{
		temp=ut_wave_data(speed);
		dist_val=temp;
		signal_flag=0;
	}
	else if(summation_flag)
	{
		temp=ut_wave_data(speed);
		dist_sum+=temp;
		summation_index++;
		if(summation_index>=dist_count[dist_count_index])
		{
			summation_flag=0;
			summation_index=0;
		}
	}
}
void key_proc()
{
	if(key_slow_down<10) return;
	key_slow_down=0;
	key_val=key_read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	if(seg_disp_mode==0)
	{
		if(key_down==4)
		{
			signal_flag=1;
			dist_val=0;
			range_disp_mode=1;
			
			summation_flag=0;
			summation_index=0;
			dist_sum=0;
			do_distance();
		}
		
		if(key_down==5)
		{
			summation_flag=1;
			summation_index=0;
			dist_sum=0;
			dist_test_timer=0;
			range_disp_mode=2;
			
			signal_flag=0;
			dist_val=0;
			do_distance();
		}
	}
	if(summation_flag==0)
	{
		if(key_down==8)
		{
			if(seg_disp_mode==0)
			{
				seg_disp_mode=1;
				para_disp_mode=0;
			}
			else
			{
				if(para_disp_mode==0)
					para_disp_mode=1;
				else
					seg_disp_mode=0;
			}
		}
		if(key_down==12)
		{
			if(seg_disp_mode==0)
			{
				dist_val=0;
				dist_sum=0;
				range_disp_mode=0;
			}
		}
	}
	if(key_down==9)
	{
		if(seg_disp_mode==1)
		{
			if(para_disp_mode==0)
				time_apart_index=(+time_apart_index)%4;
			else
				dist_count_index=(++dist_count_index)%4;
		}
	}
}

void seg_proc()
{
	if(seg_slow_down<80) return;
	seg_slow_down=0;
	switch(seg_disp_mode)
	{
		case 0:
			seg_buf[1]=12;
			seg_buf[2]=T_val/10%10;
			seg_buf[3]=T_val%10;
			if(range_disp_mode==0)
			{
				seg_buf[0]=12;
				seg_buf[4]=seg_buf[5]=seg_buf[6]=seg_buf[7]=10;			
			}
			else if(range_disp_mode==1)
			{
				seg_buf[0]=11;
				seg_buf[4]=(dist_val>=1000)?dist_val/1000%10:10;
				seg_buf[5]=(dist_val>=100)?dist_val/100%10:10;
				seg_buf[6]=(dist_val>=10)?dist_val/10%10:10;
				seg_buf[7]=dist_val%10;						
			}
			else
			{
				seg_buf[0]=13;
				seg_buf[4]=(dist_sum>=1000)?dist_sum/1000%10:10;
				seg_buf[5]=(dist_sum>=100)?dist_sum/100%10:10;
				seg_buf[6]=(dist_sum>=10)?dist_sum/10%10:10;
				seg_buf[7]=dist_sum%10;						
			}
			break;
		case 1:
			seg_buf[0]=14;
			seg_buf[2]=seg_buf[3]=seg_buf[4]=seg_buf[5]=10;
			if(para_disp_mode==0)
			{
				seg_buf[1]=1;
				seg_buf[6]=time_apart[time_apart_index]/10%10;
				seg_buf[7]=time_apart[time_apart_index]%10;
			}
			else
			{
				seg_buf[1]=2;
				seg_buf[6]=dist_count[dist_count_index]/10%10;
				seg_buf[7]=dist_count[dist_count_index]%10;				
			}
			break;
	}
}

void led_proc()
{
	ucled[0]=(seg_disp_mode==0);
	switch(range_disp_mode)
	{
		case 0:
			ucled[1]=0;
			break;
		case 1:
			if(signal_flag==0)
				ucled[1]=(dist_val<20||dist_val>100);
			break;
		case 2:
			if(summation_flag==0)
				ucled[1]=(dist_sum<20||dist_sum>100);
			break;
	}
	ucled[2]=led_flash_flag;
}

void get_temperature()
{
	float temp;
	if(T_slow_down<300) return;
	T_slow_down=0;
	temp=rd_temperature();
	T_val=(unsigned char)temp;
	speed=(float)(330+0.6*temp);
}

//累加间隔时间测距
void dist_proc()
{
	if(summation_flag==1&&dist_test_timer>=(unsigned int)time_apart[time_apart_index]*1000)
	{
		dist_test_timer=0;
		do_distance();
	}
}
void da_proc()
{
	float temp=0;
	switch(range_disp_mode)
	{
		case 0:
			temp=0;
			break;
		case 1:
			if(signal_flag==0)
			{
				if(dist_val>=100) temp=5;
				else if(dist_val>20) temp=(float)dist_val/20.0;
				else temp=1;
			}
			break;
		case 2:
			if(summation_flag==0)
			{
				if(dist_sum>=100) temp=5;
				else if(dist_sum>20) temp=(float)dist_sum/20.0;
				else temp=1;
			}			
			break;
	}
	da_write(temp*51);

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
	T_slow_down++;
	seg_disp(seg_pos,seg_buf[seg_pos]);
	seg_pos=(++seg_pos)%8;
	if(summation_flag)
	{
		dist_test_timer++;
		if(++led_flash_tick==100)
		{
			led_flash_tick=0;
			led_flash_flag=!led_flash_flag;
		}
	}
	else
	{
		led_flash_tick=0;
		led_flash_flag=0;
		dist_test_timer=0;
	}
	led_disp(ucled);
}
void main()
{
	system_init();
	while(rd_temperature()==85.0);
	Timer1_Init();
	while(1)
	{
		key_proc();
		seg_proc();
		led_proc();
		get_temperature();
		dist_proc();
		da_proc();
	}
}