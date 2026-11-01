#include <STC15F2K60S2.H>
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "onewire.h"
#include "iic.h"
#include "ultrasound.h"
#include "math.h"
idata unsigned char Key_Val,Key_Old,Key_Down,Key_Up;
idata unsigned char Key_Slow_Down;
idata bit Long_Press_Flag;//按键长按标志位
idata unsigned int Long_Press_Time_2S;//按键长按2s计时器

idata unsigned char Seg_Pos;
pdata unsigned char Seg_Buf[]={10,10,10,10,10,10,10,10};
idata unsigned char Seg_Slow_Down;
idata unsigned char Seg_Disp_Mode;//0-环境状态 1-运动检测 2-参数设置 3-统计数据
idata unsigned char Para_Disp_Mode;//参数设置界面的两个界面 0-温度参数 1-距离参数
idata unsigned char Temperature_Val_Para=30;//温度参数
idata unsigned char Distance_Para=30;//距离参数

idata bit Relay_Led_Lock;//继电器，led状态锁定标志位
idata bit Relay_Old;//继电器从关到开的标志位
idata unsigned int Relay_Count;//继电器吸合次数
pdata unsigned char ucled[]={0,0,0,0,0,0,0,0};
idata bit Led_Flash_Flag;//1-亮 0-灭
idata unsigned char Led_Flash_Clock;//led闪烁计时器 100ms


idata unsigned char Temperature_Val;//环境温度
idata unsigned int Temperature_Slow_Down;
idata bit Htemperature_Flag;//高温标志位 0-未得到高温 1-高温

idata unsigned char Light_Level;//光强等级
idata unsigned char AD_Slow_Down;

idata unsigned char Motion_State; //运动状态 0-静止 1-徘徊 2-跑动
idata bit Run_State;//跑动的状态
idata bit Stay_State;//静止状态
idata bit Paralysis_State;//徘徊状态
idata unsigned char Distance_Val;
idata unsigned char Distance_Old;
idata unsigned char Distance_Sub;//距离差值
idata unsigned int Distance_Slow_Down;
idata bit First_Get_Distance;//当从0->1表示已经获取里第一次的距离
idata bit State_Change_Flag;//运动状态锁定标志位 1-锁定 0-未锁定
idata unsigned int Distance_Lock_3S;//运动状态锁定3s计时器
idata bit Distance_Flag;//距离接近标志位 1-接近



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
		Seg_Disp_Mode=(++Seg_Disp_Mode)%4;//0-3之间循环
		Para_Disp_Mode=0;//让参数界面默认置为温度参数子界面
		if(Seg_Disp_Mode==2)//进入参数界面，锁定
		{
			Relay_Led_Lock=1;
		}
		else//退出参数界面，取消锁定
		{
			Relay_Led_Lock=0;
		}
	}
	//在参数界面下
	if(Seg_Disp_Mode==2)
	{
		if(Key_Down==5)
		{
			Para_Disp_Mode=(++Para_Disp_Mode)%2;
		}
		if(Key_Down==8)
		{
			if(Para_Disp_Mode==0) 
			{
				if(Temperature_Val_Para<80)
					Temperature_Val_Para++;
			}
			else
			{
				if(Distance_Para<80)
					Distance_Para+=5;
			}
		}
		if(Key_Down==9)
		{
			if(Para_Disp_Mode==0) 
			{
				if(Temperature_Val_Para>20)
					Temperature_Val_Para--;
			}
			else
			{
				if(Distance_Para>20)
					Distance_Para-=5;
			}
		}
	}
	//在统计界面下
	if(Seg_Disp_Mode==3)
	{
		if(Long_Press_Time_2S>=2000)
			Relay_Count=0;//清零继电器吸合次数
		if(Key_Old==89)//按键89同时按下
		{
			Long_Press_Flag=1;
		}
		else//松手或未同时按下
		{
			Long_Press_Flag=0;//标志位置0
			Long_Press_Time_2S=0;//计时器归0
		}
	}
}

void Seg_Proc()
{
	if(Seg_Slow_Down<80) return;
	Seg_Slow_Down=0;
	switch(Seg_Disp_Mode)
	{
		case 0:
			Seg_Buf[0]=11;
			Seg_Buf[1]=Temperature_Val/10%10;
			Seg_Buf[2]=Temperature_Val%10;
			Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;
			Seg_Buf[6]=12;
			Seg_Buf[7]=Light_Level;
			break;
		case 1:
			Seg_Buf[0]=13;
			Seg_Buf[1]=Motion_State+1;
			Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=10;
			Seg_Buf[5]=Distance_Val/100%10;//直接按正常的写就是高位补0，如56/100=0 0%10=0
			Seg_Buf[6]=Distance_Val/10%10;
			Seg_Buf[7]=Distance_Val%10;	
			break;
		case 2:
			Seg_Buf[0]=14;
			Seg_Buf[2]=Seg_Buf[3]=Seg_Buf[4]=Seg_Buf[5]=10;
			if(Para_Disp_Mode==0)
			{
				Seg_Buf[1]=11;
				Seg_Buf[6]=Temperature_Val_Para/10%10;
				Seg_Buf[7]=Temperature_Val_Para%10;
			}
			else
			{
				Seg_Buf[1]=13;
				Seg_Buf[6]=Distance_Para/10%10;
				Seg_Buf[7]=Distance_Para%10;				
			}				
			break;
		case 3:
			Seg_Buf[0]=12;
			Seg_Buf[1]=11;
			Seg_Buf[2]=Seg_Buf[3]=10;
			Seg_Buf[4]=(Relay_Count>=1000)?Relay_Count/1000%10:10;
			Seg_Buf[5]=(Relay_Count>=100)?Relay_Count/100%10:10;
			Seg_Buf[6]=(Relay_Count>=10)?Relay_Count/10%10:10;
			Seg_Buf[7]=Relay_Count%10;
			break;
	}
}

