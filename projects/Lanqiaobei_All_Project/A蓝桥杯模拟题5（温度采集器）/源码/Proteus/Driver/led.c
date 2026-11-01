#include <led.h>

//传入参数 addr可以为0-7表示第1-8个led enable=1 点亮 enable=0 熄灭
void Led_Disp(unsigned char addr,enable)
{
    static unsigned char tmp=0x00;
    static unsigned char tmp_old=0xff;
    
    if(enable) //如果使能，就把那一位强制置为1
    {
        tmp|=(0x01<<addr);
    }
    else //如果不使能，就把那一位强制置为0
    {
        tmp&=~(0x01<<addr);
    }
    
    if(tmp!=tmp_old) 
    {
        P1=~tmp; //只有在数据更新时才执行把~tmp赋值给P1的操作
        tmp_old=tmp;
    }
}