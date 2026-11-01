#include <STC15F2K60S2.H>
#include "string.h"
#include "stdio.h"
#include "math.h"
#include "led.h"
#include "init.h"
#include "seg.h"
#include "key.h"
#include "iic.h"
#include "ultrasound.h"
#include "uart.h"
//频率，速度计算
idata unsigned int freq;
idata unsigned int freq_time_1s;
idata unsigned int speed_10x;
idata unsigned char speed_slow_down;
pdata float speed_x,speed_y;
//处于运动状态且速度改变，和从静止（等待，空闲）到运动都要重算速度，
//把当前位置赋值给起始位置，进行时间计时器计数器的初始化
idata bit speed_update_flag;//速度更新标志位，当频率更新时，速度更新标志位置为1
//串口
idata bit uart_rx_flag;
idata unsigned char uart_rx_tick;
pdata unsigned char uart_rx_buf[]={0,0,0,0,0,0,0,0,0,0};
idata unsigned char uart_rx_index;
//按键
idata unsigned char key_slow_down;
idata unsigned char key_val,key_old,key_down,key_up;
//数码管
idata unsigned char seg_slow_down;
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_disp_mode=0;//0-坐标界面 1-速度界面 2-参数界面 
idata unsigned char state_disp_mode=0;//0-空闲 1-运行 2-等待
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata bit led_flash_flag;
idata unsigned char led_flash_tick;
//ad
idata bit light_flag;//1-日间 2-夜晚
idata unsigned char ad_slow_down;
//距离
idata bit finish_flag;//1-到达目的地
idata bit L3_flag;//led3亮灯标志位
idata unsigned char dist_slow_down;
idata unsigned int finish_time_3s;
//参数
pdata unsigned int destination[2];
pdata unsigned int location[2]={0,0};
idata unsigned char obstacle_distance;
idata unsigned char R_CT_para_10x=10; //1.0-2.0(10-20) 控制值
idata char B_CT_para=0;//-90-90 控制值
idata unsigned char R_para_10x=10; //1.0-2.0(10-20)
idata char B_para=0;//-90-90
idata bit para_disp_mode;//0-R参数 1-B参数
idata bit move_obstacle_flag;//1-障碍物移除
idata bit gotit_flag;//1-空闲状态获得目的地坐标
//运动控制
idata unsigned char move_slow_down;
pdata float new_x,new_y;
pdata unsigned int start_location[2];
pdata float move_time;//运动时间 s为单位
idata unsigned char move_time_100ms;//100ms运动计时器
idata unsigned int count_100ms;//几个100ms的计数器 1-1个100ms

