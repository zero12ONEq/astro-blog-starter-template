/*头文件声明区域*/
#include <STC15F2K60S2.H>
#include <init.h>
#include <key.h>
#include <seg.h>
#include <led.h>
#include "iic.h"
/*变量声明区域*/
unsigned char Seg_Mode=0;//0-模式1 1-模式2
unsigned char Seg_Pos=0;//0-7循环
unsigned char Seg_Buf[]={16,16,16,16,16,16,16,16};
unsigned char Key_Slow_Down=0;//10ms
unsigned char Key_Val=0,Key_Down=0,Key_Up=0,Key_Old=0;
unsigned char dat[]={30,60};
unsigned char a=200;
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
            case 19:
                dat[0]+=10;
                break;
            case 18:
                dat[1]+=10;
                break;
            case 17:
                EEPROM_Write(dat,0,2);
                break;
            case 16:
                a-=10;
                break;
            case 15:
                a+=10;
                break;
            case 14:
                EEPROM_Write(&a,8,1);
                break;
            case 13:
                if(++Seg_Mode>=2) Seg_Mode=0;
                break;
        }   
    }
        
}

/*数码管处理函数区域*/
void Seg_Proc()
{   
    switch(Seg_Mode)
    {
        case 0:
            Seg_Buf[0]=dat[0]/10%10;
            Seg_Buf[1]=dat[0]%10;
            Seg_Buf[6]=dat[1]/10%10;
            Seg_Buf[7]=dat[1]%10;
            break;
        case 1:
            Seg_Buf[0]=a/100%10;
            Seg_Buf[1]=a/10%10;
            Seg_Buf[2]=a%10;
            Seg_Buf[6]=16;
            Seg_Buf[7]=16;
            break;
    } 
}
/*其他外设处理函数区域*/

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
    Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
    if(++Seg_Pos>=8) Seg_Pos=0;
}


/*main函数区域*/
void main()
{   
    //EEPROM_Read(dat,0,2);//把保存值重新赋值给dat
    //EEPROM_Read(&a,8,1);//把保存值重新赋值给a
    System_Init();
    Timer0Init();
    while(1)
    {
        Key_Proc();
        Seg_Proc();
    }

}