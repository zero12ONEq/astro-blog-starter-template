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
idata bit key_long_flag;
idata unsigned int key_long_tick;//2s
//数码管
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down; 
idata unsigned char seg_disp_mode=0;//0-测距界面 1-参数界面 2-工厂界面
idata bit para_disp_mode;//0-距离参数 1-温度参数
idata unsigned char factory_disp_mode;//0-校准值 1-介质 2-DAC输出设置
//温度
idata unsigned int T_slow_down;
idata unsigned int T_val_10x;
idata unsigned char T_para=30;//0-80
//距离
idata unsigned char dist_slow_down;
idata unsigned int dist_val;//单位cm
idata unsigned char dist_para=40;//10-90
idata bit danwei_flag=0;//0-cm 1-m
idata bit jilu_flag;//1-开始记录
idata bit finish_flag;//1-数据记录完成
idata unsigned int time_6s;
pdata unsigned int dist_jilu_data[30];
idata unsigned char dist_jilu_index=0;
idata unsigned char time_200ms;
//校准
idata char jiaozhun_para=0;//-90-90
//速度
idata unsigned int speed=340;//单位：米每秒 10-9990
//dac
idata unsigned char dac_para_10x=10;//(0.1-2.0)1-20
idata bit da_flag;//1-开始dac输出标志位
idata unsigned char da_time_200ms;
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata bit led_flash_flag;
idata unsigned char led_flash_tick;
void key_proc()
{
	if(key_slow_down<10) return;
	key_slow_down=0;
	key_val=key_read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	if(!jilu_flag)
	{
		if(key_down==4)
		{
			seg_disp_mode=(++seg_disp_mode)%3;
			danwei_flag=0;
			para_disp_mode=0;
			factory_disp_mode=0;
			led_off();
		}
		if(key_down==5)
		{
			if(seg_disp_mode==0)
			{
				danwei_flag=!danwei_flag;
			}
			else if(seg_disp_mode==1)
			{
				para_disp_mode=!para_disp_mode;
			}
			else
			{
				factory_disp_mode=(++factory_disp_mode)%3;
			}
		}
		if(key_down==8)
		{
			if(seg_disp_mode==0)
			{
				jilu_flag=1;
				time_6s=0;
				dist_jilu_index=0;
				time_200ms=0;
				finish_flag=0;
			}
			else if(seg_disp_mode==1)
			{
				if(para_disp_mode==0)
				{
					if(dist_para<90) dist_para+=10;
				}
				else
				{
					if(T_para<80) T_para++;
				}
			}
			else
			{
				if(factory_disp_mode==0)
				{
					if(jiaozhun_para<90) jiaozhun_para+=5;
				}
				else if(factory_disp_mode==1)
				{
					if(speed<9990) speed+=10;
				}	
				else
				{
					if(dac_para_10x<20) dac_para_10x++;
				}
			}
		}
		if(key_down==9)
		{
			if(seg_disp_mode==0)
			{
				if(finish_flag)
				{
					da_flag=1;
					da_time_200ms=0;
					finish_flag=0;
					dist_jilu_index=0;
				}
			}
			else if(seg_disp_mode==1)
			{
				if(para_disp_mode==0)
				{
					if(dist_para>10) dist_para-=10;
				}
				else
				{
					if(T_para>0) T_para--;
				}
			}
			else
			{
				if(factory_disp_mode==0)
				{
					if(jiaozhun_para>-90) jiaozhun_para-=5;
				}
				else if(factory_disp_mode==1)
				{
					if(speed>10) speed-=10;
				}	
				else
				{
					if(dac_para_10x>1) dac_para_10x--;
				}
			}
		}
		if(key_long_tick>=2000)
		{
			seg_disp_mode=0;
			danwei_flag=0;
			dist_para=40;
			T_para=30;
			jiaozhun_para=0;
			speed=340;
			dac_para_10x=10;
		}
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
		case 0:
			seg_buf[0]=T_val_10x/100%10;
			seg_buf[1]=T_val_10x/10%10+',';
			seg_buf[2]=T_val_10x%10;
			seg_buf[3]=11;
			if(danwei_flag)
			{
				seg_buf[4]=(dist_val>=1000)?dist_val/1000%10:10;
				seg_buf[5]=dist_val/100%10+',';
				seg_buf[6]=dist_val/10%10;
				seg_buf[7]=dist_val%10;
			}
			else
			{
				seg_buf[4]=(dist_val>=1000)?dist_val/1000%10:10;
				seg_buf[5]=(dist_val>=100)?dist_val/100%10:10;
				seg_buf[6]=(dist_val>=10)?dist_val/10%10:10;
				seg_buf[7]=dist_val%10;				
			
			}
			break;
		case 1:
			seg_buf[0]=12;
			seg_buf[2]=seg_buf[3]=seg_buf[4]=seg_buf[5]=10;
			if(para_disp_mode==0)
			{
				seg_buf[1]=1;
				seg_buf[6]=dist_para/10%10;
				seg_buf[7]=dist_para%10;
			}
			else
			{
				seg_buf[1]=2;
				seg_buf[6]=T_para/10%10;
				seg_buf[7]=T_para%10;				
			}
			break;
		case 2:
			seg_buf[0]=13;
			if(factory_disp_mode==0)
			{
				seg_buf[1]=1;
				seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
				if(jiaozhun_para>=0)
				{
					seg_buf[5]=(jiaozhun_para>=100)?jiaozhun_para/100%10:10;
					seg_buf[6]=(jiaozhun_para>=10)?jiaozhun_para/10%10:10;
					seg_buf[7]=jiaozhun_para%10;
				}
				else
				{
					if((-jiaozhun_para)>=10)
					{
						seg_buf[5]=11;
						seg_buf[6]=(-jiaozhun_para)/10%10;
						seg_buf[7]=(-jiaozhun_para)%10;						
					}
					else
					{
						seg_buf[5]=10;
						seg_buf[6]=11;
						seg_buf[7]=(-jiaozhun_para)%10;							
					}
				}
			}
			else if(factory_disp_mode==1)
			{
				seg_buf[1]=2;
				seg_buf[2]=seg_buf[3]=10;
				seg_buf[4]=(speed>=1000)?speed/1000%10:10;
				seg_buf[5]=(speed>=100)?speed/100%10:10;
				seg_buf[6]=(speed>=10)?speed/10%10:10;
				seg_buf[7]=speed%10;
			}
			else
			{
				seg_buf[1]=3;
				seg_buf[2]=seg_buf[3]=seg_buf[4]=seg_buf[5]=10;
				seg_buf[6]=dac_para_10x/10%10+',';
				seg_buf[7]=dac_para_10x%10;					
			}
			break;		
	}
}


