#include <STC15F2K60S2.H>
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "iic.h"
#include "ds1302.h"
//频率
idata unsigned int freq_test;//频率测量值
idata int freq_val;//频率最终值
idata bit freq_get_flag;//1-测量到频率了
idata unsigned int freq_max;
pdata unsigned char freq_max_time[3];
idata unsigned int freq_time_1s;
//按键
idata unsigned char key_val,key_old,key_down,key_up;
idata unsigned char key_slow_down;
//数码管
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down;
idata unsigned char seg_disp_mode=0;//0-频率界面 1-参数界面 2-时间界面 3-回显界面
idata bit para_disp_mode;//0-超限参数界面 1-校准值参数界面
idata bit back_disp_mode;//0-频率回显 1-时间回显
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata bit led1_flash_flag;
idata unsigned char led1_flash_tick;//200ms
idata bit led2_flash_flag;
idata unsigned char led2_flash_tick;//200ms
//参数
idata unsigned int chaoxian_para=2000;//超限参数 1000-9000
idata int jiaozhun_para=0;//校准值 -900-900
//时间
pdata unsigned char ucrtc[]={13,3,5};
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
		back_disp_mode=0;
	}
	if(key_down==5)
	{
		if(seg_disp_mode==1)
		{
			para_disp_mode=!para_disp_mode;
		}
		if(seg_disp_mode==3)
		{
			back_disp_mode=!back_disp_mode;
		}
	}
	if(seg_disp_mode==1)
	{
		if(para_disp_mode==0)
		{
			if(key_down==8)
			{
				if(chaoxian_para<9000) chaoxian_para+=1000;
			}
			if(key_down==9)
			{
				if(chaoxian_para>1000) chaoxian_para-=1000;
			}
		}
		else
		{
			if(key_down==8)
			{
				if(jiaozhun_para<900) jiaozhun_para+=100;
			}
			if(key_down==9)
			{
				if(jiaozhun_para>-900) jiaozhun_para-=100;
			}			
		}
	}
}


