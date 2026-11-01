#include <STC15F2K60S2.H>
#include "stdio.h"
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "iic.h"
//按键
idata unsigned char key_val,key_old,key_down,key_up;
idata unsigned char key_slow_down;

//数码管
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down;
idata unsigned char seg_disp_mode=0;//0-数据界面 1-参数设置界面 2-计数界面
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata unsigned int time_5s;//led 5s计时器
idata bit L1_flag;//当前电压小于电压参数标志位 1-小于
//电压值
idata unsigned char ad_slow_down;
idata unsigned int V_100x;//电压值100倍
idata unsigned int V_old_100x;//上一次的电压值100倍
idata unsigned char V_para_10x;//电压参数值10倍 0-50
//计数
idata unsigned int count=0;
idata unsigned int error_key_count;//无效按键操作次数
void key_proc()
{
	if(key_slow_down<10) return;
	key_slow_down=0;
	key_val=key_read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	if(key_down==12)
	{
		switch(seg_disp_mode)
		{
			case 0:
				seg_disp_mode=1;
				break;
			case 1:
				eeprom_write(&V_para_10x,0,1);
				seg_disp_mode=2;
				break;
			case 2:
				seg_disp_mode=0;
				break;
		}
	}
	
	if(key_down==13)
	{
		if(seg_disp_mode==2)
		{
			error_key_count=0;
			count=0;
		}
		else
			error_key_count++;
	}
	
		if(key_down==16)
		{
			if(seg_disp_mode==1)
			{
				error_key_count=0;
				if(V_para_10x>=50) //如果已经达到5v，那么再按下按键，回到0v
					V_para_10x=0;
				else //否则+0.5v
					V_para_10x+=5;
			}
			else
				error_key_count++;
		}
		
		if(key_down==17)
		{
			if(seg_disp_mode==1)
			{
				error_key_count=0;
				if(V_para_10x==0) //如果按下按键前已经达到0v，那么按下按键，回到5v
					V_para_10x=50;
				else
					V_para_10x-=5;
			}
			else
				error_key_count++;
		}
}

void seg_proc()
{
	if(seg_slow_down<80) return;
	seg_slow_down=0;

	switch(seg_disp_mode)
	{
		case 0: //数据界面
			seg_buf[0]=11;
			seg_buf[1]=seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
			seg_buf[5]=V_100x/100%10+',';
			seg_buf[6]=V_100x/10%10;
			seg_buf[7]=V_100x %10;
			break;
		case 1: //参数设置界面
			seg_buf[0]=12;
			seg_buf[1]=seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
			seg_buf[5]=V_para_10x/10%10+',';
			seg_buf[6]=V_para_10x%10;
			seg_buf[7]=0;			
			break;
		case 2: //计数界面
			seg_buf[0]=13;
			seg_buf[1]=(count>=1000000)?count/1000000%10:10;
			seg_buf[2]=(count>=100000)?count/100000%10:10;
			seg_buf[3]=(count>=10000)?count/10000%10:10;
			seg_buf[4]=(count>=1000)?count/1000%10:10;
			seg_buf[5]=(count>=100)?count/100%10:10;;
			seg_buf[6]=(count>=10)?count/10%10:10;
			seg_buf[7]=count %10;				
			break;
	}
}

void led_proc()
{
	if(V_100x<V_para_10x*10&&time_5s>5000)
	{
		L1_flag=1;
	}
	else
	{
		L1_flag=0;
	}
	ucled[0]=L1_flag;
	ucled[1]=(count%2!=0);
	ucled[2]=(error_key_count>=3);
}

void get_ad()
{
	if(ad_slow_down<100) return;
	ad_slow_down=0;
	V_old_100x=V_100x;
	V_100x=(ad_read(0x03)/51.0)*100;
	if(V_old_100x>V_para_10x*10 && V_100x<V_para_10x*10)
	{
		count++;
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
	EA  = 1;        //使能全局中断
}

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
	if(V_100x<V_para_10x*10)
	{
		if(++time_5s==5000) time_5s=5001;
	}
	else
	{
		time_5s=0;
	}
		
	led_disp(ucled);
}
void main()
{
	system_init();
	Timer1_Init();
	ad_read(0x03);
	eeprom_read(&V_para_10x,0,1);
	while(1)
	{
		key_proc();
		seg_proc();
		led_proc();
		get_ad();
	}
}
