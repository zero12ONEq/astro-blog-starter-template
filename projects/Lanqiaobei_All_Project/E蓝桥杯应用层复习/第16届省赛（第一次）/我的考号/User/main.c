#include <STC15F2K60S2.H>
#include "math.h"
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "onewire.h"
#include "iic.h"
#include "ultrasound.h"
//11-0xc6-C 12-0xc8-n 13-0xc7-L 14-0x8c-P
//按键
idata unsigned char key_slow_down;
idata unsigned char key_val,key_old,key_down,key_up;
idata bit key_long_flag;
idata unsigned int key_long_tick; 
//数码管
idata unsigned char seg_slow_down;
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10}; 
idata unsigned char seg_disp_mode=0;//0-环境 1-运动 2-参数 3-统计
idata bit para_disp_mode;//0-温度 1-距离
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata bit led_flash_flag;
idata unsigned char led_flash_tick;
//温度
idata unsigned char T_val;
idata unsigned int temperature_slow_down;
//光强等级
idata unsigned char light_level;//0-3(1-4)
idata unsigned char ad_slow_down;
//运动
idata unsigned char motion_state;//0-静止 1-徘徊 2-跑动
idata unsigned char distance_val;
idata unsigned char distance_old;
idata unsigned int distance_slow_down;
idata bit got_dis_flag;
//参数
idata unsigned char T_para=30;//20-80
idata unsigned char distance_para=30;//20-80
idata unsigned int relay_count;
idata bit led_relay_lock;//1-锁定 0-不锁定
idata bit hot_flag;
idata bit close_flag;
idata bit suoding_flag;
idata unsigned int suoding_tick;
idata bit stay_flag;
idata bit paihuai_flag;
idata bit run_flag;
idata bit relay_old;  
idata unsigned char distance_sub;
void key_proc()
{
	if(key_slow_down<10) return;
	key_slow_down=0;
	key_val=Key_Read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	if(key_down==4)
	{
		seg_disp_mode=(++seg_disp_mode)%4;
		para_disp_mode=0;
		if(seg_disp_mode==2)
			led_relay_lock=1;
		else
			led_relay_lock=0;
	}
	if(seg_disp_mode==2)
	{
		if(key_down==5)
			para_disp_mode=!para_disp_mode;
		if(key_down==8)
		{
			if(para_disp_mode==0)
			{
				if(T_para<80) T_para++;
			}
			else
			{
				if(distance_para<80) distance_para+=5;
			}
		}
		if(key_down==9)
		{
			if(para_disp_mode==0)
			{
				if(T_para>20) T_para--;
			}
			else
			{
				if(distance_para>20) distance_para-=5;
			}			
		}
		
	}
	if(seg_disp_mode==3)
	{	
		if(key_long_tick>=2000)
			relay_count=0;
		if(key_old==89)
		{
			key_long_flag=1;
		}
		else
		{
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
		case 0: //环境状态
			seg_buf[0]=11;
			seg_buf[1]=T_val/10%10;
			seg_buf[2]=T_val%10;
			seg_buf[3]=seg_buf[4]=seg_buf[5]=10;
			seg_buf[6]=12;
			seg_buf[7]=light_level+1;//1-4
			break;
		case 1: //运动检测
			seg_buf[0]=13;
			seg_buf[1]=motion_state+1;
			seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
			seg_buf[5]=distance_val/100%10;
			seg_buf[6]=distance_val/10%10;
			seg_buf[7]=distance_val%10;			
			break;
		case 2: //参数设置
			seg_buf[0]=14;
			seg_buf[2]=seg_buf[3]=seg_buf[4]=seg_buf[5]=10;
			if(para_disp_mode==0) //温度参数
			{
				seg_buf[1]=11;
				seg_buf[6]=T_para/10%10;
				seg_buf[7]=T_para%10;	
			}	
			else //距离参数
			{
				seg_buf[1]=13;
				seg_buf[6]=distance_para/10%10;
				seg_buf[7]=distance_para%10;
			}
			break;
		case 3: //统计
			seg_buf[0]=12;
			seg_buf[1]=11;
			seg_buf[2]=seg_buf[3]=10;
			seg_buf[4]=(relay_count>=1000)?relay_count/1000%10:10;
			seg_buf[5]=(relay_count>=100)?relay_count/100%10:10;
			seg_buf[6]=(relay_count>=10)?relay_count/10%10:10;
			seg_buf[7]=relay_count%10;				
			break;			
	}
}


void led_proc()
{	
	if(!led_relay_lock)
	{
		if(close_flag)
		{
			if(light_level==0)
			{
				ucled[0]=1;
				ucled[1]=ucled[2]=ucled[3]=0;
			}
			else if(light_level==1)
			{
				ucled[0]=1;
				ucled[1]=1;
				ucled[2]=ucled[3]=0;			
			}
			else if(light_level==2)
			{
				ucled[0]=1;
				ucled[1]=1;
				ucled[2]=1;
				ucled[3]=0;			
			}
			else
			{
				ucled[0]=1;
				ucled[1]=1;
				ucled[2]=1;
				ucled[3]=1;				
			}
		}
		else
		{
			ucled[0]=ucled[1]=ucled[2]=ucled[3]=0;
		}
		
		if(hot_flag==1&&close_flag==1)
		{
			Relay(1);
			if(relay_old==0)
			{
				relay_count++;
			}
			relay_old=1;
		}
		else
		{
			Relay(0);
			relay_old=0;
		}
	}
	if(stay_flag)
		ucled[7]=0;
	else if(paihuai_flag)
		ucled[7]=1;
	else if(run_flag)
		ucled[7]=led_flash_flag;
	
}

void ad_proc()
{	
	float temp;
	if(ad_slow_down<100) return;
	ad_slow_down=0;
	temp=AD_Read(0x01)/51.0;
	if(temp>=3.0)
		light_level=0;
	else if(temp>=2.0)
		light_level=1;
	else if(temp>=0.5)
		light_level=2;
	else
		light_level=3;
	
}
void get_temperature()
{	
	float temp;
	if(temperature_slow_down<300) return;
	temperature_slow_down=0;
	temp=rd_temperature();
	T_val=(unsigned char)temp;
	if(temp>T_para)
		hot_flag=1;
	else
		hot_flag=0;
}

void distance_proc()
{	
	
	unsigned char temp_motion_state;
	if(distance_slow_down<999) return;
	distance_slow_down=0;
	
	if(got_dis_flag)
	{
		distance_val=Ut_Wave_Data();
		distance_sub=abs(distance_val-distance_old);
		distance_old=distance_val;
		if(distance_val<distance_para)
			close_flag=1;
		else
			close_flag=0;
		if(!suoding_flag)
		{
			if(distance_sub<5)
				temp_motion_state=0;
			else if(distance_sub<10)
				temp_motion_state=1;
			else
				temp_motion_state=2;
			if(temp_motion_state!=motion_state)
			{
				motion_state=temp_motion_state;
				suoding_flag=1;
				suoding_tick=0;
			}
		}
		if(!led_relay_lock)
		{
			stay_flag=(motion_state==0);
			paihuai_flag=(motion_state==1);
			run_flag=(motion_state==2);
		}
	}
	else
	{		
		distance_val=Ut_Wave_Data();
		distance_old=distance_val;
		got_dis_flag=1;
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
	EA=1;            //打开全局中断
}

void Timer1_Isr(void) interrupt 3
{
	key_slow_down++;
	seg_slow_down++;
	ad_slow_down++;
	temperature_slow_down++;
	distance_slow_down++;
	if(seg_buf[seg_pos]>20)
		Seg_Disp(seg_pos,seg_buf[seg_pos]-',',1);
	else
		Seg_Disp(seg_pos,seg_buf[seg_pos],0);
	seg_pos=(++seg_pos)%8;
	if(key_long_flag)
	{
		if(++key_long_tick>=2000) key_long_tick=2001;
	}
	else
	{
		key_long_tick=0;
	}
	
	if(suoding_flag)
	{
		if(++suoding_tick==3000) 
		{
			suoding_tick=0;
			suoding_flag=0;
		}
	}
	
	if(run_flag)
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
	Led_Disp(ucled);
}

void main()
{
	System_Init();
	Timer1_Init();
	while(rd_temperature()==85.0);
	AD_Read(0x01);
	while(1)
	{
		key_proc();
		seg_proc();
		led_proc();
		ad_proc();
		get_temperature();
		distance_proc();
	}

}