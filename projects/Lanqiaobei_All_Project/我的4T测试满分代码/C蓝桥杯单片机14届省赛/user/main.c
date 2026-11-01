#include <STC15F2K60S2.H>
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "onewire.h"
#include "ds1302.h"
#include "iic.h"
//频率
idata unsigned int freq_time_1s;
idata unsigned int freq;
//数码管
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down;
idata unsigned char seg_disp_mode;//0-时间界面 1-回显界面 2-参数界面 3-温湿度界面
idata unsigned char back_disp_mode;//回显界面子界面 0-温度回显 1-湿度回显 2-时间回显
pdata unsigned char seg_disp_mode_old[2];//0主界面 1回显界面子界面
//按键
idata unsigned char key_val,key_old,key_down,key_up;
idata unsigned char key_slow_down;
idata bit key_long_flag;//按键长按标志位
idata unsigned int key_long_tick;//按键2s长按计时器
//时间
pdata unsigned char ucrtc[]={13,3,5};

//温度
idata unsigned char T_val;//当前温度
idata unsigned char T_max;//最大温度
idata unsigned int T_average_10x;//平均温度10倍
idata unsigned int T_sum=0;//温度之和
idata unsigned char T_para=30;//温度参数 0-99
//湿度
idata unsigned char W_val;//当前湿度
idata unsigned char W_max;//最大湿度
idata unsigned int W_average_10x;//平均湿度10倍
idata unsigned int W_sum=0.0;//湿度之和
idata bit W_error_flag;//采集到的湿度数据无效标志位 1-无效 0-有效

pdata float old_TW_data[2];//前一次采集到的数据 0温度 1湿度
//触发
idata unsigned char trigger_count=0;//采集功能累计触发的次数
pdata unsigned char trigger_rtc[3];//最近一次触发数据采集功能的时间
idata bit trigger_flag;//采集功能触发标志位
idata unsigned int time_3s;//3s计时器
//光照读取
idata float  light_u_test;//光敏电阻分压采集
idata bit light_flag_old;//前一次的亮暗状态标志位 1-亮 0-暗
idata bit light_flag;//这一次的亮暗状态标志位 1-亮 0-暗
idata unsigned char ad_slow_down;
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata unsigned char led_flash_tick;//led闪烁0.1s计时器、
idata bit led_flash_flag;//led闪烁标志位
//按键处理函数
void key_proc()
{
	if(key_slow_down<10) return;
	key_slow_down=0;
	key_val=key_read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	if(seg_disp_mode!=3) //不处于温湿度界面按下按键才有效
	{
		if(key_down==4)
		{
			seg_disp_mode=(++seg_disp_mode)%3;//0-1-2之间循环
			back_disp_mode=0;//每次从时间界面进入回显界面前，把子界面置为温度回显界面
		}
		if(seg_disp_mode==1)
		{
			if(key_down==5)
				back_disp_mode=(++back_disp_mode)%3;//0-1-2之间循环
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
						trigger_rtc[0]=trigger_rtc[1]=trigger_rtc[2]=0;
					}
					key_long_flag=0;
					key_long_tick=0;
				}
			}
		}
		if(seg_disp_mode==2)
		{
			if(key_down==8)
				if(T_para<99) T_para++;
			if(key_down==9)
				if(T_para>0) T_para--;
		}
	}
}

//信息处理函数
void seg_proc()
{
	if(seg_slow_down<80) return;
	seg_slow_down=0;
	read_rtc(ucrtc);//读取当前时间，为下面数码管显示时间做铺垫
	
//数码管
	switch(seg_disp_mode)
	{
		case 0: //时间
			seg_buf[0]=ucrtc[0]/10%10;
			seg_buf[1]=ucrtc[0]%10;
			seg_buf[2]=11;
			seg_buf[3]=ucrtc[1]/10%10;
			seg_buf[4]=ucrtc[1]%10;
			seg_buf[5]=11;
			seg_buf[6]=ucrtc[2]/10%10;
			seg_buf[7]=ucrtc[2]%10;	
			break;
		case 1: //回显
			if(back_disp_mode==0) //温度回显
			{
				seg_buf[0]=12;
				seg_buf[1]=10;
				if(trigger_count!=0)
				{
					seg_buf[2]=T_max/10%10;
					seg_buf[3]=T_max%10;
					seg_buf[4]=11;
					seg_buf[5]=T_average_10x/100%10;
					seg_buf[6]=T_average_10x/10%10+',';
					seg_buf[7]=T_average_10x%10;
				}
				else
				{
					seg_buf[2]=seg_buf[3]=seg_buf[4]=seg_buf[5]=seg_buf[6]=seg_buf[7]=10;					
				}
			}
			else if(back_disp_mode==1) //湿度回显
			{
				seg_buf[0]=13;
				seg_buf[1]=10;
				if(trigger_count!=0)
				{
					seg_buf[2]=W_max/10%10;
					seg_buf[3]=W_max%10;
					seg_buf[4]=11;
					seg_buf[5]=W_average_10x/100%10;
					seg_buf[6]=W_average_10x/10%10+',';
					seg_buf[7]=W_average_10x%10;
				}	
				else
				{
					seg_buf[2]=seg_buf[3]=seg_buf[4]=seg_buf[5]=seg_buf[6]=seg_buf[7]=10;	
				}					
			}
			else //时间回显
			{
				seg_buf[0]=14;
				seg_buf[1]=trigger_count/10%10;
				seg_buf[2]=trigger_count%10;
				if(trigger_count!=0)
				{
					seg_buf[3]=trigger_rtc[0]/10%10;
					seg_buf[4]=trigger_rtc[0]%10;
					seg_buf[5]=11;
					seg_buf[6]=trigger_rtc[1]/10%10;
					seg_buf[7]=trigger_rtc[1]%10;	
				}
				else
				{
					seg_buf[3]=seg_buf[4]=seg_buf[5]=seg_buf[6]=seg_buf[7]=10;					
				}
			}
			break;
		case 2: //参数
			seg_buf[0]=15;
			seg_buf[1]=seg_buf[2]=seg_buf[3]=seg_buf[4]=seg_buf[5]=10;
			seg_buf[6]=T_para/10%10;
			seg_buf[7]=T_para%10;				
			break;
		case 3: //温湿度
			seg_buf[0]=16;
			seg_buf[1]=seg_buf[2]=10;
			seg_buf[3]=T_val/10%10;
			seg_buf[4]=T_val%10;
			seg_buf[5]=11;
			if(W_error_flag==0)
			{
				seg_buf[6]=W_val/10%10;
				seg_buf[7]=W_val%10;	
			}	
			else
			{
				seg_buf[6]=seg_buf[7]=17;					
			}
			break;
	
	}
}

