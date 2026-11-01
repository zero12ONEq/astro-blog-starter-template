#include <STC15F2K60S2.H>
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "onewire.h"
#include "ds1302.h"
#include "iic.h"
//频率
idata unsigned int freq;
idata unsigned int freq_time_1s;
//按键
idata unsigned char key_val,key_old,key_down,key_up;
idata unsigned char key_slow_down;
idata bit key_long_flag;
idata unsigned int key_long_tick;//2s
//数码管
idata unsigned char seg_slow_down;
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_disp_mode=0;//0-时间 1-回显 2-参数 3-温湿度界面
idata unsigned char back_disp_mode;//0-温度回显 1-湿度回显 2-时间回显
pdata unsigned char seg_mode_data_old[2];//0-数码管显示界面 1-回显界面
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata bit led_flash_flag;
idata unsigned char led_flash_tick;
//时间
pdata unsigned char ucrtc[]={13,3,5};
//温度
idata unsigned char T_val;
idata unsigned int T_sum=0;
idata unsigned char T_max;
idata unsigned int T_average_10x;
idata unsigned char T_para=30;
//湿度
idata unsigned char W_val;
idata unsigned int W_sum=0;
idata unsigned char W_max;
idata unsigned int W_average_10x;
pdata unsigned char TW_old_data[2];//上一次数据 0-温度 1-湿度
//触发
idata bit trigger_flag;
idata unsigned char trigger_count=0;
pdata unsigned char trigger_time[]={21,13,0};
idata unsigned int time_3s;
idata bit error_flag;//无效数据标志位
//ad
idata unsigned char ad_slow_down;
idata bit light_flag_val;
idata bit light_flag_old;
void key_proc()
{
	if(key_slow_down<10) return;
	key_slow_down=0;
	key_val=key_read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	if(seg_disp_mode!=3)
	{
		if(key_down==4)
		{
			seg_disp_mode=(++seg_disp_mode)%3;
			back_disp_mode=0;
		}
		if(seg_disp_mode==1)
		{
			if(key_down==5)
			{
				back_disp_mode=(++back_disp_mode)%3;
			}
			if(back_disp_mode==2)
			{
				if(key_down==9)
				{
					key_long_flag=1;
				}
				if(key_up==9)
				{
					if(key_long_tick>=2000)
					{
						T_sum=W_sum=0;
						T_val=W_val=0;
						T_max=W_max=0;
						T_average_10x=W_average_10x=0;
						trigger_count=0;
						trigger_time[0]=trigger_time[1]=trigger_time[2]=0;
					}
					key_long_flag=0;
					key_long_tick=0;
				}
			}
		}
		if(seg_disp_mode==2)
		{
			if(key_down==8)
			{
				if(T_para<99) T_para++;
			}
			if(key_down==9)
			{
				if(T_para>0) T_para--;
			}
		}
	}
}

void seg_proc()
{
	if(seg_slow_down<80) return;
	seg_slow_down=0;
	read_rtc(ucrtc);
	switch(seg_disp_mode)
	{
		case 0:
			seg_buf[0]=ucrtc[0]/10%10;
			seg_buf[1]=ucrtc[0]%10;
			seg_buf[2]=11;
			seg_buf[3]=ucrtc[1]/10%10;
			seg_buf[4]=ucrtc[1]%10;
			seg_buf[5]=11;
			seg_buf[6]=ucrtc[2]/10%10;
			seg_buf[7]=ucrtc[2]%10;			
			break;
		case 1:
			if(trigger_count!=0)
			{
				if(back_disp_mode==0)
				{
					seg_buf[0]=12;
					seg_buf[1]=10;
					seg_buf[2]=T_max/10%10;
					seg_buf[3]=T_max%10;
					seg_buf[4]=11;
					seg_buf[5]=T_average_10x/100%10;
					seg_buf[6]=T_average_10x/10%10+',';
					seg_buf[7]=T_average_10x%10;				
				}
				else if(back_disp_mode==1)
				{
					seg_buf[0]=13;
					seg_buf[1]=10;
					seg_buf[2]=W_max/10%10;
					seg_buf[3]=W_max%10;
					seg_buf[4]=11;
					seg_buf[5]=W_average_10x/100%10;
					seg_buf[6]=W_average_10x/10%10+',';
					seg_buf[7]=W_average_10x%10;					
				}
				else
				{
					seg_buf[0]=14;
					seg_buf[1]=trigger_count/10%10;
					seg_buf[2]=trigger_count%10;
					seg_buf[3]=trigger_time[0]/10%10;
					seg_buf[4]=trigger_time[0]%10;
					seg_buf[5]=11;
					seg_buf[6]=trigger_time[1]/10%10;
					seg_buf[7]=trigger_time[1]%10;					
				}
			}
			else
			{
				if(back_disp_mode==0)
				{
					seg_buf[0]=12;
					seg_buf[1]=seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
					seg_buf[5]=seg_buf[6]=seg_buf[7]=10;
				}
				else if(back_disp_mode==1)
				{
					seg_buf[0]=13;
					seg_buf[1]=seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
					seg_buf[5]=seg_buf[6]=seg_buf[7]=10;					
				}
				else
				{
					seg_buf[0]=14;
					seg_buf[1]=trigger_count/10%10;
					seg_buf[2]=trigger_count%10;
					seg_buf[3]=seg_buf[4]=seg_buf[5]=seg_buf[6]=seg_buf[7]=10;					
				}			
			}
			break;
		case 2:
			seg_buf[0]=15;
			seg_buf[1]=seg_buf[2]=seg_buf[3]=seg_buf[4]=seg_buf[5]=10;	
			seg_buf[6]=T_para/10%10;
			seg_buf[7]=T_para%10;			
			break;
		case 3:
			seg_buf[0]=16;
			seg_buf[1]=seg_buf[2]=10;
			seg_buf[3]=T_val/10%10;
			seg_buf[4]=T_val%10;
			seg_buf[5]=11;	
			if(error_flag==0)
			{
				seg_buf[6]=W_val/10%10;
				seg_buf[7]=W_val%10;	
			}
			else
			{
				seg_buf[6]=17;
				seg_buf[7]=17;					
			}				
			break;
	}
}

