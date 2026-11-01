#include <STC15F2K60S2.H>
#include "stdio.h"
#include "string.h"
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "ds1302.h"
#include "iic.h"
#include "uart.h"
//串口
bit uart_rx_flag;
idata unsigned char uart_rx_tick;
pdata unsigned char uart_rx_buf[]={0,0,0,0,0,0,0,0,0,0};
idata unsigned char uart_rx_index;
//按键
idata unsigned char key_val,key_old,key_down,key_up;
idata unsigned char key_slow_down;
bit key5_long_flag;
idata unsigned int key5_long_tick;
bit key13_long_flag;
idata unsigned int key13_long_tick;
bit key5_long_done;   // 本次按住的长按事件是否已处理
bit key13_long_done;  // 同上，给 S13 用
//数码管
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down;
idata unsigned char seg_disp_mode=0; //0-运行界面 1-地址设置界面 2-泄压记录界面 3-工厂界面
bit factory_disp_mode;//0-满量程校准值设置 1-零点校准值设置
pdata unsigned char old_disp_mode;//旧的seg_disp_mode;
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
bit L1_enable_flag;
bit L1_flash_flag;
idata unsigned char L1_flash_tick;	
idata unsigned int L1_3s_tick;
bit L2_flash_flag;
idata unsigned char L2_flash_tick;	
//ad
idata unsigned char ad_slow_down;
//时间
pdata unsigned char ucrtc[]={22,03,00};
//压力
idata unsigned char yali_val_10x;//单位kPa
idata unsigned char qian_yali_val_10x;//3s前压力值
idata unsigned char hou_yali_val_10x;//3s后压力值
bit xieya_flag;//发生泄压事件标志位 1-发生 0-未发生
pdata unsigned char xieya_time[]={0,0,0};
idata unsigned char full_ct_jiaozhun_10x;//满量程校准值控制值10倍 0.0 - 9.9kPa (0-99)
idata unsigned char zero_ct_jiaozhun_10x;//零点校准值控制值10倍：0.0 -9.9kPa(0-99)
idata unsigned char full_jiaozhun_10x=90;//满量程校准值10倍 0.0 - 9.9kPa (0-99)
idata unsigned char zero_jiaozhun_10x=0;//零点校准值10倍：0.0 -9.9kPa(0-99)
idata unsigned int yali_3s_tick;
bit got_yali_flag;
bit xieya_pingbi_flag;//1屏蔽 2不屏蔽
idata unsigned int xieya_pingbi_tick;//3s
//地址
idata unsigned char address=8;//设备地址：1-100
idata unsigned char eeprom_temp;
void key_proc()
{
	if(key_slow_down<10) return;
	key_slow_down=0;
	key_val=key_read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
	if(seg_disp_mode!=3)
	{
		if(key_down==4)
		{
			seg_disp_mode=(++seg_disp_mode)%3;
		}
	}
	
	
	if(key5_long_tick>=3000 && !key5_long_done) //持续长按超过3s，只触发一次，不会出现一直长按然后在进入与退出工厂模式之间来回切换
	{
		key5_long_done=1;
		if(seg_disp_mode!=3)
		{
			old_disp_mode=seg_disp_mode;
			factory_disp_mode=0;
			full_ct_jiaozhun_10x=full_jiaozhun_10x;
			zero_ct_jiaozhun_10x=zero_jiaozhun_10x;
			seg_disp_mode=3;
		}
		else
		{
			seg_disp_mode=old_disp_mode;
			if((full_ct_jiaozhun_10x!=full_jiaozhun_10x)||(zero_ct_jiaozhun_10x!=zero_jiaozhun_10x))
			{
				xieya_pingbi_flag=1;
				got_yali_flag=0; //不让yali_3s_tick在屏蔽的时候仍然自增
				yali_3s_tick=0; 
			}
			full_jiaozhun_10x=full_ct_jiaozhun_10x;
			zero_jiaozhun_10x=zero_ct_jiaozhun_10x;
			
		}
	}
	
	if(seg_disp_mode==3 && key_up==5 && !key5_long_done) // 短按判定：必须在 key5_long_done 被清零之前 防止进入工厂界面后里面跳转零校准值界面
	{
		factory_disp_mode=!factory_disp_mode;
	}
	
	if(key_old==5)
	{
		key5_long_flag=1;
	}
	else
	{
		key5_long_flag=0;
		key5_long_tick=0;
		key5_long_done = 0;   // 松手才清"已处理长按标志位"
	}
	
	if(key_down==8)
	{
		if(seg_disp_mode==1)
		{
			if(address>1) address--;
		}
		else if(seg_disp_mode==3)
		{
			if(factory_disp_mode==0) 
			{
				if(full_ct_jiaozhun_10x>0)
				{
					full_ct_jiaozhun_10x--;
				}
			}
			else
			{
				if(zero_ct_jiaozhun_10x>0)
				{
					zero_ct_jiaozhun_10x--;
				}				
			}
		}
	}
	
	if(key_down==9)
	{
		if(seg_disp_mode==1)
		{
			if(address<100) address++;
		}
		else if(seg_disp_mode==3)
		{
			if(factory_disp_mode==0) 
			{
				if(full_ct_jiaozhun_10x<99)
				{
					full_ct_jiaozhun_10x++;
				}
			}
			else
			{
				if(zero_ct_jiaozhun_10x<99)
				{
					zero_ct_jiaozhun_10x++;
				}				
			}
		}
	}
	
	if(key_down==12)
	{
		if(seg_disp_mode==1)
		{
			eeprom_write(&address,0x03,1);
		}
		else if(seg_disp_mode==3)
		{
			if(factory_disp_mode==0) 
			{

				eeprom_write(&full_ct_jiaozhun_10x,0x02,1);
			}
			else
			{
				eeprom_write(&zero_ct_jiaozhun_10x,0x01,1);
			}
		}
	}
	
	if(seg_disp_mode==2&&key_up==13 && !key13_long_done) //短按
	{
		xieya_time[0]=xieya_time[1]=xieya_time[2]=0;
		xieya_flag=0;
	}
	
	if(key13_long_tick>=3000 && !key13_long_done) //长按s13只处理一次，防止一直按着每隔3s，eeprom频繁写入
	{
		key13_long_done=1;//只有长按3s以上才会置为1
		full_jiaozhun_10x=90;
		zero_jiaozhun_10x=0;
		full_ct_jiaozhun_10x=90;//同步控制量，防止泄压屏蔽判定误触
		zero_ct_jiaozhun_10x=0;
		address=8;
		eeprom_write(&full_jiaozhun_10x,0x02,1);
		eeprom_write(&zero_jiaozhun_10x,0x01,1);
		eeprom_write(&address,0x03,1);
		seg_disp_mode=0;
	}
	if(key_old==13)
	{
		key13_long_flag=1;
	}
	else
	{
		key13_long_flag=0;
		key13_long_tick=0;
		key13_long_done = 0;   // 松手清空长按已处理标志位
	}
	
}


