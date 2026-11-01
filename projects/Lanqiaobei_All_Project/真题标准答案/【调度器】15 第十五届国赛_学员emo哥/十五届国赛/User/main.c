#include "stdio.h"
#include "string.h"
#include "math.h"
#include "Timer.h"
#include "bsp_init.h"
#include "bsp_key.h"
#include "bsp_led.h"
#include "bsp_seg.h"
#include "bsp_iic.h"
#include "bsp_freq.h"
#include "bsp_ultro.h"
#include "bsp_uart.h"

//函数
void task_proc();
void seg_proc();
void key_proc();
void led_proc();
void uart_proc();
//数码管
unsigned char seg_buf[8]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
unsigned char seg_string[10];
unsigned char pos;
//按键
unsigned char key_down,key_up,key_val,key_old;
//led
unsigned char ucled[8]={0};
//串口专用变量
unsigned char uart_buf[10];
unsigned char uart_index;
unsigned char uart_flag;
unsigned char uart_ms;
//调度
unsigned char cnt_seg,cnt_key,cnt_led;
unsigned char cnt_ad;
unsigned int cnt_freq;
unsigned char cnt_dist;
unsigned int cnt_1000ms;
unsigned int cnt_500ms;
unsigned int cnt_3000ms=3000;
unsigned char cnt_100ms;
xdata unsigned char cnt_speed;

//自定义
xdata float Photo=0;
pdata unsigned int freq=0;
pdata unsigned char dist=0;
xdata unsigned char state=0;//设备状态
pdata unsigned char battel=0;
pdata unsigned char screen_flag=0;
pdata float R=1;
pdata char B_CT=0;
pdata float R_sure=1;
pdata char B_sure=0;
bit R_B;//参数选择
bit rec_fin;//接受到坐标
bit sun;
pdata  int x_cur=0;
pdata  int y_cur=0;
//终点坐标
pdata  unsigned int x_cur_f=0;
pdata  unsigned int y_cur_f=0;
pdata  unsigned int x_cur_f1=0;
pdata  unsigned int y_cur_f1=0;
pdata int char_read=0;
pdata int result=0;
xdata float dx=0;
xdata float dy=0;
pdata float speed=0;
pdata float speed_x=0;
pdata float speed_y=0;
pdata float time=0;
pdata unsigned int x_cur_last=0;
pdata unsigned int y_cur_last=0;
bit xsign;
bit ysign;
xdata 	float distance=0;
pdata unsigned int freq_pre=0;


void main()
{
	
	cls_per_init();
	UartInit();
	Timer0Init();
	Timer1Init();
	
	
	while(1)	
	{
   task_proc();				
	}
	
}

void tim_isr() interrupt 3
{
	if(uart_flag==1)
		uart_ms++;
	
	//记录运动时间
	if((cnt_1000ms<100)&&(state==1)) cnt_1000ms++;
	
	
	if(state==1)
	{
		if(cnt_1000ms==100)
		{
		  time++;
//			time=time*0.1;
			cnt_1000ms=0;
		}
	}	
	
	if(cnt_seg<30) cnt_seg++;
	if(cnt_key<10) cnt_key++;
	if(cnt_led<10) cnt_led++;
	
	if(cnt_ad<200) cnt_ad++;
	if(cnt_freq<1000) cnt_freq++;
	if(cnt_dist<200) cnt_dist++;
	if(cnt_speed<100) cnt_speed++;
	
	if(cnt_500ms<500) cnt_500ms++;
	if(cnt_3000ms<3000) cnt_3000ms++;
	if(cnt_100ms<100) cnt_100ms++;
	
	seg_disp(seg_buf,pos);
	if(++pos==8) pos=0;
	
	led_disp(ucled);
	
}

