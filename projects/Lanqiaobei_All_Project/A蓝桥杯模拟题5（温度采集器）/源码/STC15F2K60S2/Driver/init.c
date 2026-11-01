#include <init.h>

void System_Init()
{
    //1、关闭LED
    P0=0xff;
    P2=(P2&0x1f)|0x80;
    P2=P2&0x1f;
    
    //2、关闭继电器，电机，蜂鸣器
    P0=0x00;
    P2=(P2&0x1f)|0xa0;
    P2=P2&0x1f;
}