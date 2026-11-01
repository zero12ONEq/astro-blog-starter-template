/*头文件声明区域*/
#include <STC15F2K60S2.H>
#include <init.h>
#include <key.h>
#include <seg.h>
#include <led.h>
#include "iic.h"
/*变量声明区域*/
unsigned char Seg_Mode=0;//0-电压显示 1-电压输出
unsigned char Seg_Pos=0;//0-7循环
unsigned char Seg_Buf[]={16,16,16,16,16,16,16,16};
unsigned char Seg_Point[]={0,0,0,0,0,1,0,0};//只有D5点小数点
float V_RB2;
unsigned char Key_Slow_Down=0;//10ms
unsigned char Key_Val=0,Key_Down=0,Key_Up=0,Key_Old=0;
bit PCF_Mode=0;//1-随电位器 0-固定2.0v
bit Led_Enable=1;//1-led功能启用 0-关闭
bit Seg_Enable=1;//1-数码管启用 0-关闭
float V_Output;
unsigned char Ucled[]={0,0,0,0,0,0,0,0};//8个Led默认熄灭
unsigned char i;
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
        switch(Key_Down)
        {
            case 4:
                if(++Seg_Mode>=2) Seg_Mode=0;
                break;
            case 5:
                PCF_Mode=!PCF_Mode;
                break;
            case 6:
                Led_Enable=!Led_Enable;
                break;
            case 7:
                Seg_Enable=!Seg_Enable;
                break;
        }
            
    }
        
}

/*数码管处理函数区域*/
void Seg_Proc()
{   
    /*信息处理区域*/
    V_RB2=Ad_Read(0x43)/51.0;
    /*数码管显示区域*/
    if(Seg_Enable)
    {
        switch(Seg_Mode)
        {
            case 0:
                Seg_Buf[0]=17;//'U'
                Seg_Buf[5]=(unsigned char)V_RB2;
                Seg_Buf[6]=(unsigned char)(V_RB2*10)%10;
                Seg_Buf[7]=(unsigned char)(V_RB2*100)%10;
                break;
            case 1:
                if(PCF_Mode)
                {
                    V_Output=V_RB2;
                }
                else
                {
                    V_Output=2.0;
                }
                Seg_Buf[0]=15;//'F'
                Seg_Buf[5]=(unsigned char)V_Output;
                Seg_Buf[6]=(unsigned char)(V_Output*10)%10;
                Seg_Buf[7]=(unsigned char)(V_Output*100)%10;
                break;
        }
    }
    else
    {
        for(i=0;i<8;++i)
        {
            Seg_Buf[i]=16;
        }
        Seg_Point[5]=0;
    }        
}
/*其他外设处理函数区域*/
void Led_Proc()
{   
   Da_Write((unsigned char)(V_Output*51));//把转换得到的模拟量输出到引脚上，便于万用表检测
   if(Led_Enable)
   {   
       if(Seg_Mode==0)
       {
            Ucled[0]=1;//电压显示模式，L1亮，L2灭
            Ucled[1]=0;
       }
       else
       {    
            Ucled[0]=0;
            Ucled[1]=1;//电压输出模式，L2亮，L1灭
       }
       
       if(V_RB2>=3.5||(V_RB2>=1.5&&V_RB2<2.5))
       {
            Ucled[2]=1;
       }
       else if((V_RB2>=2.5&&V_RB2<3.5)||V_RB2<1.5)
       {
            Ucled[2]=0;
       }
       
       if(PCF_Mode)//随RB2变化，L4点亮
       {
            Ucled[3]=1;
       }
       else  //固定2.0v，L4熄灭
       {
            Ucled[3]=0;
       }
   }
   else
   {
      for(i=0;i<8;++i)
        {
            Ucled[i]=0;
        }  
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


/*定时器0的服务中断函数*/
void Timer0server() interrupt 1
{   
    Key_Slow_Down++;
    Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
    Led_Disp(Seg_Pos,Ucled[Seg_Pos]);
    if(++Seg_Pos>=8) Seg_Pos=0;
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