void task_proc()
{
	if(cnt_seg==30)
	{
		seg_proc();
		cnt_seg=0;
	}
	
	if(cnt_key==10)
	{
		key_proc();
		cnt_key=0;
	}
	
	if(cnt_led==10)
	{
		led_proc();
		cnt_led=0;
	}
	
	//不需要减速
	uart_proc();
	//
	
	if(cnt_ad==200)
	{
		Photo=(pcf8591_adc(0x41)/51.0);
		//日间，夜间场景
		if(Photo*10>12)
			sun=1;//日间
		else
			sun=0;//夜间
		
		cnt_ad=0;
	}
	
	if(cnt_freq==1000)
	{
		freq_pre=freq;
		
		
		freq=((TH0<<8)|TL0);
		
		if(freq-freq_pre>200)
		{
		if(state==1)
		{
		x_cur_last=x_cur;
		y_cur_last=y_cur;
		
		time=0;
		}
		
		speed=3.14*R_sure*freq/100.0+B_sure;
		speed_x=speed*sqrt((dx*dx)/(dx*dx+dy*dy));
		speed_y=speed*sqrt((dy*dy)/(dx*dx+dy*dy));
	 }
		
	 
		TH0=TL0=0;
		cnt_freq=0;
	}
	
	if(cnt_speed==100)
	{
		cnt_speed=0;
	}
	
	if(cnt_dist==200)
	{
		dist=wave_dist()+3;
		//避障功能
		if((dist<=30)&&(state==1))
		{
			battel=1;//前方有障碍
			state=2;//等待状态
		}
		else if(dist>30)
			battel=0;//前方无障碍
		
		cnt_dist=0;
	}
	
	//运动状态处理
	
if(cnt_500ms==500)
{
	if((state==1)&&((x_cur!=x_cur_f)||(y_cur!=y_cur_f)))
	{		
		
//		if(xsign==0)
		x_cur=x_cur_last+(speed_x*(time))*0.1;
//		else
//		x_cur=x_cur_last-(speed_x*(time));	
//		if(ysign==0)
		y_cur=y_cur_last+(speed_y*(time))*0.1;
//		else
//		y_cur=y_cur_last-(speed_y*(time));	
		
		
//		if((xsign==0)&&(ysign==0))
//		{
			if((x_cur>=x_cur_f)&&(y_cur>=y_cur_f))
			{
				x_cur_last=x_cur_f;
				y_cur_last=y_cur_f;
				
				x_cur=x_cur_f;
				y_cur=y_cur_f;
				
				cnt_3000ms=0;
				cnt_1000ms=0;
				rec_fin=0;//可以再次接收数据
				time=0;//运动时间清0
				state=0;//恢复到空闲状态
			}
//	 }
//	 else if((xsign==1)&&(ysign==0))
//	 {
//			if((x_cur<=x_cur_f)&&(y_cur>=y_cur_f))
//			{
//				x_cur_last=x_cur_f;
//				y_cur_last=y_cur_f;
//				
//				x_cur=x_cur_f;
//				y_cur=y_cur_f;
//				
//				cnt_3000ms=0;
//				rec_fin=0;//可以再次接收数据
//				time=0;//运动时间清0
//				state=0;//恢复到空闲状态
//			}
//			
//	 }
//	 	else if((xsign==0)&&(ysign==1))
//	 {
//			if((x_cur>=x_cur_f)&&(y_cur<=y_cur_f))
//			{
//				x_cur_last=x_cur_f;
//				y_cur_last=y_cur_f;
//				
//				x_cur=x_cur_f;
//				y_cur=y_cur_f;
//				
//				cnt_3000ms=0;
//				rec_fin=0;//可以再次接收数据
//				time=0;//运动时间清0
//				state=0;//恢复到空闲状态
//			}
//			
//	 }
//	 	else if((xsign==1)&&(ysign==1))
//	 {
//			if((x_cur<=x_cur_f)&&(y_cur<=y_cur_f))
//			{
//				x_cur_last=x_cur_f;
//				y_cur_last=y_cur_f;
//				
//				x_cur=x_cur_f;
//				y_cur=y_cur_f;
//				
//				cnt_3000ms=0;
//				rec_fin=0;//可以再次接收数据
//				time=0;//运动时间清0
//				state=0;//恢复到空闲状态
//			}
//			
//	 }
	 
	}
	cnt_500ms=0;
}
	
	//继电器控制
	if(state==1)
		relay(1);
	else
		relay(0);
	
	
}

void uart_proc()
{
	if(uart_index==0) return;
	
	if(uart_ms>10)
	{
		uart_flag=0;
		uart_ms=0;
		//处理缓存区数据
		result=sscanf(uart_buf,"(%u,%u)%n",&x_cur_f1,&y_cur_f1,&char_read);
		
		if((result==2)&&(char_read==strlen(uart_buf))&&(state==0))
		{
			//发送坐标成功
			rec_fin=1;
			x_cur_f=x_cur_f1;
			y_cur_f=y_cur_f1;
			
			dx=abs(x_cur_f-x_cur);
			dy=abs(y_cur_f-y_cur);
			
//			if((x_cur_f-x_cur)<0)
//			{
//				xsign=1;
//			}
//			else
//				xsign=0;
//			
//			if((y_cur_f-y_cur)<0)
//			{
//				ysign=1;
//			}
//			else
//				ysign=0;
			
			speed_x=speed*sqrt((dx*dx)/(dx*dx+dy*dy));
		  speed_y=speed*sqrt((dy*dy)/(dx*dx+dy*dy));
			
			printf("Got it");
		}	
		else if((result==2)&&(char_read==strlen(uart_buf))&&(state==1))
			printf("Busy");
		else if((result==2)&&(char_read==strlen(uart_buf))&&(state==2))
			printf("Wait");
		else if(strcmp(uart_buf,"?")==0)
		{
			if(state==0)
			printf("Idle");
			else if(state==1)
			printf("Busy");
			else if(state==2)
			printf("Wait");
		}
		else if(strcmp(uart_buf,"#")==0)
		{
			printf("(%u,%u)",x_cur,y_cur);
		}
		else
			printf("Error");
		
	 //清空缓存区数据
	 memset(uart_buf,0,uart_index);
	 uart_index=0;		
	}
	
	
}