void seg_proc()
{
	if(seg_slow_down<80) return;
	seg_slow_down=0;
	read_rtc(ucrtc);
	switch(seg_disp_mode) //0-运行界面 1-地址设置界面 2-泄压记录界面 3-工厂界面
	{
		case 0: 
			seg_buf[0]=ucrtc[0]/10%10;
			seg_buf[1]=ucrtc[0]%10;
			seg_buf[2]=ucrtc[1]/10%10;
			seg_buf[3]=ucrtc[1]%10;
			seg_buf[4]=seg_buf[5]=10;
			seg_buf[6]=yali_val_10x/10%10+',';
			seg_buf[7]=yali_val_10x%10;
			break;
		case 1:
			seg_buf[0]=11;
			seg_buf[1]=12;
			seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
			seg_buf[5]=(address>=100)?address/100%10:10;
			seg_buf[6]=(address>=10)?address/10%10:10;
			seg_buf[7]=address%10;
			break;
		case 2:		
			seg_buf[0]=13;
			seg_buf[1]=13;
			seg_buf[2]=seg_buf[3]=10;
			if(xieya_flag)
			{
				seg_buf[4]=xieya_time[0]/10%10;
				seg_buf[5]=xieya_time[0]%10;
				seg_buf[6]=xieya_time[1]/10%10;
				seg_buf[7]=xieya_time[1]%10;
			}
			else
			{
				seg_buf[4]=seg_buf[5]=seg_buf[6]=10;
				seg_buf[7]=17;
			}
			break;
		case 3: //0-满量程校准值设置 1-零点校准值设置
			seg_buf[0]=14;
			seg_buf[2]=seg_buf[3]=seg_buf[4]=10;
			seg_buf[5]=17;
			if(factory_disp_mode==0)
			{
				seg_buf[1]=15;
				seg_buf[6]=full_ct_jiaozhun_10x/10%10+',';
				seg_buf[7]=full_ct_jiaozhun_10x%10;
			}
			else
			{
				seg_buf[1]=16;
				seg_buf[6]=zero_ct_jiaozhun_10x/10%10+',';
				seg_buf[7]=zero_ct_jiaozhun_10x%10;				
			}
			break;
	
	}

}

