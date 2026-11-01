/*头文件声明区域*/
#include <STC15F2K60S2.H>
#include <init.h>
#include <key.h>
#include <seg.h>
#include <led.h>
#include "onewire.h"

/*变量声明区域*/
unsigned char Seg_Mode=0;//0-2
unsigned char Seg_Pos=0;//0-7
unsigned char Seg_Buf[]={16,16,16,16,16,16,16,16};
unsigned char Seg_Point[]={0,0,0,0,0,0,0,0};
float Temperature;
unsigned char TMAX=30;
unsigned char TMIN=20;
unsigned char TMAX_SET;
unsigned char TMIN_SET;
unsigned short Seg_Flash_Timer=0;//500
bit Seg_Flash_Flag=1;
unsigned char Seg_Index=0;//0-1
unsigned char Key_Slow_Down=0;//10
unsigned char Key_Val=0,Key_Down=0,Key_Up=0,Key_Old=0;
unsigned char Accuracy_Grade_Set;
unsigned char Accuracy_Grade=3;
unsigned char Accuracy_Grade_Old=3;
unsigned char Accuracy_Grade_Data[]={0x1f,0x5f,0x3f,0x7f};
unsigned short Key_Timer_500ms=0;//500
unsigned char Key_Timer_100ms=0;//100
bit Is_Timing=0;
bit Long_Triggered=0;
unsigned short Onewire_Timer=0;
unsigned char Led_Num=0;//10ms
unsigned char Led_Pwm=0;//4-50%等级3;6-70%等级2;9-100%等级1
unsigned char Ucled[]={0,0,0,0,0,0,0,0};
unsigned char Error_Count=0;
/*按键处理函数声明区域*/
void Key_Proc()
{
    if(Key_Slow_Down>=10)
    {
        Key_Slow_Down=0;
        Key_Val=Key_Read();
        Key_Down=Key_Val&(Key_Val^Key_Old);
        Key_Up=~Key_Val&(Key_Val^Key_Old);
        Key_Old=Key_Val;
        if(Seg_Mode==1||Seg_Mode==2)
        {
            if(Key_Down==14)
            {
                Is_Timing=1;
            }
            if(Key_Timer_500ms<500) //短按
            {
                if(Key_Up==14)
                {
                    Key_Timer_500ms=0;
                    Is_Timing=0;
                    if(Seg_Mode==1)
                    {
                        if(Seg_Index==0)
                        {
                            if(++TMAX_SET>=71) TMAX_SET=10;
                        }
                        else
                        {
                            if(++TMIN_SET>=71) TMIN_SET=10;
                        }
                    }
                    else 
                    {
                        if(++Accuracy_Grade_Set>=4) Accuracy_Grade_Set=0;
                    }
                }
            }
            else //长按
            {
                if(!Long_Triggered) //达到500ms，在0-100ms先执行一次累加
                {
                    Long_Triggered=1;
                    if(Seg_Mode==1)
                    {
                        if(Seg_Index==0)
                        {
                            if(++TMAX_SET>=71) TMAX_SET=10;
                        }
                        else
                        {
                            if(++TMIN_SET>=71) TMIN_SET=10;
                        }
                    }
                    else 
                    {
                        if(++Accuracy_Grade_Set>=4) Accuracy_Grade_Set=0;
                    }
                }
                if(Key_Old==14)
                {
                    if(Key_Timer_100ms>=100)
                    {
                        Key_Timer_100ms=0;
                        if(Seg_Mode==1)
                        {
                            if(Seg_Index==0)
                            {
                                if(++TMAX_SET>=71) TMAX_SET=10;
                            }
                            else
                            {
                                if(++TMIN_SET>=71) TMIN_SET=10;
                            }
                        }
                        else 
                        {
                            if(++Accuracy_Grade_Set>=4) Accuracy_Grade_Set=0;
                        }
                    }
                }
                if(Key_Up==14)
                {
                    Long_Triggered=0;
                    Is_Timing=0;
                    Key_Timer_500ms=0;
                    Key_Timer_100ms=0;
                }
            }
            
            if(Key_Down==15)
            {
                Is_Timing=1;
            }
            if(Key_Timer_500ms<500) //短按
            {
                if(Key_Up==15)
                {
                    Key_Timer_500ms=0;
                    Is_Timing=0;
                    if(Seg_Mode==1)
                    {
                        if(Seg_Index==0)
                        {
                            if(--TMAX_SET<=9) TMAX_SET=70;
                        }
                        else
                        {
                            if(--TMIN_SET<=9) TMIN_SET=70;
                        }
                    }
                    else 
                    {
                        if(--Accuracy_Grade_Set==255) Accuracy_Grade_Set=3;
                    }
                }
            }
            else //长按
            {
                if(!Long_Triggered) //时间达到500ms，在0-100ms内先执行一次递减
                {
                    Long_Triggered=1;
                    if(Seg_Mode==1)
                    {
                        if(Seg_Index==0)
                        {
                            if(--TMAX_SET<=9) TMAX_SET=70;
                        }
                        else
                        {
                            if(--TMIN_SET<=9) TMIN_SET=70;
                        }
                    }
                    else 
                    {
                        if(--Accuracy_Grade_Set==255) Accuracy_Grade_Set=3;
                    }
                }
                if(Key_Old==15)
                {
                    if(Key_Timer_100ms>=100)
                    {
                        Key_Timer_100ms=0;
                        if(Seg_Mode==1)
                        {
                            if(Seg_Index==0)
                            {
                                if(--TMAX_SET<=9) TMAX_SET=70;
                            }
                            else
                            {
                                if(--TMIN_SET<=9) TMIN_SET=70;
                            }
                        }
                        else 
                        {
                            if(--Accuracy_Grade_Set==255) Accuracy_Grade_Set=3;

                        }
                    }
                }
                if(Key_Up==15)
                {
                    Long_Triggered=0;
                    Is_Timing=0;
                    Key_Timer_500ms=0;
                    Key_Timer_100ms=0;
                }
            }
        }
        switch(Key_Down)
        {   
            case 12:
                if(Seg_Mode==0)
                {   
                    Seg_Index=0;
                    TMAX_SET=TMAX;
                    TMIN_SET=TMIN;
                    Seg_Mode=1;
                }
                else if(Seg_Mode==1)
                {   
                    if(TMAX_SET>TMIN_SET)
                    {   
                        Error_Count=0;
                        TMAX=TMAX_SET;
                        TMIN=TMIN_SET;
                    }
                    else
                    {
                        Error_Count++;
                    }
                    Accuracy_Grade_Set=Accuracy_Grade;
                    Seg_Mode=2;
                }
                else if(Seg_Mode==2)
                {   
                    Accuracy_Grade=Accuracy_Grade_Set;
                    Seg_Mode=0;
                }
                break;
            case 13:
                if(Seg_Mode==1)
                {
                    if(++Seg_Index>=2) Seg_Index=0;
                }
                break;
            case 16:
                TMAX=30;
                TMIN=20;
                Accuracy_Grade=3;
                break;
        }
    }
}

