#include "ultrasound.h"
#include "intrins.h"
sbit US_TX=P1^0;
sbit US_RX=P1^1;
void Delay12us(void)	//@12.000MHz
{
	unsigned char data i;

	_nop_();
	i = 3;
	while (--i);
}

void Ut_Wave_Init()
{
	unsigned char i;
	EA=0;
	for(i=0;i<8;i++)
	{
		
		US_TX=1;
		Delay12us();
		US_TX=0;
		Delay12us();
	}
	EA=1;
}

unsigned char Ut_Wave_Data()
{
	unsigned int time;
	CMOD=0x00;
	CH=CL=0;
	
	Ut_Wave_Init();
	CR=1;
	
	while((US_RX==1)&&(CF==0));
	CR=0;
	if(CF==0)
	{
		time= CH<<8|CL;
		return (time*0.017);
	}
	else
	{
		CF=0;
		return 0;
	}
}