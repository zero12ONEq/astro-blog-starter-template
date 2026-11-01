#include <STC15F2K60S2.H>
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "iic.h"
#include "ultrasound.h"
//按键
idata unsigned char key_val,key_old,key_down,key_up;
idata unsigned char key_slow_down;
//数码管
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down;
idata unsigned char seg_disp_mode=0;//0-电压界面 1-参数界面 2-测距界面
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata bit led_flash_flag;
idata unsigned char led_flash_tick;
//ad
idata unsigned char ad_slow_down;
//距离
idata unsigned char dist_slow_down;
idata unsigned char dist_val;
idata bit dist_flag;//测距标志位 1-启动测距 0-未启动测距
//电压
idata unsigned int V_val_100x;
idata bit para_flag;//0-上限参数 1-下限参数
idata unsigned char V_para_up_10x=45;//上限参数十倍 最大值为50（5v）最小值为5（0.5v）
idata unsigned char V_para_down_10x=5;//下限参数十倍 最大值为50（5v）最小值为5（0.5v）
idata unsigned char V_para_ct_up_10x;//上限参数十倍 最大值为50（5v）最小值为5（0.5v）
idata unsigned char V_para_ct_down_10x;//下限参数十倍 最大值为50（5v）最小值为5（0.5v）
void key_proc()
{
	if(key_slow_down<10) return;
	key_slow_down=0;
	key_val=key_read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	if(key_down==4)
	{
		if(seg_disp_mode==0)
		{
			seg_disp_mode=2;
		}
		else if(seg_disp_mode==2)
		{
			V_para_ct_up_10x=V_para_up_10x;
			V_para_ct_down_10x=V_para_down_10x;
			para_flag=0;	
			seg_disp_mode=1;
		}
		else
		{
			V_para_up_10x=V_para_ct_up_10x;
			V_para_down_10x=V_para_ct_down_10x;
			seg_disp_mode=0;
		}
	}
	if(seg_disp_mode==1)
	{
		if(key_down==5)
		{
			para_flag=!para_flag;
		}
		if(key_down==6)
		{
			if(para_flag==0)
			{
				if(V_para_ct_up_10x>=50)
					V_para_ct_up_10x=5;
				else
					V_para_ct_up_10x+=5;
			}
			else
			{
				if(V_para_ct_down_10x>=50)
					V_para_ct_down_10x=5;
				else
					V_para_ct_down_10x+=5;				
			}
		}
		if(key_down==7)
		{
			if(para_flag==0)
			{
				if(V_para_ct_up_10x<=5)
					V_para_ct_up_10x=50;
				else
					V_para_ct_up_10x-=5;
			}
			else
			{
				if(V_para_ct_down_10x<=5)
					V_para_ct_down_10x=50;
				else
					V_para_ct_down_10x-=5;				
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
			seg_buf[0]=11;
			seg_buf[1]=seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
			seg_buf[5]=V_val_100x/100%10+',';
			seg_buf[6]=V_val_100x/10%10;
			seg_buf[7]=V_val_100x%10;
			break;
		case 1:
			seg_buf[0]=12;
			seg_buf[1]=seg_buf[2]=10;
			seg_buf[3]=V_para_ct_up_10x/10%10+',';
			seg_buf[4]=V_para_ct_up_10x%10;
			seg_buf[5]=10;
			seg_buf[6]=V_para_ct_down_10x/10%10+',';
			seg_buf[7]=V_para_ct_down_10x%10;			
			break;
		case 2:
			seg_buf[0]=13;
			seg_buf[1]=seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
			if(dist_flag)
			{
				seg_buf[5]=(dist_val>=100)?dist_val/100%10:10;
				seg_buf[6]=(dist_val>=10)?dist_val/10%10:10;
				seg_buf[7]=dist_val%10;
			}
			else
			{
				seg_buf[5]=seg_buf[6]=seg_buf[7]=14;
			}
			break;
	}
}


void ad_proc()
{
	float temp;
	if(ad_slow_down<100) return;
	ad_slow_down=0;
	ad_read(0x43);
	temp=ad_read(0x43)/51.0;
	V_val_100x=(unsigned int)(temp*100);
}

void dist_proc()
{
	if(dist_slow_down<100) return;
	dist_slow_down=0;
	if(V_para_down_10x*10<V_val_100x&&V_val_100x<V_para_up_10x*10)
	{
		dist_flag=1;
		dist_val=ut_wave_data();
	}
	else
	{
		dist_flag=0;
		dist_val=0;
	}

}
void da_proc()
{
	float temp;
	if(dist_flag==0) 
	{
		da_write(0*51);
	}
	else
	{
		if(dist_val>=80) temp=5;
		else if(dist_val>20) temp=(4.0/60.0)*(dist_val-20)+1.0;
		else temp=1;
		da_write(temp*51);
	}
}
void led_proc()
{
	switch(seg_disp_mode)
	{
		case 0://0-电压界面 1-参数界面 2-测距界面
			ucled[0]=1;
			ucled[1]=0;
			ucled[2]=0;
			break;
		case 1:
			ucled[0]=0;
			ucled[1]=0;
			ucled[2]=1;			
			break;
		case 2:
			ucled[0]=0;
			ucled[1]=1;
			ucled[2]=0;
			break;
	}
	ucled[7]=led_flash_flag;

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
	if(seg_buf[seg_pos]>20)
		seg_disp(seg_pos,seg_buf[seg_pos]-',',1);
	else
		seg_disp(seg_pos,seg_buf[seg_pos],0);
	seg_pos=(++seg_pos)%8;
	if(dist_flag)
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
	Timer1_Init();
	while(1)
	{
		key_proc();
		seg_proc();
		ad_proc();
		dist_proc();
		da_proc();
		led_proc();
	}
}