/*数码管处理函数区域*/
void Seg_Proc()
{
    unsigned char i;
    if(Seg_Flash_Timer>=500)
    {
        Seg_Flash_Timer=0;
        Seg_Flash_Flag=!Seg_Flash_Flag;
    }
    switch(Seg_Mode)
    {
        case 0:
            Seg_Buf[0]=12;//"C"
            Seg_Buf[1]=16;
            Seg_Buf[2]=16;
            Seg_Buf[3]=16;
            Seg_Buf[4]=(unsigned char)Temperature/10%10;
            Seg_Buf[5]=(unsigned char)Temperature%10;
            Seg_Buf[6]=(unsigned short)(Temperature*10)%10;
            Seg_Buf[7]=12;//"C"
            if((unsigned char)Temperature<10)
            {   
                Seg_Point[4]=1;
                Seg_Point[5]=0;
            }
            else
            {   
                Seg_Point[4]=0;
                Seg_Point[5]=1;
            }
            break;
        case 1:
            Seg_Buf[0]=17;//"P"
            Seg_Buf[1]=16;
            Seg_Buf[2]=16;
            for(i=0;i<2;i++)
            {
                if(Seg_Index==i&&!Seg_Flash_Flag)
                {
                    Seg_Buf[3+3*i]=16;
                    Seg_Buf[4+3*i]=16;
                }
                else
                {
                    Seg_Buf[3+3*i]=(i==0)?TMAX_SET/10%10:TMIN_SET/10%10;
                    Seg_Buf[4+3*i]=(i==0)?TMAX_SET%10:TMIN_SET%10;
                }
            }
            Seg_Buf[5]=18;//"-"
            break;
            
        case 2:
            Seg_Buf[0]=10;//"A"
            Seg_Buf[1]=16;
            Seg_Buf[2]=16;
            Seg_Buf[3]=16;
            Seg_Buf[4]=16;
            Seg_Buf[5]=16;
            Seg_Buf[6]=18;//"-"
            Seg_Buf[7]=Accuracy_Grade_Set+1;
            break;
    }
}
/*其他外设处理函数区域*/
void Led_Proc()
{
    Ucled[0]=0;
    Ucled[1]=0;
    Ucled[2]=0;

    if(Temperature>TMAX)
    {
        Led_Pwm=9;
        Ucled[0]=1;
    }
    else if(Temperature>=TMIN&&Temperature<=TMAX)
    {
        Led_Pwm=6;
        Ucled[1]=1;
    }
    else if(Temperature<TMIN)
    {
        Led_Pwm=4;
        Ucled[2]=1;
    }
    if(Error_Count)
    {
        Ucled[3]=1;
    }
    else
    {
        Ucled[3]=0;
    }
}

