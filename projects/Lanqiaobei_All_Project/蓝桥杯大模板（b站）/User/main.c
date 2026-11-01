/*头文件声明区域*/
#include <STC15F2K60S2.H>
#include <init.h>
#include <led.h>
#include <key.h>
#include <seg.h>
#include "onewire.h"
/*变量定义区域*/
float t;
unsigned char Seg_Pos=0;//0-7
unsigned char Seg_Buf[]={16,16,16,16,16,16,16,16};
/*按键处理函数区域*/
void Key_Proc()
{

}

/*数码管处理函数区域*/
void Seg_Proc()
{
    t=read_t();
    Seg_Buf[0]=(unsigned char)t/10%10;
    Seg_Buf[1]=(unsigned char)t%10;
    Seg_Buf[2]=(unsigned int)(t*10)%10;
}

/*其他外设处理函数区域*/
void Led_Proc()
{

}
/*定时器0的初始化函数区域*/
void Timer0Init(void)		//1毫秒@12.000MHz 自动重装载版本
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
    
    ET0=1;//打开定时器0的中断
    EA=1;//打开全局中断
}
/*定时器0的服务中断函数区域*/
void Timer0server() interrupt 1
{
    Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
    if(++Seg_Pos>=8) Seg_Pos=0;
}
/*main函数区域*/
void main()
{   
    System_Init();//外设初始化函数调用
    Timer0Init();//定时器0的初始化函数调用
    while(1)
    {
       Key_Proc();
       Seg_Proc();
       Led_Proc();
    }

}