void led_proc()
{
	ucled[0]=L1_flash_flag;
	ucled[1]=L2_flash_flag;
	ucled[2]=(xieya_flag==1);
}

void ad_proc()
{
	float temp1;
	float temp2;
	if(ad_slow_down<100) return;
	ad_slow_down=0;
	ad_read(0x03);
	temp1=ad_read(0x03)/51.0;
	temp2=((full_jiaozhun_10x-zero_jiaozhun_10x)/10.0)/5.0;//斜率
	yali_val_10x=(unsigned char)((temp2*temp1)*10+zero_jiaozhun_10x);
	
	if(xieya_pingbi_flag==0)
	{
		if(got_yali_flag==0)
		{
			got_yali_flag=1;
			qian_yali_val_10x=yali_val_10x;
		}
		if(yali_3s_tick==3000)
		{
			yali_3s_tick=0;
			got_yali_flag=0;
			hou_yali_val_10x=yali_val_10x;
			if((int)hou_yali_val_10x-(int)qian_yali_val_10x<(-15)) //unsigned char相减永远不可能为负数，转化为int型。
			{
				xieya_flag=1;
				read_rtc(xieya_time);
			}
		}
	}
	
}

void uart_proc()
{
	int result,num;
	unsigned char temp_address;
	float yali_val;
	if(uart_rx_index==0) return;
	if(uart_rx_tick>=10)
	{
		uart_rx_tick=0;
		uart_rx_flag=0;
		result=sscanf(uart_rx_buf,"#%bu?%n",&temp_address,&num);
		if(result==1&&num==strlen(uart_rx_buf)&&temp_address==address)
		{
			L1_enable_flag=1;
			L1_flash_tick=0;
			L1_3s_tick=0;
			yali_val=(float)yali_val_10x/10.0;
			printf("%.1fkPa@%02bu:%02bu",yali_val,ucrtc[0],ucrtc[1]);
		}
		memset(uart_rx_buf,0,10);
		uart_rx_index=0;
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
	EA  = 1;        //使能全局中断
}

void Timer1_Isr(void) interrupt 3
{
	key_slow_down++;
	seg_slow_down++;
	ad_slow_down++;
	if(seg_buf[seg_pos]>=20)
		seg_disp(seg_pos,seg_buf[seg_pos]-',',1);
	else
		seg_disp(seg_pos,seg_buf[seg_pos],0);
	seg_pos=(++seg_pos)%8;
	
	if(key5_long_flag)
	{
		if(++key5_long_tick>=3000) key5_long_tick=3001;
	}
	else
	{
		key5_long_tick=0;
	}

	if(key13_long_flag)
	{
		if(++key13_long_tick>=3000) key13_long_tick=3001;
	}
	else
	{
		key13_long_tick=0;
	}	
	if(got_yali_flag) yali_3s_tick++;
	
	if(xieya_pingbi_flag)
	{
		if(++xieya_pingbi_tick==3000)
		{
			xieya_pingbi_tick=0;
			xieya_pingbi_flag=0;
		}
	}
	if(uart_rx_flag) uart_rx_tick++;
	if(L1_enable_flag)
	{
		if(++L1_flash_tick==200)
		{
			L1_flash_tick=0;
			L1_flash_flag=!L1_flash_flag;
		}
		if(++L1_3s_tick==3000)
		{
			L1_3s_tick=0;
			L1_flash_tick=0;
			L1_flash_flag=0;
			L1_enable_flag=0;
		}
	}
	if(seg_disp_mode==3)
	{
		if(++L2_flash_tick==200)
		{
			L2_flash_tick=0;
			L2_flash_flag=!L2_flash_flag;
		}
	}
	else
	{
		L2_flash_tick=0;
		L2_flash_flag=0;
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
	set_rtc(ucrtc);
	
	eeprom_read(&eeprom_temp,0x00,1);
	if(eeprom_temp!=0XAA) //首次上电
	{
		eeprom_temp=0XAA;
		eeprom_write(&full_jiaozhun_10x,0x02,1);
		eeprom_write(&zero_jiaozhun_10x,0x01,1);
		eeprom_write(&address,0x03,1);
		eeprom_write(&eeprom_temp,0x00,1);
	}
	eeprom_read(&full_jiaozhun_10x,0x02,1);
	eeprom_read(&zero_jiaozhun_10x,0x01,1);
	eeprom_read(&address,0x03,1);	
	Uart1_Init();
	Timer1_Init();
	while(1)
	{
		key_proc();
		seg_proc();		
		ad_proc();
		uart_proc();
		led_proc();
	}
}
