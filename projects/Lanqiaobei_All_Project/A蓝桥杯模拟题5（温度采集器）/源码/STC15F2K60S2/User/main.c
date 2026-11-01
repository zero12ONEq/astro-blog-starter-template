/*头文件声明区域*/
#include <STC15F2K60S2.H>
#include <init.h>
#include <key.h>
#include <seg.h>
#include <led.h>
/*变量声明区域*/
unsigned char Seg_Pos=0;//0-7
unsigned char Seg_Buf[]={16,16,16,16,16,16,16,16};
unsigned char Seg_Mode=0;//0-2
unsigned short Seg_Flash_Timer=0;//500ms
bit Seg_Flash_Flag=1;//数码管闪烁标志位 1-亮状态 0-灭状态
unsigned char Key_Val=0,Key_Down=0,Key_Up=0,Key_Old=0;
unsigned char Key_Slow_Down=0;//10ms
unsigned char Tinput_Data[]={17,17,17};
unsigned short Tinput_Datas=0;
float Temperature=0.0;
unsigned char Tinput_Index=0;//0-2
bit Point_Flag=0;//1-点了小数点 0-没点小数点
unsigned char Seg_Point[]={0,0,0,0,0,0,0,0};
unsigned char TMAX=30;
unsigned char TMIN=20;
unsigned char TMAX_SET;
unsigned char TMIN_SET;
unsigned char Tset_Index=0;//0-1
bit Is_timing=0;
bit Long_Triggered=0;
unsigned short Key_Timer_500ms=0;
unsigned char Key_Timer_100ms=0;
unsigned char Ucled[]={0,0,0,0,0,0,0,0};//默认全部熄灭
unsigned char Led_Num=0;//0-5 6ms
unsigned char Led_Pwm=0;//1 3 5
unsigned char Error_Count=0;//错误参数设置计数器
bit Frist_Set_Flag=0;//进行了一次参数设置之后 置为1
/*按键处理函数区域*/
void Key_Proc()
{   
    unsigned char m;
    if(Key_Slow_Down>=10)
    {
        Key_Slow_Down=0;
        Key_Val=Key_Read();
        Key_Down=Key_Val&(Key_Val^Key_Old);
        Key_Up=~Key_Val&(Key_Val^Key_Old);
        Key_Old=Key_Val;
        if(Key_Down>=4&&Key_Down<=13)
        {
            if(Seg_Mode==0&&Tinput_Index<3)
            {
                Tinput_Data[Tinput_Index]=Key_Down-4;
                Tinput_Index++;
            }
        }
        if(Seg_Mode==2)
        {
            if(Key_Down==17)
            {
                Is_timing=1;
            }
            if(Key_Timer_500ms<500)
            {
                if(Key_Up==17)
                {
                    Is_timing=0;
                    Key_Timer_500ms=0;
                    if(Tset_Index==0)
                    {
                        if(++TMAX_SET>=71) TMAX_SET=10;
                    }
                    else
                    {
                        if(++TMIN_SET>=71) TMIN_SET=10;
                    }     
                }
            }
            else
            {   
                Long_Triggered=1;
                if(Key_Old==17)
                {
                    if(Key_Timer_100ms>=100)
                    {
                        Key_Timer_100ms=0;
                        if(Tset_Index==0)
                        {
                            if(++TMAX_SET>=71) TMAX_SET=10;
                        }
                        else
                        {
                            if(++TMIN_SET>=71) TMIN_SET=10;
                        } 
                    }
                }
                if(Key_Up==17)
                {
                    Is_timing=0;
                    Key_Timer_500ms=0;
                    Key_Timer_100ms=0;
                    Long_Triggered=0;
                }
            }
            
            if(Key_Down==18)
            {
                Is_timing=1;
            }
            if(Key_Timer_500ms<500)
            {
                if(Key_Up==18)
                {
                    Is_timing=0;
                    Key_Timer_500ms=0;
                    if(Tset_Index==0)
                    {
                        if(--TMAX_SET<=9) TMAX_SET=70;
                    }
                    else
                    {
                        if(--TMIN_SET<=9) TMIN_SET=70;
                    }     
                }
            }
            else
            {   
                Long_Triggered=1;
                if(Key_Old==18)
                {
                    if(Key_Timer_100ms>=100)
                    {
                        Key_Timer_100ms=0;
                        if(Tset_Index==0)
                        {
                            if(--TMAX_SET<=9) TMAX_SET=70;
                        }
                        else
                        {
                            if(--TMIN_SET<=9) TMIN_SET=70;
                        } 
                    }
                }
                if(Key_Up==18)
                {
                    Is_timing=0;
                    Key_Timer_500ms=0;
                    Key_Timer_100ms=0;
                    Long_Triggered=0;
                }
            }
        }
        
        switch(Key_Down)
        {
            case 14:
                if(Seg_Mode==0&&Point_Flag==0&&Tinput_Data[0]!=17)
                {
                    Seg_Point[4+Tinput_Index]=1;//只有第6，第7个数码管可以显示小数点，并且是数码管输入完数据才能打小数点。
                    Point_Flag=1;
                }
                break;
            case 15:
                if(Seg_Mode==1||Seg_Mode==2)
                {
                    if(Seg_Mode==1)
                    {
                        TMAX_SET=TMAX;
                        TMIN_SET=TMIN;
                        Tset_Index=0;
                        Seg_Mode=2;
                    }
                    else
                    {   
                        Frist_Set_Flag=1;
                        if(TMAX_SET>=TMIN_SET)
                        {
                            TMAX=TMAX_SET;
                            TMIN=TMIN_SET;
                            Error_Count=0;
                        }
                        else
                        {
                            TMAX_SET=TMAX;
                            TMIN_SET=TMIN;
                            Error_Count++;
                        }
                        Tset_Index=0;
                        Seg_Mode=1;
                    }
                }
                break;
            case 16:
                if(Seg_Mode==2)
                {
                    Tset_Index++;
                    if(Tset_Index>=2) Tset_Index=0;
                }
                break;
            case 19:
                if(Seg_Mode!=0)
                {
                   Tinput_Index=0;
                   Point_Flag=0;
                   Seg_Point[5]=Seg_Point[6]=0;
                   for(m=0;m<3;++m)
                   {
                       Tinput_Data[m]=17;
                   }
                   Seg_Mode=0;
                }
                else
                {
                    if(Point_Flag==0||Tinput_Index<3)
                    {
                        Tinput_Index=0;
                        Point_Flag=0;
                        Seg_Point[5]=Seg_Point[6]=0;
                        for(m=0;m<3;++m)
                        {
                            Tinput_Data[m]=17;
                        }
                    }
                    else
                    {   
                        Tinput_Datas=Tinput_Data[0]*100+Tinput_Data[1]*10+Tinput_Data[2]+5;
                        if(Seg_Point[5]==1)
                        {
                            Temperature=Tinput_Datas/100.0;
                        }
                        if(Seg_Point[6]==1)
                        {
                            Temperature=Tinput_Datas/10.0;
                        }
                        if((unsigned char)Temperature>85)
                        {
                            Tinput_Index=0;
                            Point_Flag=0;
                            Seg_Point[5]=Seg_Point[6]=0;
                            for(m=0;m<3;++m)
                            {
                                Tinput_Data[m]=17;
                            }
                        }
                        else
                        {
                            Tinput_Index=0;
                            Point_Flag=0;
                            Seg_Point[5]=Seg_Point[6]=0;
                            Seg_Mode=1;
                        }
                    } 
                }
                break;
        }
    }
    
}

