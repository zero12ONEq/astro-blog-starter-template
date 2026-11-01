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
idata unsigned int key_long_tick;
//数码管
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down;
idata unsigned char seg_disp_mode=0;//0-测距界面 1-参数界面 2工厂界面
idata bit para_disp_mode;//0-距离参数 1-温度参数
idata unsigned char gongchang_disp_mode;//0-校准值界面 1-介质设置界面 2-dac设置输出界面
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata bit led_flash_flag;
idata unsigned char led_flash_tick;
//温度
idata unsigned int T_slow_down;
idata unsigned int T_val_10x;
//距离
idata bit danwei_flag=0;//0-cm 1-m
idata unsigned int distance_val;
pdata unsigned int record_data[30]; // 记录下来的距离数据，每 200ms 采集一次，6 秒一共 30 次
idata unsigned int record_data_index;
idata unsigned int distance_jilu;
idata unsigned char dist_slow_down;
//参数
idata unsigned char distance_para=40;//10-90
idata unsigned char T_para=30;//0-80
idata char jiaozhun_para=0;//-90cm-90cm 
idata unsigned int speed=340;//10-9990 m/s
idata unsigned char dac_xiaxian_10x=10;//1-20
idata bit jilu_flag;//"记录"标志位 0-不使能 1-使能
idata bit finish_jilu_flag;//完成记录标志位
idata unsigned int Time_6s;
idata unsigned char time200ms;
idata unsigned char da_time200ms;
idata bit da_flag;//开始进行da输出的标准位
void key_proc()
{
	if(key_slow_down<10) return;
	key_slow_down=0;
	key_val=key_read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	if(!jilu_flag) //如果不使能“记录”
	{
		if(key_down==4)
		{
			seg_disp_mode=(++seg_disp_mode)%3;
			led_off();//每次切换界面，关闭所有led，清空上一个界面的led数据
			danwei_flag=0;//默认测距界面显示cm
			para_disp_mode=0;//默认进入参数界面显示距离参数
			gongchang_disp_mode=0;
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
				gongchang_disp_mode=(++gongchang_disp_mode)%3;
			}
		}
		if(key_down==8)
		{
			if(seg_disp_mode==1)
			{
				if(para_disp_mode==0)
				{
					if(distance_para<90) distance_para+=10;
				}
				else
				{
					if(T_para<80) T_para++;
				}
			}
			else if(seg_disp_mode==2)
			{
				if(gongchang_disp_mode==0)
				{
					if(jiaozhun_para<90) jiaozhun_para+=5;
				}
				else if(gongchang_disp_mode==1)
				{
					if(speed<9990) speed+=10;
				}
				else
				{
					if(dac_xiaxian_10x<20) dac_xiaxian_10x++;
				}
			}
			else
			{
				jilu_flag=1;
				record_data_index = Time_6s = time200ms = 0; 
				finish_jilu_flag=0; 
			}
		}
		
		if(key_down==9)
		{
			if(seg_disp_mode==1)
			{
				if(para_disp_mode==0)
				{
					if(distance_para>10) distance_para-=10;
				}
				else
				{
					if(T_para>0) T_para--;
				}
			}
			else if(seg_disp_mode==2)
			{
				if(gongchang_disp_mode==0)
				{
					if(jiaozhun_para>-90) jiaozhun_para-=5;
				}
				else if(gongchang_disp_mode==1)
				{
					if(speed>10) speed-=10;
				}
				else
				{
					if(dac_xiaxian_10x>1) dac_xiaxian_10x--;
				}
			}
			else
			{
				if(finish_jilu_flag)  //如果录入完数据了
				{
          da_flag = 1;
					da_time200ms = 0;
          record_data_index = 0;
          finish_jilu_flag = 0;
				}
			}
		}
		
		if(key_long_tick>2000)
		{
			seg_disp_mode=0;
			danwei_flag=0;
			distance_para=40;
			T_para=30;
			jiaozhun_para=0;
			speed=340;
			dac_xiaxian_10x=10;
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
			if(danwei_flag==0)
			{
				seg_buf[4]=(distance_val>=1000)?distance_val/1000%10:10;
				seg_buf[5]=(distance_val>=100)?distance_val/100%10:10;
				seg_buf[6]=(distance_val>=10)?distance_val/10%10:10;
				seg_buf[7]=distance_val%10;
			}
			else
			{
				seg_buf[4]=(distance_val>=1000)?distance_val/1000%10:10;
				seg_buf[5]=distance_val/100%10+',';
				seg_buf[6]=distance_val/10%10;
				seg_buf[7]=distance_val%10;
			}
			break;
		case 1:
			seg_buf[0]=12;
			seg_buf[2]=seg_buf[3]=seg_buf[4]=seg_buf[5]=10;
			if(para_disp_mode==0)
			{
				seg_buf[1]=1;
				seg_buf[6]=distance_para/10%10;
				seg_buf[7]=distance_para%10;
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
			if(gongchang_disp_mode==0) //校准值设置界面
			{
				seg_buf[1]=1;
				seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
				if(jiaozhun_para>=0)
				{
					seg_buf[5]=10;
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
			else if(gongchang_disp_mode==1) //介质设置界面
			{
				seg_buf[1]=2;
				seg_buf[2]=seg_buf[3]=10;
				seg_buf[4]=(speed>=1000)?speed/1000%10:10;	
				seg_buf[5]=(speed>=100)?speed/100%10:10;
				seg_buf[6]=(speed>=10)?speed/10%10:10;	
				seg_buf[7]=speed%10;	
			}
			else //dac输出界面
			{
				seg_buf[1]=3;
				seg_buf[2]=seg_buf[3]=seg_buf[4]=	seg_buf[5]=10;
				seg_buf[6]=dac_xiaxian_10x/10%10+',';	
				seg_buf[7]=dac_xiaxian_10x%10;				
			}
			break;	
	}
}

void led_proc()
{
	unsigned char i;
	if(seg_disp_mode==0)
	{
		for(i=0;i<8;i++)
		{
			ucled[i]=(distance_val>>i)&0x01;
		}
		if(distance_val>255)
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
	if(distance_para-5<=distance_val&&distance_para+5>=distance_val&&(T_val_10x/10.0)<=T_para)
	{
		relay(1);
	}
	else
	{
		relay(0);
	}
}

void da_proc()
{
	idata float V_low;
	if(da_time200ms<200) return;
	da_time200ms=0;
	V_low = dac_xiaxian_10x / 10.0;   // DAC下限电压（默认1.0V）
	if(record_data[record_data_index] >= 90)
		da_write(5*51.0);
	else if(record_data[record_data_index] <= 10)
		da_write(V_low*51.0);
	else
		da_write(((5.0 - V_low) / 80.0 * record_data[record_data_index] + ((9*V_low-5)/8.0))*51.0); 
	if(++record_data_index == 30)
		da_flag = 0;
}
void get_temperature()
{	
	idata float temp;
	if(T_slow_down<300) return;
	T_slow_down=0;
	temp=rd_temperature();
	T_val_10x=(unsigned int)(temp*10);
}

void get_distance()
{
	idata unsigned int time; 
	idata float temp_dist;
	if(dist_slow_down<100) return;
	dist_slow_down=0;
	time=ut_wave_data();
	temp_dist = (time*(speed/10000.0)*0.5) + jiaozhun_para;
  distance_val = (temp_dist > 0) ? (unsigned int)temp_dist : 0; //如果temp_dist为负数，那么实际距离赋值为0
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
	T_slow_down++;
	dist_slow_down++;
	if(da_flag) da_time200ms++;
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
	if(jilu_flag) 
	{
		if(++time200ms == 200) 
		{
      time200ms = 0;
      record_data[record_data_index] = distance_val;  // 存入数组
      record_data_index++;                         // 索引递增
		}
		if(++Time_6s==6000)
		{
			jilu_flag=0;
			Time_6s=0;
			finish_jilu_flag=1; //完成记录标志位置为1
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
		led_proc();
		relay_proc();
		da_proc();
		get_temperature();
		get_distance();
	}
}
