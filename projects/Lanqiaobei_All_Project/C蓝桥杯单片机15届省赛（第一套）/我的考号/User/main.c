#include <STC15F2K60S2.H>
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "ds1302.h"
#include "iic.h"
//频率
idata unsigned int Freq;//频率测量值
idata int Freq_Max=0;//最大频率值
idata unsigned int Para_Freq=2000;//频率超限参数1000-9000Hz
idata int Para_Adjust=0;//频率校准值 -900~900Hz
idata int Freq_Final;//频率数据最终结果 =频率测量值+频率校准值
idata unsigned int Time_1s;
//按键
idata unsigned char Key_Val,Key_Old,Key_Down,Key_Up;
idata unsigned char Key_Slow_Down;

//数码管
idata unsigned char Seg_Pos=0;
pdata unsigned char Seg_Buf[]={10,10,10,10,10,10,10,10};
idata unsigned char Seg_Slow_Down;
idata unsigned char Seg_Disp_Mode;//0-频率界面 1-参数界面 2-时间界面 3-回显界面
idata bit Para_Disp_Mode;//参数界面子界面 0-超限参数界面 1-校准值参数界面
idata bit Back_Disp_Mode;//回显界面子界面 0-频率回显界面 1-时间回显界面

//led
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata bit Led1_Flash_Flag;
idata bit Led2_Flash_Flag;
idata unsigned char Time1_200ms;//L1闪烁计时器
idata unsigned char Time2_200ms;//L2闪烁计时器

//时间
pdata unsigned char ucRtc[]={13,3,5};//当前时间
pdata unsigned char max_freq_time[3];//最大频率产生的时间

void Key_Proc()
{
	if(Key_Slow_Down<10) return;
	Key_Slow_Down=0;
	
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Val^Key_Old);
	Key_Up=~Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;
	
	if(Key_Down==4)
	{
		Seg_Disp_Mode=(++Seg_Disp_Mode)%4;
		Para_Disp_Mode=Back_Disp_Mode=0;//每次切换界面时，重置子界面
	}
	if(Key_Down==5)
	{
		if(Seg_Disp_Mode==1)
			Para_Disp_Mode=!Para_Disp_Mode;
		else if(Seg_Disp_Mode==3)
			Back_Disp_Mode=!Back_Disp_Mode;
	}
	if(Seg_Disp_Mode==1)
	{
		if(Para_Disp_Mode==0)
		{
			if(Key_Down==8)
			{
				if(Para_Freq<9000) Para_Freq+=1000;
			}
			if(Key_Down==9)
			{
				if(Para_Freq>1000) Para_Freq-=1000;
			}
		}
		else
		{
			if(Key_Down==8)
			{
				if(Para_Adjust<900) Para_Adjust+=100;
			}
			if(Key_Down==9)
			{
				if(Para_Adjust>-900) Para_Adjust-=100;
			}			
		}
	}
}

