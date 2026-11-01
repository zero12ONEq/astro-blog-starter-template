#include <STC15F2K60S2.H>
#include "stdio.h"
#include "string.h"
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "ds1302.h"
#include "onewire.h"
#include "iic.h"
#include "ultrasound.h"
#include "uart.h"
//频率
idata unsigned int freq;
idata unsigned int temp_freq;
bit got_freq_flag;//已经获得频率标志位
idata unsigned int freq_sub;
idata unsigned int freq_time_1s;
pdata unsigned int freq_compare[5]={0};
idata unsigned char freq_index;
bit change_error_flag;//剧烈变化异常标志位
bit num_error_flag;//数值异常标志位
bit qushi_error_flag;//趋势异常标志位
//串口
bit uart_rx_flag;
idata unsigned char uart_rx_tick;
pdata unsigned char uart_rx_buf[64]={0};
idata unsigned char uart_rx_index;
//按键
idata unsigned char key_val,key_old,key_down,key_up;
idata unsigned char key_slow_down;
//数码管
idata unsigned char seg_pos;
pdata unsigned char seg_buf[]={10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down;
//时间
pdata unsigned char ucrtc[]={23,59,50}; 
//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
//ad
idata unsigned char ad_slow_down;
//温度
idata unsigned int T_slow_down;
//dist
idata unsigned char dist_slow_down;
//异常
bit last_502;
idata unsigned char last_503=0xff;
bit last_504;
//参数
idata unsigned int F;
idata unsigned char S;
idata unsigned int H1_cm;
idata unsigned int H2_cm;
idata unsigned int r_cm;
idata unsigned int L_cm;
idata unsigned int W_cm;
idata unsigned int H_cm;
void key_proc()
{
	if(key_slow_down<10) return;
	key_slow_down=0;
	key_val=key_read();
	key_down=key_val&(key_val^key_old);
	key_up=~key_val&(key_val^key_old);
	key_old=key_val;
}


void seg_proc()
{
	if(seg_slow_down<80) return;
	seg_slow_down=0;

}


void led_proc()
{



}


void ad_proc()
{
	if(ad_slow_down<100) return;
	ad_slow_down=0;

}


void T_proc()
{
	if(T_slow_down<300) return;
	T_slow_down=0;

}


void dist_proc()
{
	if(dist_slow_down<100) return;
	dist_slow_down=0;

}

void freq_proc()
{
		if(got_freq_flag==0) return;
		got_freq_flag=0;
		read_rtc(ucrtc);
		if(freq>F)
		{
			num_error_flag=1;
			if(last_502==0&&uart_rx_index==0&&uart_rx_flag==0)
			{
				last_502=1;
				printf("[502,%02bu%02bu%02bu,%u]",ucrtc[0],ucrtc[1],ucrtc[2],freq);
			}
		}
		else
		{
			num_error_flag=0;
			last_502=0;
		}
		
		if(freq_index>=5)
		{
			if(freq_compare[0]<freq_compare[1]&&
				 freq_compare[1]<freq_compare[2]&&
			   freq_compare[2]<freq_compare[3]&& 
			   freq_compare[3]<freq_compare[4])
			{
				qushi_error_flag=1;
				if(last_503!=1&&&uart_rx_index==0&&uart_rx_flag==0)
				{
					last_503=1;
					printf("[503,%02bu%02bu%02bu,1]",ucrtc[0],ucrtc[1],ucrtc[2]);		
				}
			
			}
			else if(freq_compare[0]>freq_compare[1]&&
				      freq_compare[1]>freq_compare[2]&&
			        freq_compare[2]>freq_compare[3]&& 
			        freq_compare[3]>freq_compare[4])
			{
				qushi_error_flag=1;
				if(last_503!=0&&&uart_rx_index==0&&uart_rx_flag==0)
				{
					last_503=0;
					printf("[503,%02bu%02bu%02bu,0]",ucrtc[0],ucrtc[1],ucrtc[2]);		
				}
			
			}	
			else
			{
				qushi_error_flag=0;
				last_503=0xff;
			}
			freq_index=0;
		}
		if(change_error_flag)
		{
			if(last_504==0&&uart_rx_index==0&&uart_rx_flag==0)
			{
				printf("[504,%02bu%02bu%02bu,%u]",ucrtc[0],ucrtc[1],ucrtc[2],freq_sub);
			}
		}
		else
		{
			last_504=0;
		}

}
bit str_eq(unsigned char*a,unsigned char*b)
{
	unsigned char i;
	while(a[i]!='\0'&&b[i]!='\0')
	{
		if(a[i]!=b[i])
		{
			return 0;
		}
		i++;
	}
	return (a[i]==b[i]);
}

unsigned int parse_m_to_cm(unsigned char*str)
{
	unsigned char i;
	unsigned char dot=0xff;
	unsigned int integer=0,docimal=0;
	while(str[i]!='\0')
	{
		if(str[i]=='.')
		{
			dot=i;
			break;
		}
		i++;
	}
	i=0;
	if(dot!=0xff)
	{
		while(i<dot)
		{
			integer=integer*10+(str[i]-'0');
			i++;
		}
		i++;
		if(str[i]>='0'&&str[i]<='9')
		{
			docimal=str[i]-'0';
		}
	}
	else
	{
		while(str[i]!='\0')
		{
			integer=integer*10+(str[i]-'0');
		}
	}
	return integer*100+docimal*10;
}

unsigned int parse_int(unsigned char*str)
{
	unsigned char i=0;
	unsigned int integer=0;
	while(str[i]>='0'&&str[i]<='9')
	{
		integer=integer*10+(str[i]-'0');
		i++;
	}
	return integer;
}

unsigned char extract_one(unsigned char start,unsigned char*id,unsigned char*val)
{
	unsigned char i=start,j=0;
	while(i<uart_rx_index&&uart_rx_buf[i]!='(') i++;
	if(i>=uart_rx_index)
	{
		return 0xff;
	}
	i++;
	j=0;
	while(i<uart_rx_index&&uart_rx_buf[i]!=','&&uart_rx_buf[i]!=':')
	{
		id[j++]=uart_rx_buf[i++];
	}
	id[j]='\0';
	if(i>=uart_rx_index)
	{
		return 0xff;
	}
	i++;
	j=0;
	while(i<uart_rx_index&&uart_rx_buf[i]!=')')
	{
		val[j++]=uart_rx_buf[i++];
	}
	val[j]='\0';
	if(i>=uart_rx_index)
	{
		return 0xff;
	}
	return i;
}


void handle_param(unsigned char*id,unsigned char*val,unsigned char*ok,unsigned char*err)
{
		unsigned int t;
		if(str_eq(val,"?"))
		{
			if(str_eq(id,"H1")){printf("(H1,%u.%u)",H1_cm/100,H1_cm/10%10);*ok=1;}
			else if(str_eq(id,"H2")){printf("(H2,%u.%u)",H2_cm/100,H2_cm/10%10);*ok=1;}
			else if(str_eq(id,"F")){printf("(F,%u)",F);*ok=1;}
			else if(str_eq(id,"S")){printf("(S,%bu)",S);*ok=1;}
			else if(str_eq(id,"r")){printf("(r,%u.%u)",r_cm/100,r_cm/10%10);*ok=1;}
			else if(str_eq(id,"L")){printf("(L,%u.%u)",L_cm/100,L_cm/10%10);*ok=1;}
			else if(str_eq(id,"W")){printf("(W,%u.%u)",W_cm/100,W_cm/10%10);*ok=1;}
			else if(str_eq(id,"H")){printf("(H,%u.%u)",H_cm/100,H_cm/10%10);*ok=1;}
			else *err=1;
			
			return;
		}
		if(str_eq(id,"T"))
		{
			unsigned char k;
			unsigned char h,m,s;
			for(k=0;k<6;k++)
			{
				if(val[k]<='0'||val[k]>='9')
				{
					*err=1;
					return;
				}
			}
			h=(val[0]-'0')*10+(val[1]-'0');
			m=(val[2]-'0')*10+(val[3]-'0');
			s=(val[4]-'0')*10+(val[5]-'0');
			if(h>23||m>59||s>59)
			{
				*err=1;
				return;
			}
			ucrtc[0]=h;ucrtc[1]=m;ucrtc[2]=s;
			set_rtc(ucrtc);
			*ok=1;
			return;
		}
		
		if(str_eq(id,"H1")) {t=parse_m_to_cm(val); if(t<H2_cm){*err=1; return;} H1_cm=t;*ok=1;return;}
		if(str_eq(id,"H2")) {t=parse_m_to_cm(val); if(t>H1_cm){*err=1; return;} H2_cm=t;*ok=1;return;}
		if(str_eq(id,"F")){F=parse_int(val);*ok=1;return;}
		if(str_eq(id,"S")){t=parse_int(val);if(t>2){*err=1;return;} S=(unsigned char)t;ok=1;return;}
		if(str_eq(id,"r")) {t=parse_m_to_cm(val); if(t==0){*err=1; return;} r_cm=t; *ok=1; return;}
		if(str_eq(id,"L")) {t=parse_m_to_cm(val); if(t==0){*err=1; return;} L_cm=t; *ok=1; return;}
		if(str_eq(id,"W")) {t=parse_m_to_cm(val); if(t==0){*err=1; return;} W_cm=t; *ok=1; return;}
		if(str_eq(id,"H")) {t=parse_m_to_cm(val); if(t==0){*err=1; return;} H_cm=t; *ok=1; return;}
}
void uart_proc()
{
	unsigned char pos=0;
	unsigned char id[5];
	unsigned char val[10];
	unsigned char ok=0,err=0;
	if(uart_rx_index==0) return;
	if(uart_rx_tick>=10)
	{
		uart_rx_tick=0;
		uart_rx_flag=0;
		while(pos<uart_rx_index)
		{
			pos=extract_one(pos,id,val);
			if(pos==0xff) {err=1; break;}
			handle_param(id,val,&ok,&err);
			pos++;
		}
		if(ok) printf("OK");
		else if(ok&&err) printf("WARN");
		else printf("ERROR");
		
		uart_rx_index=0;
		memset(uart_rx_buf,0,64);
	}

}


void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			//设置定时器模式
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
	ad_slow_down++;
	T_slow_down++;
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
		got_freq_flag=1;
		if(temp_freq!=0)
		{
			if(temp_freq>=freq) freq_sub=temp_freq-freq;
			else freq_sub=freq-temp_freq;
			if(freq_sub>1000)
			{
				change_error_flag=1;
			}
			else
			{
				change_error_flag=0;
			}
			temp_freq=freq;
		}
		else
		{
			temp_freq=freq;
		}
		
		if(freq_index<5)
		{
			freq_compare[freq_index++]=freq;
		}
		TH0=TL0=0;
	}
	if(uart_rx_flag) uart_rx_tick++;
	led_disp(ucled);
}

void Uart1_Isr(void) interrupt 4
{
	if (RI)				//检测串口1接收中断
	{
		uart_rx_flag=1;
		uart_rx_tick=0;
		if(uart_rx_index<64)
			uart_rx_buf[uart_rx_index++]=SBUF;
		else
		{
			uart_rx_index=0;
			memset(uart_rx_buf,0,64);
			uart_rx_flag=0;
			uart_rx_tick=0;
		}
		RI = 0;			//清除串口1接收中断请求位
	}
}
void main()
{
	system_init();
	Timer0_Init();
	Uart1_Init();
	Timer1_Init();
	while(1)
	{
		key_proc();
		seg_proc();
		led_proc();
		ad_proc();
		T_proc();
		dist_proc();
		uart_proc();
	}
}