void uart_isr() interrupt 4
{
	if(RI)
	{
		uart_flag=1;
		uart_ms=0;
		uart_buf[uart_index++]=SBUF;
		RI=0;
		
		if(uart_index>10)
		{
			memset(uart_buf,0,10);
			uart_index=0;
		}
		
	}
}


void seg_proc()
{
	
	switch(screen_flag)
	{
		case 0:
			if(state==0)
			sprintf(seg_string,"L%3u-%3u",(unsigned int)x_cur,(unsigned int)y_cur);
			else
			sprintf(seg_string,"L%3u-%3u",(unsigned int)x_cur_f,(unsigned int)y_cur_f);
		break;
			
		case 1:
			if(state==0)
			sprintf(seg_string,"E2 -----");
			else if(state==1)
			sprintf(seg_string,"E1 %6.1f",speed);
			else
			sprintf(seg_string,"E3 %5u",(unsigned int)dist);	
		break;
		
		case 2:
			sprintf(seg_string,"P %3.1f %3d",R,(int)B1);
		break;	
	}
	seg_tran(seg_string,seg_buf);
}

void key_proc()
{
	
	key_val=key_read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	
	switch(key_down)
	{
		case 4:
			if(state==0)//空闲状态
			{
				if(rec_fin==1)//接受到目的地坐标
				{
					state=1;//运动状态	
				}	 
			}
			else if(state==2)//等待状态
			{
				if(battel==0)//障碍物已清除
				{
					state=1;//运动状态
				}
				else
					state=2;
			}
			else if(state==1)//运行状态
			{
				state=2;//等待状态
			}
		break;
			
		case 5:
			if(state==0)
			{
				//置零当前位置
				x_cur=0;
				y_cur=0;
//				xsign=ysign=0;
				x_cur_last=0;
				y_cur_last=0;
				rec_fin=1;
//			//重新设置速度
//			dx=(x_cur_f-x_cur);
//			dy=(y_cur_f-y_cur);
//				
//			if((dx==0)&&(dy==0))
//				return ;
//			
//			distance = sqrt(dx * dx + dy * dy);
//			
//			speed_x=speed*(((dx)*1.0)/distance);
//		  speed_y=speed*(((dy)*1.0)/distance);
			}
		break;
			
		case 8:
			if(++screen_flag==3) screen_flag=0;
		  
		   if(screen_flag==2)
				 R_B=0;//每次进入为R参数
			 else if(screen_flag!=2)
			 {
				 //退出时生效
				 R_sure=R;
				 B_sure=B_CT;
			 }
    break;		
		
		case 9:
		if(screen_flag==2)
		{
			if(R_B==0)
				R_B=1;//b参数
			else
				R_B=0;
	   }
		break;
		 
		case 12:
      if(screen_flag==2)
			{
				if(R_B==0)
				{
					R+=0.1;
				if((R*10)==21)	
					R=2;
				}
				
				else
				{
					  B_CT+=5;
					if(B_CT==95)
						B_CT=90;
				}
			}
    break;	

    case 13:
    if(screen_flag==2)
		{
				if(R_B==0)
				{
					R-=0.1;
					if((R*10)==9)	
					R=1;
				}

				else
				{
					B_CT-=5;
					if(B_CT==-95)
					B_CT=-90;
				}
		 }
    break;	
		 
	}
	
}

void led_proc()
{
	if(state==0)
		ucled[0]=0;
	else if(state==1)
		ucled[0]=1;
	else
	{
		if(cnt_100ms==100)
		{
			ucled[0]^=1;
			cnt_100ms=0;
		}
	}
	
	if(state==1)
	{
		if(sun==1)
			ucled[1]=0;
		else
			ucled[1]=1;
	}
	else
		ucled[1]=0;
	
	
	if(cnt_3000ms<3000)
	{
		ucled[2]=1;
	}
	else
		ucled[2]=0;
	
}