void led_proc()
{
	ucled[0]=(seg_disp_mode==0);
	ucled[1]=(seg_disp_mode==1);
	ucled[2]=(seg_disp_mode==3);
	ucled[3]=led_flash_flag;
	ucled[4]=error_flag;
	if(T_val>TW_old_data[0]&&W_val>TW_old_data[1]&&trigger_count>=2)
	{
		ucled[5]=1;
	}
	else
	{
		ucled[5]=0;
	}
}

void ad_proc()
{
	float temp;
	if(ad_slow_down<100) return;
	ad_slow_down=0;
	temp=ad_read(0x01)/51.0;
	light_flag_old=light_flag_val;
	if(temp>4.0)
		light_flag_val=1;
	else
		light_flag_val=0;
	if(seg_disp_mode!=3&&(light_flag_old==1&&light_flag_val==0))
	{
		trigger_flag=1;
		time_3s=0;
	}	
	else
	{
		trigger_flag=0;
	}
}

void trigger_proc()
{
	float temp;
	if(trigger_flag==0) return;
	temp=rd_temperature();
	TW_old_data[0]=T_val;
	T_val=(unsigned char)temp;
	if(freq>=200&&freq<=2000)
	{
		trigger_count++;
		read_rtc(trigger_time);
		if(T_val>T_max)
			T_max=T_val;
		T_sum+=T_val;
		T_average_10x=((float)T_sum/(float)trigger_count)*10;
		TW_old_data[1]=W_val;
		W_val=10+(2.0/45.0)*(freq-200);
		W_sum+=W_val;
		if(W_val>W_max)
			W_max=W_val;
		W_average_10x=((float)W_sum/(float)trigger_count)*10;
		error_flag=0;
	}
	else
	{
		error_flag=1;
	}
	seg_mode_data_old[0]=seg_disp_mode;
	seg_mode_data_old[1]=back_disp_mode;
	seg_disp_mode=3;
	trigger_flag=0;
}

void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			//设置定时器模式，为P34引脚读取外部脉冲模式
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
	if(seg_buf[seg_pos]>=20)
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
	if(seg_disp_mode==3&&++time_3s==3000)
	{
			time_3s=0;
			seg_disp_mode=seg_mode_data_old[0];
			back_disp_mode=seg_mode_data_old[1];
	}

	if(key_long_flag)
	{
		if(++key_long_tick>=2000) key_long_tick=2001;
	}
	else
	{
		key_long_tick=0;
	}
	if(T_val>T_para)
	{
		if(++led_flash_tick==100)
		{
			led_flash_tick=0;
			led_flash_flag=!led_flash_flag;
		}
	}
	else
	{
		led_flash_flag=0;
		led_flash_tick=0;
	}
	led_disp(ucled);
}

void main()
{
	system_init();
	ad_read(0x01);
	set_rtc(ucrtc);
	while(rd_temperature()==85.0);
	Timer0_Init();
	Timer1_Init();
	while(1)
	{
		key_proc();
		seg_proc();
		led_proc();
		ad_proc();
		trigger_proc();
	}
}