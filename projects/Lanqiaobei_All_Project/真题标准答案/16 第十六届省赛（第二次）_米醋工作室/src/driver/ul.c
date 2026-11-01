#include "ul.h"
#include "intrins.h"

sbit US_TX=P1^0;
sbit US_RX=P1^1;

void Delay12us(void)	//@12.000MHz
{
	unsigned char data i;

	_nop_();
	_nop_();
	i = 38;
	while (--i);
}

void Ut_Wave_Init()
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		US_TX=1;
		Delay12us();
		US_TX=0;
		Delay12us();
	}
}

unsigned int Ut_Wave_Data(float speed)
{
	unsigned int time;
	float temp=0;
	CMOD = 0x00;
	CH=CL=0;
	
	Ut_Wave_Init();
	
	CR=1;
	
	while((US_RX==1)&&(CF==0));
	
	CR=0;
	
	if(CF==0)
	{
		time=CH<<8|CL;
		temp=(float)time*speed/20000.0f;
		return temp;
	}
	else 
	{
		CF=0;
		return 999;
	}
}