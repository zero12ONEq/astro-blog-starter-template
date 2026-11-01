#include "led.h"
idata unsigned char temp_1=0x00;
idata unsigned char temp_1_old=0xff;

//传入参数：数组名  无返回值
void Led_Disp(unsigned char*ucled)
{
    unsigned char temp;
    temp_1=0x00;
    temp_1=(ucled[0]<<0)|(ucled[1]<<1)|(ucled[2]<<2)|(ucled[3]<<3)|
            (ucled[4]<<4)|(ucled[5]<<5)|(ucled[6]<<6)|(ucled[7]<<7);
    if(temp_1!=temp_1_old)
    {
        P0=~temp_1;
        //开门
        temp=P2&0x1f;
        temp=temp|0x80;
        P2=temp;
        //关门
        temp=P2&0x1f;
        P2=temp;
        
        temp_1_old=temp_1;
    }
}

//快速关led的函数
void Led_Off()
{
    unsigned char temp;
    P0=0xff;//关闭所有LED
    //开门
    temp=P2&0x1f;
    temp=temp|0x80;
    P2=temp;
    //关门
    temp=P2&0x1f;
    P2=temp;

    temp_1_old=0x00;//置为0，防止进入上面的if(temp_1!=temp_1_old)语句
}
/*蜂鸣器，继电器，电机使能，关闭函数*/
//因为这三个是同一个芯片控制的且都是操控P0引脚 因此采用同一套全局变量控制
idata unsigned char temp_2=0x00;
idata unsigned char temp_2_old=0xff;
//蜂鸣器使能，关闭函数
void Beep(bit enable)
{   
    unsigned char temp;
    if(enable)
        temp_2|=0x40;//D6置为1
    else
        temp_2&=(~0x40);//D6置为0
    if(temp_2!=temp_2_old)
    {
        P0=temp_2;
        //开门
        temp=P2&0x1f;
        temp=temp|0xa0;
        P2=temp;
        //关门
        temp=P2&0x1f;
        P2=temp;
        
        temp_2_old=temp_2;        
    }
}
//电机使能，关闭函数
void Motor(bit enable)
{
    unsigned char temp;
    if(enable)
        temp_2|=0x20;//D5置为1
    else
        temp_2&=(~0x20);//D5置为0
    if(temp_2!=temp_2_old)
    {
        P0=temp_2;
        //开门
        temp=P2&0x1f;
        temp=temp|0xa0;
        P2=temp;
        //关门
        temp=P2&0x1f;
        P2=temp;
        
        temp_2_old=temp_2;        
    }    
}
//继电器使能，关闭函数
void Relay(bit enable)
{
    unsigned char temp;
    if(enable)
        temp_2|=0x10;//D4置为1
    else
        temp_2&=(~0x10);//D4置为0
    if(temp_2!=temp_2_old)
    {
        P0=temp_2;
        //开门
        temp=P2&0x1f;
        temp=temp|0xa0;
        P2=temp;
        //关门
        temp=P2&0x1f;
        P2=temp;
        
        temp_2_old=temp_2;        
    }
}
    