void seg_proc()
{
	if(seg_slow_down<80) return;
	seg_slow_down=0;
	read_rtc(ucrtc);
	switch(seg_disp_mode) //0-频率界面 1-参数界面 2-时间界面 3-回显界面
	{
		case 0:
			seg_buf[0]=11;
			seg_buf[1]=seg_buf[2]=10;
			if(freq_val>=0)
			{
				seg_buf[3]=(freq_val>=10000)?freq_val/10000%10:10;
				seg_buf[4]=(freq_val>=1000)?freq_val/1000%10:10;
				seg_buf[5]=(freq_val>=100)?freq_val/100%10:10;
				seg_buf[6]=(freq_val>=10)?freq_val/10%10:10;
				seg_buf[7]=freq_val%10;
			}
			else
			{
				seg_buf[3]=seg_buf[4]=seg_buf[5]=10;
				seg_buf[6]=seg_buf[7]=16;
			}
			break;
		case 1: //0-超限参数界面 1-校准值参数界面
			seg_buf[0]=12;
			seg_buf[2]=seg_buf[3]=10;
			if(para_disp_mode==0)
			{
				seg_buf[1]=1;
				seg_buf[4]=chaoxian_para/1000%10;
				seg_buf[5]=chaoxian_para/100%10;
				seg_buf[6]=chaoxian_para/10%10;
				seg_buf[7]=chaoxian_para%10;
			}
			else
			{
				seg_buf[1]=2;
				if(jiaozhun_para>=0)
				{
					seg_buf[4]=10;
					seg_buf[5]=(jiaozhun_para>=100)?jiaozhun_para/100%10:10;
					seg_buf[6]=(jiaozhun_para>=10)?jiaozhun_para/10%10:10;
					seg_buf[7]=jiaozhun_para%10;					
				}
				else
				{
					if((-jiaozhun_para)>=100)
					{
						seg_buf[4]=13;
						seg_buf[5]=(-jiaozhun_para)/100%10;
						seg_buf[6]=(-jiaozhun_para)/10%10;
						seg_buf[7]=(-jiaozhun_para)%10;						
					}
					else if((-jiaozhun_para)>=10)
					{
						seg_buf[4]=10;
						seg_buf[5]=13;
						seg_buf[6]=(-jiaozhun_para)/10%10;
						seg_buf[7]=(-jiaozhun_para)%10;							
					}
					else
					{
						seg_buf[4]=10;
						seg_buf[5]=10;
						seg_buf[6]=13;
						seg_buf[7]=(-jiaozhun_para)%10;							
					}
				}
			}
			break;
		case 2:
			seg_buf[0]=ucrtc[0]/10%10;
			seg_buf[1]=ucrtc[0]%10;
			seg_buf[2]=13;
			seg_buf[3]=ucrtc[1]/10%10;
			seg_buf[4]=ucrtc[1]%10;
			seg_buf[5]=13;
			seg_buf[6]=ucrtc[2]/10%10;
			seg_buf[7]=ucrtc[2]%10;			
			break;
		case 3:  //0-频率回显 1-时间回显
			if(back_disp_mode==0)
			{
				seg_buf[0]=14;
				seg_buf[1]=11;
				seg_buf[2]=10;
				seg_buf[3]=(freq_max>=10000)?freq_max/10000%10:10;
				seg_buf[4]=(freq_max>=1000)?freq_max/1000%10:10;
				seg_buf[5]=(freq_max>=100)?freq_max/100%10:10;
				seg_buf[6]=(freq_max>=10)?freq_max/10%10:10;
				seg_buf[7]=freq_max%10;		
			}
			else
			{
				seg_buf[0]=14;
				seg_buf[1]=15;
				seg_buf[2]=freq_max_time[0]/10%10;
				seg_buf[3]=freq_max_time[0]%10;
				seg_buf[4]=freq_max_time[1]/10%10;
				seg_buf[5]=freq_max_time[1]%10;
				seg_buf[6]=freq_max_time[2]/10%10;
				seg_buf[7]=freq_max_time[2]%10;						
			}
			break;
	}
}

void freq_proc()
{
	if(freq_get_flag==0) return;
	freq_get_flag=0;
	freq_val=freq_test+jiaozhun_para;
	if(freq_val>=0&&freq_val>freq_max)
	{
		freq_max=freq_val;
		read_rtc(freq_max_time);
	}
	if(freq_val>=0)
	{
		if(freq_val>chaoxian_para)
			da_write(5*51);
		else if(freq_val>=500)
			da_write((1+(4.0/(float)(chaoxian_para-500))*(freq_val-500))*51);
		else
			da_write(1*51);
	}
	else
	{
		da_write(0*51);
	}
}

void led_proc()
{
	ucled[0]=led1_flash_flag;
	if(freq_val>chaoxian_para)
	{
		ucled[1]=led2_flash_flag;
	}
	else
		ucled[1]=0;
	
	if(freq_val<0)
		ucled[1]=1;
	

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
	if(seg_buf[seg_pos]>20)
		seg_disp(seg_pos,seg_buf[seg_pos]-',',1);
	else
		seg_disp(seg_pos,seg_buf[seg_pos],0);
	seg_pos=(++seg_pos)%8;
	if(++freq_time_1s==1000)
	{
		freq_time_1s=0;		
		freq_test=TH0<<8|TL0;
		freq_get_flag=1;
		TH0=TL0=0;
	}
	if(seg_disp_mode==0)
	{
		if(++led1_flash_tick==200)
		{
			led1_flash_tick=0;
			led1_flash_flag=!led1_flash_flag;
		}
	}
	else
	{
		led1_flash_tick=0;
		led1_flash_flag=0;
	}
	if(freq_val>chaoxian_para)
	{
		if(++led2_flash_tick==200)
		{
			led2_flash_tick=0;
			led2_flash_flag=!led2_flash_flag;
		}		
	}
	else
	{
		led2_flash_tick=0;
		led2_flash_flag=0;
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
		freq_proc();
		led_proc();
	}
}