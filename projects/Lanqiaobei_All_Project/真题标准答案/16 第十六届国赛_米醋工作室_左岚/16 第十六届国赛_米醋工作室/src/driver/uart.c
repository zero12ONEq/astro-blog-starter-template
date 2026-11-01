#include "uart.h"
#include "stdio.h"
void Uart1_Init(void)
{
	SCON = 0x50;
	AUXR |= 0x01;
	AUXR &= 0xFB;
	T2L = 0xE6;
	T2H = 0xFF;
	AUXR |= 0x10;
	ES = 1;
	EA = 1;
}

extern char putchar (char ch)
{
    SBUF = ch;

    while(TI == 0)
        ;

    TI = 0;

    return ch;
}