void move_proc()
{	
	if(move_slow_down<200) return;
	move_slow_down=0;
	if(state_disp_mode==1)
	{
		if(obstacle_distance<30) 
		{
			state_disp_mode=2;
			return;//遇到障碍物切换等待状态，并返回
		}
		move_time=count_100ms*0.1;
		new_x=(float)start_location[0]+speed_x*move_time;
		new_y=(float)start_location[1]+speed_y*move_time;
		if(speed_x>=0)
		{
			if(new_x>=(float)destination[0])  new_x=(float)destination[0];
		}
		else
		{
			if(new_x<=(float)destination[0])  new_x=(float)destination[0];
		}
		
		if(speed_y>=0)
		{
			if(new_y>=(float)destination[1])  new_y=(float)destination[1];
		}
		else
		{
			if(new_y<=(float)destination[1])  new_y=(float)destination[1];
		}
		location[0]=(unsigned int)(new_x+0.5f);
		location[1]=(unsigned int)(new_y+0.5f);
		if(location[0]==destination[0]&&location[1]==destination[1])
		{
			finish_flag=1;
			finish_time_3s=0;
			state_disp_mode=0;
		}
	}
}
void speed_proc()
{	
	if(speed_slow_down<200) return;
	speed_slow_down=0;
	if(speed_update_flag)
	{
		unsigned int speed_10x_old;
		float temp_speed_10x;
		float dist_x,dist_y,dist;
		speed_10x_old=speed_10x;
		temp_speed_10x=((float)(3.14*(R_para_10x/10.0)*freq)/100.0f+B_para)*10;
		speed_10x=(temp_speed_10x<0)?0:(unsigned int)temp_speed_10x;
		if(state_disp_mode==1 && speed_10x!=speed_10x_old) //处于运动态，并且速度改变
		{
			start_location[0]=location[0];
			start_location[1]=location[1];
			move_time_100ms=0;
			count_100ms=0;
			dist_x=(float)destination[0]-(float)location[0];
			dist_y=(float)destination[1]-(float)location[1];
			dist=sqrt(dist_x*dist_x+dist_y*dist_y);
			if(dist>0) //如果每达到目的地
			{
				speed_x=((float)speed_10x/10.0)*(dist_x/dist);
				speed_y=((float)speed_10x/10.0)*(dist_y/dist);
			}
			else //如果到达目的地
			{
				speed_x=0;
				speed_y=0;
			}
		}
	}
}
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
			if(state_disp_mode==0&&gotit_flag==1)
			{
				float temp_speed_10x;
				float dist_x,dist_y,dist;
				gotit_flag=0;
				temp_speed_10x=((float)(3.14*(R_para_10x/10.0)*freq)/100.0f+B_para)*10;
				speed_10x=(temp_speed_10x<0)?0:(unsigned int)temp_speed_10x;	
				start_location[0]=location[0];
				start_location[1]=location[1];
				move_time_100ms=0;
				count_100ms=0;
				dist_x=(float)destination[0]-(float)location[0];
				dist_y=(float)destination[1]-(float)location[1];
				dist=sqrt(dist_x*dist_x+dist_y*dist_y);
				if(dist>0)
				{
					speed_x=(speed_10x/10.0)*(dist_x/dist);
					speed_y=(speed_10x/10.0)*(dist_y/dist);					
				}
				else
				{
					speed_x=0;
					speed_y=0;
				}
				state_disp_mode=1;
			}
			else if(state_disp_mode==2&&obstacle_distance>=30)
			{
				float temp_speed;
				float dist_x,dist_y,dist;
				temp_speed=((float)(3.14*(R_para_10x/10.0)*freq)/100.0f+B_para)*10;
				speed_10x=(temp_speed<0)?0:(unsigned int)temp_speed;	
				start_location[0]=location[0];
				start_location[1]=location[1];
				move_time_100ms=0;
				count_100ms=0;
				dist_x=(float)destination[0]-(float)location[0];
				dist_y=(float)destination[1]-(float)location[1];
				dist=sqrt(dist_x*dist_x+dist_y*dist_y);
				if(dist>0)
				{
					speed_x=(speed_10x/10.0)*(dist_x/dist);
					speed_y=(speed_10x/10.0)*(dist_y/dist);					
				}
				else
				{
					speed_x=0;
					speed_y=0;
				}
				state_disp_mode=1;				
			}
			else if(state_disp_mode==1)
			{
				state_disp_mode=2;
			}
				
	}
	if(key_down==5)
	{
		if(state_disp_mode==0)
		{
			location[0]=location[1]=0;
		}
	}
	if(key_down==8)
	{
		switch(seg_disp_mode)
		{
			case 0:
				seg_disp_mode=1;
				break;
			case 1:
				B_CT_para=B_para;
				R_CT_para_10x=R_para_10x;
				para_disp_mode=0;
				seg_disp_mode=2;
				break;
			case 2:
				seg_disp_mode=0;
				B_para=B_CT_para;
				R_para_10x=R_CT_para_10x;
				break;
		}
	}
	if(seg_disp_mode==2)
	{
		if(key_down==9)
		{
			para_disp_mode=!para_disp_mode;
		}
		if(key_down==12)
		{
			if(para_disp_mode==0)
			{
				if(R_CT_para_10x<20) R_CT_para_10x++;
			}
			else
			{
				if(B_CT_para<90) B_CT_para+=5;
			}
		}
		if(key_down==13)
		{
			if(para_disp_mode==0)
			{
				if(R_CT_para_10x>10) R_CT_para_10x--;
			}
			else
			{
				if(B_CT_para>-90) B_CT_para-=5;
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
			seg_buf[4]=14;
			if(state_disp_mode==1||state_disp_mode==2)
			{
				seg_buf[1]=(destination[0]>=100)?destination[0]/100%10:10;
				seg_buf[2]=(destination[0]>=10)?destination[0]/10%10:10;
				seg_buf[3]=destination[0]%10;
				seg_buf[5]=(destination[1]>=100)?destination[1]/100%10:10;
				seg_buf[6]=(destination[1]>=10)?destination[1]/10%10:10;
				seg_buf[7]=destination[1]%10;
			}
			else
			{
				seg_buf[1]=(location[0]>=100)?location[0]/100%10:10;
				seg_buf[2]=(location[0]>=10)?location[0]/10%10:10;
				seg_buf[3]=location[0]%10;
				seg_buf[5]=(location[1]>=100)?location[1]/100%10:10;
				seg_buf[6]=(location[1]>=10)?location[1]/10%10:10;
				seg_buf[7]=location[1]%10;				
			}
			break;
		case 1:
			seg_buf[0]=12;
			seg_buf[2]=10;
			if(state_disp_mode==1)
			{
				seg_buf[1]=1;
				seg_buf[3]=(speed_10x>=10000)?speed_10x/10000%10:10;
				seg_buf[4]=(speed_10x>=1000)?speed_10x/1000%10:10;
				seg_buf[5]=(speed_10x>=100)?speed_10x/100%10:10;
				seg_buf[6]=speed_10x/10%10+',';
				seg_buf[7]=speed_10x%10;
			}
			else if(state_disp_mode==0)
			{
				seg_buf[1]=2;
				seg_buf[3]=seg_buf[4]=seg_buf[5]=seg_buf[6]=seg_buf[7]=14;
			}
			else
			{
				seg_buf[1]=3;
				seg_buf[3]=(obstacle_distance>=10000)?obstacle_distance/10000%10:10;
				seg_buf[4]=(obstacle_distance>=1000)?obstacle_distance/1000%10:10;
				seg_buf[5]=(obstacle_distance>=100)?obstacle_distance/100%10:10;
				seg_buf[6]=(obstacle_distance>=10)?obstacle_distance/10%10:10;
				seg_buf[7]=obstacle_distance%10;				
			}
			break;
		case 2:
			seg_buf[0]=13;
			seg_buf[1]=10;
			seg_buf[4]=10;
			seg_buf[2]=R_CT_para_10x/10%10+',';
			seg_buf[3]=R_CT_para_10x%10;
			if(B_CT_para>=0)
			{
				seg_buf[5]=10;
				seg_buf[6]=(B_CT_para>=10)?B_CT_para/10%10:10;
				seg_buf[7]=B_CT_para%10;
			}
			else
			{
				if((-B_CT_para)>=10)
				{
					seg_buf[5]=14;
					seg_buf[6]=(-B_CT_para)/10%10;
					seg_buf[7]=(-B_CT_para)%10;					
				}
				else
				{
					seg_buf[5]=10;
					seg_buf[6]=14;
					seg_buf[7]=(-B_CT_para)%10;						
				}
			}
			break;		
	}
}

void led_proc()
{
	switch(state_disp_mode)
	{
		case 0:
			ucled[0]=0;
			ucled[1]=0;
			relay(0);
			break;
		case 1:
			ucled[0]=1;
			ucled[1]=(!light_flag);
			relay(1);
			break;
		case 2:
			ucled[0]=led_flash_flag;
			ucled[1]=0;
			relay(0);
			break;
	}
	ucled[2]=L3_flag;
}

void get_ad()
{
	float temp;
	if(ad_slow_down<100) return;
	ad_slow_down=0;
	temp=ad_read(0x01)/51.0;
	if(temp>1.2)
		light_flag=1;
	else
		light_flag=0;
}

void get_distance()
{
	unsigned char temp;
	if(dist_slow_down<100) return;
	dist_slow_down=0;
	temp=ut_wave_data();
	obstacle_distance= (temp==0)?255:temp;
	if(state_disp_mode==1)
	{
		if(obstacle_distance<30)
			state_disp_mode=2;
	}
}

void uart_proc()
{
	unsigned int x,y;
	if(uart_rx_index==0) return;
	if(uart_rx_tick>=10)
	{
		uart_rx_tick=0;
		uart_rx_flag=0;
		if(uart_rx_buf[0]=='(' && uart_rx_buf[uart_rx_index-1]==')')
		{
			if(sscanf(uart_rx_buf,"(%u,%u)",&x,&y)==2)
			{
				if(state_disp_mode==0)
				{
					printf("Got it");
					destination[0]=x;
					destination[1]=y;
					gotit_flag=1;
				}
				else if(state_disp_mode==1)
				{
					printf("Busy");
				}
				else
				{
					printf("Wait");
				}
			}
			else
			{
				printf("Error");
			}
		}
		else if(strcmp(uart_rx_buf,"?")==0)
		{
				if(state_disp_mode==0)
				{
					printf("Idle");
				}
				else if(state_disp_mode==1)
				{
					printf("Busy");
				}
				else
				{
					printf("Wait");
				}
		}
		else if(strcmp(uart_rx_buf,"#")==0)
		{
				printf("(%u,%u)",location[0],location[1]);
		}
		else
		{
			printf("Error");
		}
		// 清空缓冲区
    memset(uart_rx_buf, 0, 10);
    uart_rx_index = 0;
	}
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
	speed_slow_down++;
	move_slow_down++;
	ad_slow_down++;
	dist_slow_down++;
	if(seg_buf[seg_pos]>=20)
		seg_disp(seg_pos,seg_buf[seg_pos]-',',1);
	else
		seg_disp(seg_pos,seg_buf[seg_pos],0);
	seg_pos=(++seg_pos)%8;
	if(++freq_time_1s==1000)
	{
		freq_time_1s=0;
		freq=TH0<<8|TL0;
		TH0=TL0=0;
		speed_update_flag=1;
	}
	if(uart_rx_flag) uart_rx_tick++;
	if(state_disp_mode==2)
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
	if(finish_flag)
	{
		L3_flag=1;
		if(++finish_time_3s==3000)
		{
			finish_time_3s=0;
			L3_flag=0;
			finish_flag=0;//3s后终点标志位置为0
		}
	}

	if(state_disp_mode==1)
	{
		if(++move_time_100ms==100)
		{
			move_time_100ms=0;
			count_100ms++;
		}
	}
	else
	{
		move_time_100ms=0;
		count_100ms=0;
	}
	led_disp(ucled);
}

void Uart1_Isr(void) interrupt 4
{
	if (RI)				//检测串口1接收中断
	{
		uart_rx_flag=1;
		uart_rx_tick=0;
		if(uart_rx_index<10)
		{
			uart_rx_buf[uart_rx_index++]=SBUF;
		}
		else
		{
			memset(uart_rx_buf,0,10);
			uart_rx_index=0;
			uart_rx_flag=0;
			uart_rx_tick=0;
		}
		RI = 0;			//清除串口1接收中断请求位
	}
}

void main()
{
	system_init();
	ad_read(0x01);
	Timer0_Init();
	Uart1_Init();
	Timer1_Init();
	while(1)
	{
		move_proc();
		speed_proc();
		key_proc();
		seg_proc();
		led_proc();
		get_ad();
		get_distance();
		uart_proc();
	}
}
