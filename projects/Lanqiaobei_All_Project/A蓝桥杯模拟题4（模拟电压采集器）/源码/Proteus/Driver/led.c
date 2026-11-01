#include <led.h>

/*led显示函数*/
//#1、无返回值 传入参数 addr:0-7表示点亮1-8个led enable：1-点亮 0-熄灭
void Led_Disp(unsigned char addr,enable)
{
    static unsigned char tmp=0x00;//使用static修饰，使得每次函数调用结束，tmp的值保留，下一次进入函数tmp的值还是一样
    static unsigned char tmp_old=0xff;
    
    if(enable) //如果使能
    {
        tmp|=0x01<<addr;
    }
    else //如果不使能
    {
        tmp&=~(0x01<<addr);
    }
    if(tmp!=tmp_old) //只有当tmp的数据刷新了，才把新的tmp的值赋值给P1
    {
        P1=~tmp;
        tmp_old=tmp;
    }


}