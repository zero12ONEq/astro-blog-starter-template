#include <STC15F2K60S2.H>
#include <intrins.h>

sbit TX = P1^0;
sbit RX = P1^1;

void Delay12us(void);
void Sonic_Init(void);
unsigned char Sonic_Read(void);