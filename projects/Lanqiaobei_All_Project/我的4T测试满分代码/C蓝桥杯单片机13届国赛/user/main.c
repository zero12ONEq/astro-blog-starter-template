#include <STC15F2K60S2.H>
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "iic.h"
#include "ultrasound.h"
//频率
idata unsigned int freq;
idata unsigned int freq_time_1s;
idata unsigned char freq_para_10x=90;//(10-120)1.0-12.0 单位kHz
//按键
idata unsigned char key_val,key_old,key_down,key_up;
idata unsigned char key_slow_down;
idata bit key_long_flag;
idata unsigned int key_long_tick;
//数码管
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down;
idata unsigned char seg_disp_mode=0;//0-频率界面 1-湿度界面 2-测距界面 3-参数界面
idata bit freq_danwei=0;//0-Hz 1-kHz
idata bit dist_danwei=0;//0-cm 1-m
idata unsigned char para_disp_mode;//0-频率参数 1-湿度参数 2-距离参数
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata bit L1_flash_flag;
idata bit L2_flash_flag;
idata bit L3_flash_flag;
idata unsigned char led1_flash_tick;
idata unsigned char led2_flash_tick;
idata unsigned char led3_flash_tick;
//继电器
idata unsigned char relay_count;
idata bit relay_old = 0;
idata bit relay_count_dirty = 0;
//电机
idata unsigned char motor_period;//0-9
idata unsigned char motor_compare;
//ad
idata unsigned char ad_slow_down;
//超声波
idata unsigned char dist_slow_down;
idata unsigned char dist_val;
idata unsigned char dist_para_10x=6;//单位m (0.1-1.2)1-12
//湿度
idata unsigned char W_val;
idata unsigned char W_para=40;//10-60 单位：百分之
//eeprom
idata unsigned char relay_count_pre = 0;
idata unsigned char eeprom_lock = 13;
idata unsigned char eeprom_temp = 0;
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
		seg_disp_mode=(++seg_disp_mode)%4;
		para_disp_mode=0;
		led_off();
	}
	if(seg_disp_mode==3)
	{
		if(key_down==5)
		{
			para_disp_mode=(++para_disp_mode)%3;
		}
		if(key_down==6)
		{
			if(para_disp_mode==0)
			{
				if(freq_para_10x>=120) 
					freq_para_10x=10;
				else
					freq_para_10x+=5;
			}
			else if(para_disp_mode==1)
			{
				if(W_para>=60) 
					W_para=10;
				else	
					W_para+=10;
			}
			else
			{
				if(dist_para_10x>=12) 
					dist_para_10x=1;
				else
					dist_para_10x++;
			}
		}
		
		if(key_down==7)
		{
			if(para_disp_mode==0)
			{
				if(freq_para_10x<=10) 
					freq_para_10x=120;
				else
					freq_para_10x-=5;
			}
			else if(para_disp_mode==1)
			{
				if(W_para<=10) 
					W_para=60;
				else
					W_para-=10;
			}
			else
			{
				if(dist_para_10x<=1) 
					dist_para_10x=12;
				else
					dist_para_10x--;
			}
		}		
	}
	if(seg_disp_mode==2)
	{
		if(key_down==6)
			dist_danwei=!dist_danwei;
	}
	if(seg_disp_mode==0)
	{
		if(key_down==7)
			freq_danwei=!freq_danwei;
	}
	if(seg_disp_mode==1)
	{
		if(key_down==7)
		{
			key_long_flag=1;
		}
		if(key_up==7)
		{
			if(key_long_tick>=1000)
			{
				relay_count=0;
				relay_count_dirty = 1;
			}
			key_long_flag=0;
			key_long_tick=0;
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
			seg_buf[1]=10;
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
		case 1:
			seg_buf[0]=12;
			seg_buf[1]=seg_buf[2]=seg_buf[3]=seg_buf[4]=seg_buf[5]=10;
			seg_buf[6]=W_val/10%10;
			seg_buf[7]=W_val%10;
			break;
		case 2:
			seg_buf[0]=13;
			seg_buf[1]=seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
			if(dist_danwei==0)
			{
				seg_buf[5]=(dist_val>=100)?dist_val/100%10:10;
				seg_buf[6]=(dist_val>=10)?dist_val/10%10:10;
				seg_buf[7]=dist_val%10;
			}
			else
			{
				seg_buf[5]=dist_val/100%10+',';
				seg_buf[6]=dist_val/10%10;
				seg_buf[7]=dist_val%10;				
			}
			break;
		case 3:
			seg_buf[0]=14;
			if(para_disp_mode==0)
			{
				seg_buf[1]=1;
				seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
				seg_buf[5]=(freq_para_10x>=100)?freq_para_10x/100%10:10;
				seg_buf[6]=freq_para_10x/10%10+',';
				seg_buf[7]=freq_para_10x%10;
			}
			else if(para_disp_mode==1)
			{
				seg_buf[1]=2;
				seg_buf[2]=seg_buf[3]=seg_buf[4]=seg_buf[5]=10;
				seg_buf[6]=W_para/10%10;
				seg_buf[7]=W_para%10;				
			}
			else
			{
				seg_buf[1]=3;
				seg_buf[2]=seg_buf[3]=seg_buf[4]=seg_buf[5]=10;
				seg_buf[6]=dist_para_10x/10%10+',';
				seg_buf[7]=dist_para_10x%10;			
			}
			break;		
	
	}
}