void Led_Proc()
{
	if(!Relay_Led_Lock)//如果不锁定
	{
		if(Distance_Flag)
		{
			switch(Light_Level)
			{
				case 1:
					ucled[0]=1;
					ucled[1]=ucled[2]=ucled[3]=0;
					break;
				case 2:
					ucled[0]=ucled[1]=1;
					ucled[2]=ucled[3]=0;
					break;
				case 3:
					ucled[0]=ucled[1]=ucled[2]=1;
					ucled[3]=0;
					break;
				case 4:
					ucled[0]=ucled[1]=ucled[2]=ucled[3]=1;
					break;
			}
		}
		else
		{
			ucled[0]=ucled[1]=ucled[2]=ucled[3]=0;
		}
		if(Distance_Flag==1&&Htemperature_Flag==1)
		{
			Relay(1);
			if(Relay_Old==0)
			{
				Relay_Count++;
			}
			Relay_Old=1;
		}
		else
		{
			Relay(0);
			Relay_Old=0;
		}
	}
	if(Stay_State) 
		ucled[7]=0;
	else if(Paralysis_State)
		ucled[7]=1;
	else if(Run_State)
		ucled[7]=Led_Flash_Flag;
	Led_Disp(ucled);
}
void Get_AD()
{
	unsigned char temp;
	if(AD_Slow_Down<100)return;
	AD_Slow_Down=0;
	temp=AD_Read(0x01);
	if(temp>=3*51)Light_Level=1;
	else if(temp>=2*51)Light_Level=2;
	else if(temp>=0.5*51)Light_Level=3;
	else Light_Level=4;	
}

void Get_Temperature()
{
	float temp;
	if(Temperature_Slow_Down<300)return;
		Temperature_Slow_Down=0;
	temp=rd_temperature();
	Temperature_Val=temp;
	if(temp>Temperature_Val_Para) 
	{
		Htemperature_Flag=1;
	}
	else
	{
		Htemperature_Flag=0;
	}
	
}

void Get_Distance()
{
	unsigned char temp_motion_state;
	if(Distance_Slow_Down<999) return;
		Distance_Slow_Down=0;
	if(First_Get_Distance==1)
	{
		Distance_Val=Ut_Wave_Data();
		Distance_Sub=abs(Distance_Val-Distance_Old);
		Distance_Old=Distance_Val;
		Distance_Flag=(Distance_Val<Distance_Para);
		if(!State_Change_Flag)
		{
			if(Distance_Sub<5) temp_motion_state=0;
			else if(Distance_Sub<10) temp_motion_state=1;
			else temp_motion_state=2;
			
			//如果状态改变
			if(temp_motion_state!=Motion_State)
			{
				State_Change_Flag=1;
				Distance_Lock_3S=0;  //3s锁定计时器初始化为0
				Motion_State=temp_motion_state;
			}
			
		}

		if(!Relay_Led_Lock)//led和继电器不锁定才根据实际情况把下面的三个状态置0或置1
		{
			Stay_State=(Motion_State==0);
			Paralysis_State=(Motion_State==1);
			Run_State=(Motion_State==2);
		}
	}
	else
	{
		First_Get_Distance=1;//获取了第一次的距离
		Distance_Val=Ut_Wave_Data();
		Distance_Old=Distance_Val;
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
	Key_Slow_Down++;
	Seg_Slow_Down++;
	Temperature_Slow_Down++;
	Distance_Slow_Down++;
	AD_Slow_Down++;
	
	//数码管动态扫描
	if(Seg_Buf[Seg_Pos]>20)
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-',',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	Seg_Pos=(++Seg_Pos)%8;

	
	if(Long_Press_Flag)//如果长按按键89
	{
		if(++Long_Press_Time_2S>=2000) Long_Press_Time_2S=2001;
	}
	else//如果没有长按
	{
		Long_Press_Time_2S=0;
	}
	
	if(Run_State)
	{
		if(++Led_Flash_Clock==100)
		{
			Led_Flash_Clock=0;
			Led_Flash_Flag=!Led_Flash_Flag;//在0-1之间翻转
		}
	}
	else
	{
		Led_Flash_Clock=0;
		Led_Flash_Flag=0;
	}
	
	if(State_Change_Flag)
	{
		if(++Distance_Lock_3S==3000)
		{
			Distance_Lock_3S=0;
			State_Change_Flag=0;
		}
	}
	
}


void main()
{
	System_Init();
	Timer1_Init();
	while(rd_temperature()==85.0);
	AD_Read(0x01);
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Get_AD();
		Get_Temperature();
		Get_Distance();
	}


}