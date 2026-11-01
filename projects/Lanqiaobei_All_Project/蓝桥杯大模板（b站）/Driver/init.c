#include <init.h>

/*外设初始化函数*/
void System_Init()
{
    //#1、LED初始化（关闭所有LED）
    P0=0xff;
    P2=(P2&0x1f)|0x80;//开门
    P2=P2&0x1f;//关门
    
    //#2、继电器，电机，蜂鸣器初始化（全部关闭）
    P0=0x00;
    P2=(P2&0x1f)|0xa0;//开门
    P2=P2&0x1f;//关门
}