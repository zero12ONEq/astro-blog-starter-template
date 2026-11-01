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

void ut_wave_init()
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

unsigned char ut_wave_data()
{
	unsigned int time;
	CMOD=0x00;
	CH=CL=0;
	ut_wave_init();
	CR=1;
	while((US_RX==1)&&(CF==0));
	CR=0;
	if(CF==0)
	{
		time=CH<<8|CL;
		return time*0.017;
	}
	else
	{
		CF=0;
		return 0;
	}
}

//unsigned int ut_wave_data(float speed)
//{
//	float temp=0;
//	unsigned int time;
//	CMOD=0x00;
//	CH=CL=0;
//	ut_wave_int();
//	CR=1;
//	while(US_RX==1)&&(CF==0));
//	CR=0;
//	if(CF==0)
//	{
//		time=CH<<8|CL;
//		temp=(float)time*speed/20000.0f;
//		return temp;
//	}
//	else
//	{
//		CF=0;
//		return 999;
//	}
//}