void led_proc()
{
	unsigned char i;
	if(seg_disp_mode==0)
	{
		if(dist_val<255)
		{
			for(i=0;i<8;i++)
			{
				ucled[i]=(dist_val>>i)&0x01;
			}
		}
		else
		{
			ucled[0]=ucled[1]=ucled[2]=ucled[3]=1;
			ucled[4]=ucled[5]=ucled[6]=ucled[7]=1;
		}
	}
	else if(seg_disp_mode==1)
	{
		ucled[7]=1;
		ucled[0]=ucled[1]=ucled[2]=ucled[3]=0;
		ucled[4]=ucled[5]=ucled[6]=0;		
	}
	else 
	{
		ucled[0]=led_flash_flag;
		ucled[1]=ucled[2]=ucled[3]=0;
		ucled[4]=ucled[5]=ucled[6]=ucled[7]=0;	
	}
}

void relay_proc()
{
	if(dist_para-5<=dist_val&&dist_val<=dist_para+5&&(T_val_10x/10.0)<=T_para)
	{
		relay(1);
	}
	else
	{
		relay(0);
	}
		
}
void get_temperature()
{
	float temp;
	if(T_slow_down<300) return;
	T_slow_down=0;
	temp=rd_temperature();
	T_val_10x=(unsigned int)(temp*10);
}

void da_proc()
{
	unsigned int temp1;
	float temp2;
	if(da_time_200ms<200) return;
	da_time_200ms=0;
	temp1=dist_jilu_data[dist_jilu_index];
	if(temp1>90)
		da_write(5*51);
	else if(temp1>=10)
	{
		temp2=((float)(5-(dac_para_10x/10.0))/80.0)*(temp1-10)+(dac_para_10x/10.0);
		da_write(temp2*51);
	}
	else
		da_write((dac_para_10x/10.0)*51);
	if(++dist_jilu_index==30)
		da_flag=0;
}
void get_distance()
{
	unsigned int time;
	float temp;
	if(dist_slow_down<100) return;
	dist_slow_down=0;
	time=ut_wave_data();
	temp=time*((float)speed/20000.0)+jiaozhun_para;
	dist_val=(temp>=0)?(unsigned int)temp:0;
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
	dist_slow_down++;
	if(da_flag) da_time_200ms++;
	if(seg_buf[seg_pos]>20)
		seg_disp(seg_pos,seg_buf[seg_pos]-',',1);
	else
		seg_disp(seg_pos,seg_buf[seg_pos],0);
	seg_pos=(++seg_pos)%8;
	if(key_long_flag)
	{
		if(++key_long_tick>=2000) key_long_tick=2001;
	}
	else
	{
		key_long_tick=0;
	}
	if(jilu_flag)
	{
		if(++time_200ms==200)
		{
			time_200ms=0;
			if(dist_jilu_index<30)
				dist_jilu_data[dist_jilu_index++]=dist_val;
		}
		if(++time_6s==6000)
		{
			time_6s=0;
			finish_flag=1;
			jilu_flag=0;
		}
	}
	if(seg_disp_mode==2)
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
		relay_proc();
		get_temperature();
		da_proc();
		get_distance();
	}

}