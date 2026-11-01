#include "init.h"

void System_Init()
{
    unsigned char temp;
    /*关闭LED*/
    P0=0xff;
    //开门
    temp=P2&0x1f;
    temp=temp|0x80;
    P2=temp;
    //关门
    temp=P2&0x1f;
    P2=temp;
    
    /*关闭继电器，电机，蜂鸣器*/
    P0=0x00;
    //开门
    temp=P2&0x1f;
    temp=temp|0xa0;
    P2=temp;
    //关门
    temp=P2&0x1f;
    P2=temp;
}