void led_proc()
{
	switch(seg_disp_mode) //0-频率界面 1-湿度界面 2-测距界面 3-参数界面
	{
		case 0:
			ucled[0]=1;
			ucled[1]=0;
			ucled[2]=0;
			break;
		case 1:
			ucled[0]=0;
			ucled[1]=1;
			ucled[2]=0;
			break;
		case 2:
			ucled[0]=0;
			ucled[1]=0;			
			ucled[2]=1;
			break;
		case 3: //0-频率参数 1-湿度参数 2-距离参数
			if(para_disp_mode==0)
			{
				ucled[0]=L1_flash_flag;
				ucled[1]=0;
				ucled[2]=0;
			}
			else if(para_disp_mode==1)
			{
				ucled[1]=L2_flash_flag;
				ucled[0]=ucled[2]=0;
			}
			else
			{
				ucled[0]=ucled[1]=0;
				ucled[2]=L3_flash_flag;
			}
			break;
	}
	ucled[3]=(freq>freq_para_10x*100);
	ucled[4]=(W_val>W_para);
	ucled[5]=(dist_val>dist_para_10x*10);
}



void relay_proc()
{
	bit relay_new = (dist_val > dist_para_10x * 10);

	if(relay_new != relay_old)
	{
			relay(relay_new);
			relay_old = relay_new;
			relay_count++;
			relay_count_dirty = 1;
	}
}
		
void eeprom_proc()
{
	if(!relay_count_dirty) return;
	eeprom_write(&relay_count, 0, 1);
	relay_count_dirty = 0;
}

void motor_proc()
{
	if(freq>freq_para_10x*100)
		motor_compare=8;//占空比为8
	else
		motor_compare=2;//占空比为2
}
void ad_proc()
{
	float temp1;
	float temp2;
	if(ad_slow_down<100) return;
	ad_slow_down=0;
	ad_read(0x43);
	temp1=ad_read(0x43)/51.0;
	if(temp1>5)
		W_val=100;
	else
		W_val=(unsigned char)(20*temp1);

	if(W_val>80)
		da_write(5*51);
	else if(W_val>=W_para)
	{
		temp2=(4.0/(float)(80-W_para))*(W_val-W_para)+1.0;
		da_write(temp2*51);
	}
	else
	{
		da_write(1*51);
	}
}


void dist_proc()
{
	if(dist_slow_down<100) return;
	dist_slow_down=0;
	dist_val=ut_wave_data();
}

void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			//设置定时器模式 为P34引脚读取外部脉冲模式
	TL0 = 0x00;				//设置定时初始值
	TH0 = 0x00;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
}



void Timer1_Init(void)		//100微秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x9C;				//设置定时初始值
	TH1 = 0xFF;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	ET1 = 1;				//使能定时器1中断
	EA = 1;         //使能全局中断
}



void Timer2_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xFB;			//定时器时钟12T模式
	T2L = 0x18;				//设置定时初始值
	T2H = 0xFC;				//设置定时初始值
	AUXR |= 0x10;			//定时器2开始计时
	IE2 |= 0x04;			//使能定时器2中断
	EA = 1;           //使能全局中断
}

void Timer1_Isr(void) interrupt 3
{
	if(motor_period<motor_compare)
		motor(1);
	else
		motor(0);
	motor_period=(++motor_period)%10;	
}
void Timer2_Isr(void) interrupt 12
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
	if(++freq_time_1s==1000)
	{
		freq_time_1s=0;
		freq=TH0<<8|TL0;
		TH0=TL0=0;
	}
	if(key_long_flag)
	{
		if(++key_long_tick>=1000) key_long_tick=1001;
	}
	else
	{
		key_long_tick=0;
	}

	if(seg_disp_mode==3&&para_disp_mode==0) //0-频率参数 1-湿度参数 2-距离参数
	{
		if(++led1_flash_tick==100)
		{
			led1_flash_tick=0;
			L1_flash_flag=!L1_flash_flag;
		}
	}
	else
	{
		led1_flash_tick=0;
		L1_flash_flag=0;	
	}
	
	if(seg_disp_mode==3&&para_disp_mode==1) //0-频率参数 1-湿度参数 2-距离参数
	{
		if(++led2_flash_tick==100)
		{
			led2_flash_tick=0;
			L2_flash_flag=!L2_flash_flag;
		}
	}
	else
	{
		led2_flash_tick=0;
		L2_flash_flag=0;	
	}

	if(seg_disp_mode==3&&para_disp_mode==2) //0-频率参数 1-湿度参数 2-距离参数
	{
		if(++led3_flash_tick==100)
		{
			led3_flash_tick=0;
			L3_flash_flag=!L3_flash_flag;
		}
	}
	else
	{
		led3_flash_tick=0;
		L3_flash_flag=0;	
	}		
	led_disp(ucled);
}

void main()
{
	system_init();	
	eeprom_read(&relay_count, 0, 1);
	Timer0_Init();
	Timer1_Init();
	Timer2_Init();
	while(1)
	{
		key_proc();
		seg_proc();
		led_proc();
		dist_proc();
		relay_proc();
		eeprom_proc();
		motor_proc();
		ad_proc();
	}

}