void Seg_Proc()
{
	if(Seg_Slow_Down<80) return;
	Seg_Slow_Down=0;
	Read_Rtc(ucRtc);//读取时间
	switch(Seg_Disp_Mode)
	{
		case 0: //频率界面
			if(Freq_Final>=0)
			{
				Seg_Buf[0]=11;
				Seg_Buf[1]=10;
				Seg_Buf[2]=10;
				Seg_Buf[3]=(Freq_Final>=10000)?Freq_Final/10000%10:10;
				Seg_Buf[4]=(Freq_Final>=1000)?Freq_Final/1000%10:10;
				Seg_Buf[5]=(Freq_Final>=100)?Freq_Final/100%10:10;
				Seg_Buf[6]=(Freq_Final>=10)?Freq_Final/10%10:10;
				Seg_Buf[7]=Freq_Final%10;		
			}	
			else
			{
				Seg_Buf[0]=11;
				Seg_Buf[1]=10;
				Seg_Buf[2]=10;
				Seg_Buf[3]=10;
				Seg_Buf[4]=10;
				Seg_Buf[5]=10;
				Seg_Buf[6]=16;
				Seg_Buf[7]=16;						
			}
			break;
		case 1://参数界面
			Seg_Buf[0]=12;
			Seg_Buf[2]=10;
			Seg_Buf[3]=10;			
			if(Para_Disp_Mode==0) //超限参数界面
			{
				Seg_Buf[1]=1;
				Seg_Buf[4]=Para_Freq/1000%10;
				Seg_Buf[5]=Para_Freq/100%10;
				Seg_Buf[6]=Para_Freq/10%10;
				Seg_Buf[7]=Para_Freq%10;
			
			}
			else //校准值参数界面
			{
				Seg_Buf[1]=2;
				if(Para_Adjust>=0)  //大于等于0
				{
					Seg_Buf[4]=10;
					Seg_Buf[5]=(Para_Adjust>=100)?Para_Adjust/100%10:10;
					Seg_Buf[6]=(Para_Adjust>=10)?Para_Adjust/10%10:10;
					Seg_Buf[7]=Para_Adjust%10;						
				
				}
				else //小于0
				{
					if((-Para_Adjust)>=100)
					{
						Seg_Buf[4]=13;
						Seg_Buf[5]=(-Para_Adjust)/100%10;
						Seg_Buf[6]=(-Para_Adjust)/10%10;
						Seg_Buf[7]=(-Para_Adjust)%10;							
					}
					else if((-Para_Adjust)>=10)
					{
						Seg_Buf[4]=10;
						Seg_Buf[5]=13;
						Seg_Buf[6]=(-Para_Adjust)/10%10;
						Seg_Buf[7]=(-Para_Adjust)%10;							
					}
					else
					{
						Seg_Buf[4]=10;
						Seg_Buf[5]=10;
						Seg_Buf[6]=13;
						Seg_Buf[7]=(-Para_Adjust)%10;							
					}
						
				}
			}
			break;
		case 2: //时间界面
			Seg_Buf[0]=ucRtc[0]/10%10;
			Seg_Buf[1]=ucRtc[0]%10;
			Seg_Buf[2]=13;
			Seg_Buf[3]=ucRtc[1]/10%10;
			Seg_Buf[4]=ucRtc[1]%10;
			Seg_Buf[5]=13;
			Seg_Buf[6]=ucRtc[2]/10%10;
			Seg_Buf[7]=ucRtc[2]%10;					
			break;
		case 3: //回显界面
			Seg_Buf[0]=14;			
			if(Back_Disp_Mode==0)//频率回显界面
			{

				Seg_Buf[1]=11;
				Seg_Buf[2]=10;
				Seg_Buf[3]=(Freq_Max>=10000)?Freq_Max/10000%10:10;
				Seg_Buf[4]=(Freq_Max>=1000)?Freq_Max/1000%10:10;
				Seg_Buf[5]=(Freq_Max>=100)?Freq_Max/100%10:10;
				Seg_Buf[6]=(Freq_Max>=10)?Freq_Max/10%10:10;
				Seg_Buf[7]=Freq_Max%10;						
			}
			else //时间回显界面
			{
				Seg_Buf[1]=15;
				Seg_Buf[2]=max_freq_time[0]/10%10;
				Seg_Buf[3]=max_freq_time[0]%10;
				Seg_Buf[4]=max_freq_time[1]/10%10;
				Seg_Buf[5]=max_freq_time[1]%10;
				Seg_Buf[6]=max_freq_time[2]/10%10;
				Seg_Buf[7]=max_freq_time[2]%10;					
			}
			break;		
	}
}

void Led_Proc()
{
	if(Seg_Disp_Mode==0)
	{
		ucled[0]=Led1_Flash_Flag;
	}
	else
	{
		ucled[0]=0;
	}
	
	if(Freq_Final>Para_Freq)
	{
		ucled[1]=Led2_Flash_Flag;
	}
	else
	{
		ucled[1]=0;
	}
	
	if(Freq_Final<0)
	{
		ucled[1]=1;
	}
	
}

void AD_DA()
{
	idata float k;
	idata float b;
	if(Freq_Final<0)
	{
		Da_Write(0*51);
	}
	else if(Freq_Final<=500)
	{
		Da_Write(1*51);
	}
	else if(Freq_Final>500&&Freq_Final<Para_Freq)
	{
		k=4.0/(Para_Freq-500);
		b=((float)Para_Freq-2500.0)/(Para_Freq-500);
		Da_Write((k*Freq_Final+b)*51);
	}
	else
	{
		Da_Write(5*51);
	}

}

void Freq_Proc()
{
	unsigned char i;
	Freq_Final=Freq+Para_Adjust;
	if(Freq_Final >= 0 && Freq_Max < Freq_Final)//只有当频率测量不是负数时，才更新最大值
	{			
		Freq_Max=Freq_Final;
		for(i=0;i<3;i++)
		{
			max_freq_time[i]=ucRtc[i];
		}
	}
}
//定时器0充当计数器
void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			//设置定时器模式为计数器模式，读取P34引脚处的外部脉冲
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
	Key_Slow_Down++;
	Seg_Slow_Down++;
	if(Seg_Buf[Seg_Pos]>20)
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-',',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	Seg_Pos=(++Seg_Pos)%8;
	//led显示
	Led_Disp(ucled);
	if(Seg_Disp_Mode==0)
	{
		if(++Time1_200ms==200)
		{
			Time1_200ms=0;
			Led1_Flash_Flag=!Led1_Flash_Flag;
		}
	}
	else
	{
		Time1_200ms=0;
		Led1_Flash_Flag=0;
	}
	
	if(Freq_Final>Para_Freq)
	{
		
		if(++Time2_200ms==200)
		{
			Time2_200ms=0;
			Led2_Flash_Flag=!Led2_Flash_Flag;
		}
	}
	else
	{
		Time2_200ms=0;
		Led2_Flash_Flag=0;
	}	

	
	if(++Time_1s==1000)
	{
		Time_1s=0;
		Freq=(TH0<<8)|TL0;
		TH0=TL0=0;
	}
}
void main()
{
	System_Init();
	Set_Rtc(ucRtc);
	Timer0_Init();
	Timer1_Init();
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Freq_Proc();
		AD_DA();
	}
}
