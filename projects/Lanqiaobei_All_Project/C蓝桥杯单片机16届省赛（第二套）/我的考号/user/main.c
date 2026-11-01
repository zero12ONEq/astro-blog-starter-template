#include <STC15F2K60S2.H>
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "onewire.h"
#include "iic.h"
#include "ultrasound.h"
//按键
idata unsigned char key_val,key_old,key_down,key_up;
idata unsigned char key_slow_down;
//数码管
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down;
idata unsigned char seg_disp_mode=0;//0-测距界面 1-参数界面
idata unsigned char dist_test_flag=2;//0-单次测量 1-累加测量 2-熄灭
idata unsigned char dist_test_flag_old;//上一次的测距模式
idata bit para_disp_mode;//0-时间设置 1-测量次数设置
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata unsigned char led_flash_tick;//100ms
idata bit led_flash_flag;
//温度
idata unsigned char T_val; 
idata unsigned int T_slow_down;
//测距
idata unsigned char dist_val;//单次测量时的数据
idata unsigned char dist_many_val;//多次测量时，每一次的测量数据
idata unsigned int dist_sum;
idata bit finish_flag;//测距完成标志位
//参数
pdata unsigned char time_apart[4]={2,4,6,4};//时间间隔 单位s
idata unsigned int dist_test_timer;//测距时间间隔计时器
idata unsigned char time_apart_index=0;
pdata unsigned char dist_count[4]={3,5,7,5};//测量次数
idata unsigned char dist_count_index=0;
idata unsigned char dist_count_val;
idata float speed;
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
			dist_test_flag=0;
			finish_flag=0;
			dist_val=0;
			dist_count_val=0;
		}
		if(key_down==5)
		{
			dist_test_flag=1;
			dist_count_val=dist_count[dist_count_index];
			finish_flag=0;
			dist_many_val=0;
			dist_sum=0;
			
			dist_many_val=ut_wave_data(speed);
			dist_sum+=dist_many_val;
			dist_count_val--;
		}
	}
	
	if(dist_count_val==0)
	{
		if(key_down==8)
		{
			if(seg_disp_mode==0)
			{
				seg_disp_mode=1;
				para_disp_mode=0;
			}
			else if(seg_disp_mode==1&&para_disp_mode==0)
			{
				seg_disp_mode=1;
				para_disp_mode=1;
			}
			else if(seg_disp_mode==1&&para_disp_mode==1)
			{
				seg_disp_mode=0;
			}
		}
		if(key_down==12)
		{
			if(seg_disp_mode==0)
			{
				dist_test_flag_old=dist_test_flag;
				dist_test_flag=2;
				if(dist_test_flag_old==0)
				{
					dist_val=0;
					finish_flag=0;
				}
				else if(dist_test_flag_old==1)
				{
					dist_many_val=0;
					dist_sum=0;
					finish_flag=0;
				}
			}
		}
	}
	if(key_down==9)
	{
		if(seg_disp_mode==1)
		{
			if(para_disp_mode==0)
			{
				time_apart_index=(++time_apart_index)%4;
			}
			else
			{
				dist_count_index=(++dist_count_index)%4;
			}					
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
			if(dist_test_flag==0)
			{
				seg_buf[0]=11;
				seg_buf[4]=(dist_val>=1000)?dist_val/1000%10:10;
				seg_buf[5]=(dist_val>=100)?dist_val/100%10:10;
				seg_buf[6]=(dist_val>=10)?dist_val/10%10:10;
				seg_buf[7]=dist_val%10;
			}
			else if(dist_test_flag==1)
			{
				seg_buf[0]=13;
				seg_buf[4]=(dist_sum>=1000)?dist_sum/1000%10:10;
				seg_buf[5]=(dist_sum>=100)?dist_sum/100%10:10;
				seg_buf[6]=(dist_sum>=10)?dist_sum/10%10:10;
				seg_buf[7]=dist_sum%10;
			}
			else
			{
				seg_buf[0]=12;
				seg_buf[4]=10;
				seg_buf[5]=10;
				seg_buf[6]=10;
				seg_buf[7]=10;				
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
	unsigned int dist_temp;
	ucled[0]=(seg_disp_mode==0);
	if(finish_flag)
	{
		dist_temp=(dist_test_flag==0)?dist_val:dist_sum;
		ucled[1]=(dist_temp<20||dist_temp>100);
	}
	if(dist_test_flag==2)
	{
		ucled[1]=0;
	}
	if(dist_test_flag==1&&finish_flag==0)
	{
		ucled[2]=led_flash_flag;
	}
	if(dist_test_flag==1&&finish_flag==1)
	{
		ucled[2]=0;
	}
}

void get_temperature()
{
	float temp;
	if(T_slow_down<300) return;
	T_slow_down=0;
	temp=rd_temperature();
	T_val=(unsigned char)temp;
	speed=330+(0.6*temp);
}


void dist_proc()
{
	if(finish_flag) return;
	if(dist_test_flag==0)
	{
		dist_val=ut_wave_data(speed);
		finish_flag=1;
	}
	
	if(dist_test_flag==1&&dist_test_timer>=time_apart[time_apart_index]*1000)
	{
			dist_test_timer=0;
			dist_count_val--;
			dist_many_val=ut_wave_data(speed);
			dist_sum+=dist_many_val;
			if(dist_count_val==0)
			{
				finish_flag=1;
			}
	}
}

void da_proc()
{
	unsigned int dist_temp;
	float da_temp;
	if(finish_flag)
	{		
		dist_temp=(dist_test_flag==0)?dist_val:dist_sum;
		if(dist_temp>100)
			da_write(5*51);
		else if(dist_temp>=20)
		{
			da_temp=(4.0/80.0)*(dist_temp-20)+1;
			da_write(da_temp*51);
		}
		else
		{
			da_write(1*51);
		}
	}
	else
	{
		da_write(0*51);
	}
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
	if(dist_test_flag==1)
	{
		dist_test_timer++;
	}
	else
	{
		dist_test_timer=0;
	}
	if(dist_test_flag==1&&dist_count_val!=0)
	{
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
