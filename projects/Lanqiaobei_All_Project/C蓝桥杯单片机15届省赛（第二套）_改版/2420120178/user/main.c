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
idata unsigned char key_long_flag;
idata unsigned int key_long_tick;
//数码管
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down;
idata unsigned char seg_disp_mode=0;//0-温度界面 1-dac输出控制界面 2-校准值界面
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata unsigned int time1_2s;
idata unsigned int time2_2s;
idata unsigned int time_3s;
idata bit led_flash_flag;
idata unsigned char led_flash_tick;
idata bit T_up_flag;//温度上升标志位
idata bit T_down_flag;//温度下降标志位
//温度
idata unsigned char T_val;
idata unsigned char T_old;
idata unsigned int T_slow_down;
idata bit baojing_flag;
//dac
idata unsigned char dac_data=100;//0-255
idata bit dac_model=0;//0-温度控制 1-手动控制
//校准值
idata char jiaozhun_para=0;//-9 - 9
idata char jiaozhun_para_ct=0;
//锁定
idata bit lock_flag=0;//1-锁定 0-解锁
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
				jiaozhun_para_ct=jiaozhun_para;
				seg_disp_mode=2;
				break;
			case 2:
				seg_disp_mode=0;
				jiaozhun_para=jiaozhun_para_ct;
				break;			
		}		
	}
	if(key_down==16)
	{
		switch(seg_disp_mode)
		{
			case 0:
				if(lock_flag==0)
					relay(1);
				break;
			case 1:
				if(dac_data<255) dac_data+=5;
				break;
			case 2:
				if(jiaozhun_para_ct<9) jiaozhun_para_ct++;
				break;			
		}
	}
	if(key_down==17)
	{
		switch(seg_disp_mode)
		{
			case 0:
				if(lock_flag==0)
					relay(0);
				break;
			case 1:
				if(dac_data>0) dac_data-=5;
				break;
			case 2:
				if(jiaozhun_para_ct>-9) jiaozhun_para_ct--;
				break;			
		}
	}
	if(key_down==13)
	{
		key_long_flag=1;
	}
	if(key_up==13)
	{
		if(key_long_tick>=1500)
		{
			lock_flag=!lock_flag;
		}
		else
		{
			dac_model=!dac_model;
		}
		key_long_flag=0;
		key_long_tick=0;
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
			seg_buf[1]=seg_buf[2]=seg_buf[3]=seg_buf[4]=seg_buf[5]=10;
			seg_buf[6]=T_val/10%10;
			seg_buf[7]=T_val%10;
			break;
		case 1:
			seg_buf[0]=12;
			seg_buf[1]=seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
			seg_buf[5]=(dac_data>=100)?dac_data/100%10:10;
			seg_buf[6]=(dac_data>=10)?dac_data/10%10:10;
			seg_buf[7]=dac_data%10;
			break;
		case 2:
			seg_buf[0]=13;
			seg_buf[1]=seg_buf[2]=seg_buf[3]=seg_buf[4]=seg_buf[5]=10;
			if(jiaozhun_para_ct>=0)
			{
				seg_buf[6]=10;
				seg_buf[7]=jiaozhun_para_ct%10;
			}
			else
			{
				seg_buf[6]=14;
				seg_buf[7]=(-jiaozhun_para_ct)%10;
			}
			break;
	
	}
}

void led_proc()
{
	ucled[0]=(!dac_model);
	ucled[1]=T_up_flag;
	ucled[2]=T_down_flag;
	ucled[3]=led_flash_flag;
	ucled[7]=(!lock_flag);
}

void get_temperature()
{
	float temp_val;
	char diff;
	if(T_slow_down<300) return;
	T_slow_down=0;
	T_old=T_val;
	temp_val=rd_temperature();
	T_val=(temp_val+jiaozhun_para<0)? 0:(unsigned char)(temp_val+jiaozhun_para);
	if(T_val>T_old) 
	{
		if(T_up_flag==0)
		{
			T_up_flag=1;
			time1_2s=0;
		}
	}
	if(T_val<T_old)
	{
		if(T_down_flag==0)
		{
			T_down_flag=1;
			time2_2s=0;	
		}			
	}
	
	diff = (char)T_val - (char)T_old;
	if(diff > 1 || diff < -1)
	{
		if(baojing_flag==0)
		{
			baojing_flag=1;
			time_3s=0;
			led_flash_tick=0;
			led_flash_flag=1;
		}
	}
}

void da_proc()
{
	float temp;
	if(dac_model==0)
	{
		if(T_val>=40) temp=5;
		else if(T_val>10) temp=(1.0/10.0)*(T_val-10)+2;
		else temp=2;
		da_write((unsigned char)(temp*51));
	}
	else
	{
		da_write(dac_data);
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
	
	if(key_long_flag)
	{
		if(++key_long_tick>=1500) key_long_tick=1501;
	}
	else
	{
		key_long_tick=0;
	}
	if(T_up_flag)
	{
		if(++time1_2s==2000)
		{
			T_up_flag=0;
			time1_2s=0;
		}
	}
	if(T_down_flag)
	{
		if(++time2_2s==2000)
		{
			T_down_flag=0;
			time2_2s=0;
		}
	}	
	if(baojing_flag)
	{
		if(++led_flash_tick==200)
		{
			led_flash_tick=0;
			led_flash_flag=!led_flash_flag;
		}
		if(++time_3s==3000)
		{
			time_3s=0;
			led_flash_tick=0;
			led_flash_flag=0;
			baojing_flag=0;
		}
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
		get_temperature();	
		led_proc();
		da_proc();
	}

}
