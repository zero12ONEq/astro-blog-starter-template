#include <led.h>

/*led显示函数*/
//传入参数：addr：led位选值 0-7 enable：1-使能/点亮 0-不使能/不点亮
void Led_Disp(unsigned char addr,enable)
{
    static unsigned char temp=0x00;//用static修饰 让每次调用函数时，tmp和tmp_old的值为上次的值。防止每次进来tmp都被赋值成0x00，重复刷新。
    static unsigned char temp_old=0xff;
    if(enable)//如果使能
    {
        temp|=(0x01<<addr);//把第addr位置为1
    }
    else//如果不使能
    {
        temp&=~(0x01<<addr);//把第addr位置为0
    }
    if(temp!=temp_old)//只有当tmp的数据改变时，才执行给P0赋值以及开关锁存器的操作，因为P0,P2引脚是多个外设都要使用的，防止影响其他外设。
    {
        P0=~temp;
        P2=(P2&0x1f)|0x80;//开门
        P2=P2&0x1f;//关门
        temp_old=temp;
    } 
}