void SET_DS18B20_Accuracy()
{
    unsigned short convert_time;

    // 检测精度等级是否改变，只在改变时重新配置DS18B20
    if(Accuracy_Grade != Accuracy_Grade_Old)
    {
        Accuracy_Grade_Old = Accuracy_Grade;
        Config_Temper(Accuracy_Grade_Data[Accuracy_Grade]);
        Onewire_Timer = 0;  // 重置定时器
    }

    // 根据精度等级设置转换时间
    switch(Accuracy_Grade)
    {
        case 0: convert_time = 94;  break;  // 9位精度: 93.75ms
        case 1: convert_time = 188; break;  // 10位精度: 187.5ms
        case 2: convert_time = 375; break;  // 11位精度: 375ms
        case 3: convert_time = 750; break;  // 12位精度: 750ms
        default: convert_time = 750; break;
    }

    // 定时读取温度
    if(Onewire_Timer >= convert_time)
    {
        Onewire_Timer = 0;
        Temperature = Read_T();  // 直接使用Read_T()返回值，不需要再除以任何数
    }
}
/*定时器0的初始化函数*/
void Timer0Init(void)		
{
	AUXR &= 0x7F;		
	TMOD &= 0xF0;		
	TL0 = 0x18;		    
	TH0 = 0xFC;		   
	TF0 = 0;		    
	TR0 = 1;		    

    ET0=1;
    EA=1;
}

/*定时器1的初始化函数*/
void Timer1Init(void)		
{
	AUXR &= 0xBF;		
	TMOD &= 0x0F;		
	TL1 = 0x18;		
	TH1 = 0xFC;		
	TF1 = 0;		
	TR1 = 1;		

    ET1=1;
    EA=1;
}
/*定时器0的服务中断函数*/
void Timer0server() interrupt 1
{   
    if(++Led_Num>=10) Led_Num=0;
    if(Led_Num<=Led_Pwm)
    {
        Led_Disp(Seg_Pos,Ucled[Seg_Pos]);
    }
    else
    {
        Led_Disp(Seg_Pos,0);
    }
    Seg_Flash_Timer++;
    Key_Slow_Down++;
    if(Is_Timing)
    {
        Key_Timer_500ms++;
    }
    if(Long_Triggered)
    {
        Key_Timer_100ms++;
    }
    if(Seg_Mode==0)
    {
        Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
    }
    else
    {
        Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
    }
    if(++Seg_Pos>=8) Seg_Pos=0;
}

/*定时器1的服务中断函数*/
void Timer1server() interrupt 3
{
    Onewire_Timer++;
}
/*main函数区域*/
void main()
{
    System_Init();
    Timer0Init();
    Timer1Init();
    while(1)
    {
        Key_Proc();
        Seg_Proc();
        Led_Proc();
        SET_DS18B20_Accuracy();
    }

}