//其他模块处理函数
void led_proc()
{
	ucled[0]=(seg_disp_mode==0);
	ucled[1]=(seg_disp_mode==1);
	ucled[2]=(seg_disp_mode==3);
	if(T_val>T_para)
		ucled[3]=led_flash_flag;
	else
		ucled[3]=0;//温度实际值低于测量值，熄灭
	
	ucled[4]=W_error_flag;
	ucled[5]=(trigger_count>=2&&(T_val>old_TW_data[0])&&(W_val>old_TW_data[1]));

}

void get_ad()
{
	if(ad_slow_down<100) return;
	ad_slow_down=0;
	ad_read(0x01);
	light_u_test=ad_read(0x01)/51.0;

	light_flag_old=light_flag;//先把上一次的亮暗状态赋值出去
	if(light_u_test>4.0)
		light_flag=1; //亮状态
	else
		light_flag=0; //暗状态
	//不在温湿度界面并且从亮变为暗，触发采集功能
	if(seg_disp_mode!=3&&(light_flag_old==1&&light_flag==0))
		trigger_flag=1;
	else
		trigger_flag=0;
}

void trigger_proc()
{
	float temp;
	if(!trigger_flag) return;//如果不触发，直接返回		
	old_TW_data[0]=T_val; //把上一次的温度赋值出去
	temp=rd_temperature();//读取新的温度
	T_val=(unsigned char)temp;
	if(freq>=200&&freq<=2000) //判定湿度数据是否有效 如果有效
	{
		W_error_flag=0;//湿度数据无效标志位置为0
		trigger_count++;//更新采集次数
		read_rtc(trigger_rtc);//读取触发采集的当前时间
		//更新最大温度，平均温度
		if(T_val>T_max)
			T_max=T_val;
		T_sum+=T_val;//计算温度之和
		T_average_10x=((float)T_sum/(float)trigger_count)*10;		
		//更新最新湿度数据，更新最大湿度，平均湿度
		old_TW_data[1]=W_val;
		W_val=10+(2.0/45.0)*(freq-200);;
		W_sum+=W_val;
		if(W_val>W_max)
			W_max=W_val;		
		W_average_10x=((float)W_sum/(float)trigger_count)*10;
	}
	else //如果无效
	{
		W_error_flag=1; //无效标志位置为1
	}
	//触发采集功能 储存当前显示界面，立刻跳转温湿度界面
	seg_disp_mode_old[0]=seg_disp_mode;
	seg_disp_mode_old[1]=back_disp_mode;
	seg_disp_mode=3;
	time_3s=0;          //初始化3s计时器
	trigger_flag = 0;  // 处理完清除标志

}
//定时器0的初始化函数
void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			//设置定时器模式为计数器模式
	TL0 = 0x00;				//设置定时初始值
	TH0 = 0x00;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
}

//定时器1的初始化函数
void Timer1_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	ET1 = 1;				//使能定时器1中断
	EA = 1;					//使能全局中断
}

//定时器1的服务中断函数
void Timer1_Isr(void) interrupt 3
{
	key_slow_down++;
	seg_slow_down++;
	ad_slow_down++;
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
	if(seg_disp_mode==3&&++time_3s==3000)
	{
		time_3s=0;
		seg_disp_mode=seg_disp_mode_old[0];
		back_disp_mode=seg_disp_mode_old[1];
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
		led_flash_tick=0;
		led_flash_flag=0;
	}
	led_disp(ucled);
}
void main()
{
	system_init();
	set_rtc(ucrtc);
	Timer0_Init();
	Timer1_Init();
	while(1)
	{
		key_proc();
		seg_proc();
		led_proc();
		get_ad();
		trigger_proc();
	}

}