/*数码管处理函数区域*/
void Seg_Proc()
{   
    unsigned char a;
    unsigned char i;
    if(Seg_Flash_Timer>=500)
    {
        Seg_Flash_Timer=0;
        Seg_Flash_Flag=!Seg_Flash_Flag;
    }
    switch(Seg_Mode)
    {
        case 0:
            Seg_Buf[0]=10;//'A'
            Seg_Buf[1]=16;
            Seg_Buf[2]=16;
            Seg_Buf[3]=16;
            Seg_Buf[4]=16;
            for(a=0;a<3;++a)
            {
                if(Tinput_Index==a&&!Seg_Flash_Flag) //选中且为为灭状态，则熄灭
                {
                    Seg_Buf[5+a]=16;
                }
                else //没选中或选中但是为亮状态，则正常显示
                {
                    Seg_Buf[5+a]=Tinput_Data[a];
                }
            }
            break;
        case 1:
            Seg_Buf[0]=12;//'C'
            Seg_Buf[1]=16;
            Seg_Buf[2]=16;
            Seg_Buf[3]=16; 
            Seg_Buf[4]=16;
            Seg_Buf[5]=16;
            Seg_Buf[6]=(Temperature>=10)?(unsigned char)Temperature/10%10:16;
            Seg_Buf[7]=(unsigned char)Temperature%10;
            break;
        case 2:
            Seg_Buf[0]=18;//'P'
            Seg_Buf[1]=16;
            Seg_Buf[2]=16;
            Seg_Buf[3]=16;
            for(i=0;i<2;++i)
            {
                if(Tset_Index==i&&!Seg_Flash_Flag)
                {
                    Seg_Buf[4+2*i]=16;
                    Seg_Buf[5+2*i]=16;
                }
                else
                {   
                    Seg_Buf[4+2*i]=(i==0)?TMAX_SET/10%10:TMIN_SET/10%10;
                    Seg_Buf[5+2*i]=(i==0)?TMAX_SET%10:TMIN_SET%10;
                }
            }
            break;
    }
     
}

/*其他外设处理函数区域*/
void Led_Proc()
{   
    if(Frist_Set_Flag)
    {
        if(Temperature>TMAX)
        {
            Led_Pwm=5;
            Ucled[0]=1;
        }
        else
        {
            Ucled[0]=0;
        }
        if(Temperature>=TMIN&&Temperature<=TMAX)
        {
            Led_Pwm=3;
            Ucled[1]=1;
        }
        else
        {
            Ucled[1]=0;
        }
        if(Temperature<TMIN)
        {
            Led_Pwm=1;
            Ucled[2]=1;
        }
        else
        {
            Ucled[2]=0;
        }
        if(Error_Count!=0)
        {
            Ucled[3]=1;
        }
        else
        {
            Ucled[3]=0;
        }
    } 
}

/*定时器0的初始化函数区域*/
void Timer0Init(void)		//1毫秒@12.000MHz,16位自动重装载
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
    
    ET0=1;//打开定时器0的中断（局部中断）
    EA=1;//打开全局中断
}

/*定时器0的中断服务函数*/
void Timer0server() interrupt 1
{
    Seg_Flash_Timer++;
    Key_Slow_Down++;
    if(Is_timing)
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
    if(Led_Num<=Led_Pwm)
    {
        Led_Disp(Seg_Pos,Ucled[Seg_Pos]);
    }
    else
    {
        Led_Disp(Seg_Pos,0);
    }
    if(++Led_Num>=6) Led_Num=0;
    
}
/*main函数区域*/
void main()
{
    System_Init();
    Timer0Init();
    while(1)
    {
        Key_Proc();
        Seg_Proc();
        Led_Proc();
    }
    
}