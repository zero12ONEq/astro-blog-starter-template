#include <STC15F2K60S2.H>
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "onewire.h"
#include "ds1302.h"
//按键
idata unsigned char key_val,key_old,key_down,key_up;
idata unsigned char key_slow_down;
idata bit key_long_flag;//按键17长按标志位
//数码管
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down;
idata unsigned char seg_disp_mode=0;//0-温度显示界面 1-时间显示界面 2-参数设置界面

//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata unsigned char led_time_100ms;//led闪烁计时器
idata bit led_flash_flag;//led闪烁标志位
idata bit relay_flag;//继电器吸合标志位
//温度
idata unsigned int T_val_10x;//温度10倍
idata unsigned char T_para=23;//10-99
idata unsigned int temp_slow_down;
//时间
pdata unsigned char ucrtc[]={12,59,55};
//模式控制
idata bit model_flag=0;//0-温度控制模式 1-时间控制模式
idata unsigned int time_5s;//继电器，led5s计时器
idata bit time_flag;//整点标志位

void key_proc()
{
	if(key_slow_down<10) return;
	key_slow_down=0;
	key_val=key_read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	if(key_down==12)
		seg_disp_mode=(++seg_disp_mode)%3;//0,1,2之间循环
	if(key_down==13)
		model_flag=!model_flag;
	if(seg_disp_mode==2)
	{
		if(key_down==16)
		{
			if(T_para<99) T_para++;
		}
		if(key_down==17)
		{
			if(T_para>10) T_para--;
		}
	}
	if(seg_disp_mode==1)
	{
		if(key_old==17)
		{
			key_long_flag=1;
		}
		if(key_up==17)
		{
			key_long_flag=0;
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
			seg_buf[0]=11;
			seg_buf[1]=1;
			seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
			seg_buf[5]=T_val_10x/100%10;
			seg_buf[6]=T_val_10x/10%10+',';
			seg_buf[7]=T_val_10x%10;
			break;
		case 1:
			seg_buf[0]=11;
			seg_buf[1]=2;
			seg_buf[2]=10;
			seg_buf[5]=12;
			if(key_long_flag==0)
			{
				seg_buf[3]=ucrtc[0]/10%10;
				seg_buf[4]=ucrtc[0]%10;
				seg_buf[6]=ucrtc[1]/10%10;
				seg_buf[7]=ucrtc[1]%10;	
			}	
			else
			{
				seg_buf[3]=ucrtc[1]/10%10;
				seg_buf[4]=ucrtc[1]%10;
				seg_buf[6]=ucrtc[2]/10%10;
				seg_buf[7]=ucrtc[2]%10;					
			}				
			break;
		case 2:
			seg_buf[0]=11;
			seg_buf[1]=3;
			seg_buf[2]=seg_buf[3]=seg_buf[4]=seg_buf[5]=10;
			seg_buf[6]=T_para/10%10;
			seg_buf[7]=T_para%10;			
			break;
	}
}

void led_relay_proc()
{
	if(ucrtc[1]==0&&ucrtc[2]==0)//整点时，标志位置为1
	{
		time_flag=1;
		time_5s=0;  //初始化5s计时器
	}
	if(model_flag==0)//温度控制模式
	{
		if(T_val_10x>(T_para*10))
		{
			relay(1);//开发板上的L10指示灯点亮，就表示继电器吸合
			relay_flag=1;
		}
		else
		{
			relay(0);
			relay_flag = 0;  
		}
	}
	else //时间控制模式
	{		
			if(time_flag)
			{
				relay(1);
				relay_flag=1;
			}
			else
			{
				relay(0);
				relay_flag = 0;  //继电器断开，就清零
			}
	}
	
	ucled[0]=time_flag;
	ucled[1]=(!model_flag);
	ucled[2]=led_flash_flag;
}

void get_temperature()
{
	idata float temp;
	if(temp_slow_down<300) return;
	temp_slow_down=0;
	temp=rd_temperature();
	T_val_10x=(unsigned int)(temp*10);
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
	EA = 1;        //使能全局中断
}

void Timer1_Isr(void) interrupt 3
{
	key_slow_down++;
	seg_slow_down++;
	temp_slow_down++;
	if(seg_buf[seg_pos]>20)
		seg_disp(seg_pos,seg_buf[seg_pos]-',',1);
	else
		seg_disp(seg_pos,seg_buf[seg_pos],0);
	seg_pos=(++seg_pos)%8;
	
	if(time_flag)
	{
		if(++time_5s==5000)
		{
			time_5s=0;
			time_flag=0;		
		}
	}

	if(relay_flag)
	{
		if(++led_time_100ms==100) 
		{
			led_time_100ms=0;
			led_flash_flag=!led_flash_flag;
		}
	}
	else
	{
		led_time_100ms=0;
		led_flash_flag=0;
	}
	led_disp(ucled);
}

void main()
{
	system_init();
	Timer1_Init();
	while(rd_temperature()==85.0);
	set_rtc(ucrtc);
	while(1)
	{
		key_proc();
		seg_proc();
		led_relay_proc();
		get_temperature();
	}
}
