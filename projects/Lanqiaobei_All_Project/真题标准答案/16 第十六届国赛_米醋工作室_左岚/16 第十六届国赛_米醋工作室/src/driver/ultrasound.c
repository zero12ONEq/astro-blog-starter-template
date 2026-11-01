#include "ultrasound.h"
#include "intrins.h"

sbit US_TX = P1 ^ 0;
sbit US_RX = P1 ^ 1;

void Delay12us(void)
{
    unsigned char data i;

    _nop_();
    i = 3;
    while (--i)
        ;
}

void Ut_Wave_Init()
{
    unsigned char i;
    EA = 0;
    for (i = 0; i < 8; i++)
    {
        US_TX = 1;
        Delay12us();
        US_TX = 0;
        Delay12us();
    }
    EA = 1;
}

unsigned char Ut_Wave_Data(unsigned int speed_10x)
{
    unsigned int time;

    CMOD = 0x00;
    CH = CL = 0;

    Ut_Wave_Init();

    CR = 1;

    while ((US_RX == 1) && (CF == 0))
        ;

    CR = 0;

    if (CF == 0)
    {
        time = CH << 8 | CL;
        // time*speed_10x/10/2/1000000*100=time*speed_10x/200000
        return ((float)time / 1000.0f * speed_10x / 200.0f);
    }
    else
    {
        CF = 0;
        return 0;
    }
}
