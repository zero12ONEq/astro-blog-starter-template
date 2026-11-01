#include "bsp_ultro.h"

sbit TX=P1^0;
sbit RX=P1^1;

void Delay12us()		//@12.000MHz
{
	unsigned char i;

	_nop_();
	_nop_();
	i = 33;
	while (--i);
}

void wave_init()
{
	unsigned char i=0;
	
	EA=0;
	for(i=0;i<8;i++)
	{
	TX=1;
	Delay12us();
	TX=0;
	Delay12us();
	}	
	EA=1;
}

unsigned char wave_dist()
{
	unsigned char dist;
	
	CMOD=0X00;
	CH=CL=0;
	wave_init();
	CR=1;
	while((RX==1)&&(CF==0));
	CR=0;
	if(CF==1)
	{
		CF=0;
		dist=255;
	}
	else
		dist=((CH<<8)|CL)*0.017;
	
	return dist;
}
