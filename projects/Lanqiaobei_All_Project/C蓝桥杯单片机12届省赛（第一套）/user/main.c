#include <STC15F2K60S2.H>
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "onewire.h"
#include "iic.h"

//按键
idata unsigned char key_val,key_old,key_down,key_up;
idata unsigned char key_slow_down;

//数码管
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down;
idata unsigned char seg_disp_mode=0;//0-温度显示界面 1-参数设置界面 2-DAC输出界面
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};

//温度
idata unsigned int T_val_100x;//温度数据100倍
idata unsigned char T_para=25;//温度参数
idata unsigned char T_para_set;//温度参数设置变量
idata unsigned int temp_slow_down;
//DAC
idata unsigned int V_val_100x;//DAC输出电压值的100倍
idata bit model_flag=0;//模式标志位 0-模式1 1-模式2
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
		switch(seg_disp_mode)
		{
			case 0:
				T_para_set=T_para;//进入设置界面前把参数值赋值给参数设置值
				seg_disp_mode=1;
				break;
			case 1:
				T_para=T_para_set;//离开参数设置界面前，把设置值赋值给参数值
				seg_disp_mode=2;
				break;
			case 2:
				seg_disp_mode=0;
				break;
		}
	}
	if(seg_disp_mode==1)
	{
		if(key_down==8) 
		{
			if(T_para_set>0)
				T_para_set--;
		}
		if(key_down==9) 
		{
			if(T_para_set<99)
				T_para_set++;
		}
	}
	if(key_down==5)
		model_flag=!model_flag;
}


void seg_proc()
{
	if(seg_slow_down<80) return;
	seg_slow_down=0;

	switch(seg_disp_mode)
	{
		case 0:
			seg_buf[0]=11;
			seg_buf[1]=seg_buf[2]=seg_buf[3]=10;
			seg_buf[4]=T_val_100x/1000%10;
			seg_buf[5]=T_val_100x/100%10+',';
			seg_buf[6]=T_val_100x/10%10;
			seg_buf[7]=T_val_100x%10;
			break;
		case 1:
			seg_buf[0]=12;
			seg_buf[1]=seg_buf[2]=seg_buf[3]=seg_buf[4]=seg_buf[5]=10;
			seg_buf[6]=T_para_set/10%10;
			seg_buf[7]=T_para_set%10;
			break;
		case 2:
			seg_buf[0]=13;
			seg_buf[1]=seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
			seg_buf[5]=V_val_100x/100%10+',';
			seg_buf[6]=V_val_100x/10%10;
			seg_buf[7]=V_val_100x%10;			
			break;
	}
}


void led_proc()
{
	ucled[0]=(!model_flag);
	ucled[1]=(seg_disp_mode==0);
	ucled[2]=(seg_disp_mode==1);
	ucled[3]=(seg_disp_mode==2);
}


void get_temperature()
{
	if(temp_slow_down<300) return;
	temp_slow_down=0;
	T_val_100x=(unsigned int)(rd_temperature()*100);
	if(model_flag==0)
	{
		if(T_val_100x<T_para*100)
			V_val_100x=0;
		else
			V_val_100x=500;
	}
	else
	{
		if(T_val_100x>=4000)
			V_val_100x=400;
		else if(T_val_100x>2000)
			V_val_100x=(unsigned int)(((3.0/20.0)*((T_val_100x-2000)/100.0)+1.0)*100);
		else
			V_val_100x=100;
	}
	da_write(V_val_100x/100.0*51.0);
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
	temp_slow_down++;
	if(seg_buf[seg_pos]>20)
		seg_disp(seg_pos,seg_buf[seg_pos]-',',1);
	else
		seg_disp(seg_pos,seg_buf[seg_pos],0);
	seg_pos=(++seg_pos)%8;
	
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
	}
}
