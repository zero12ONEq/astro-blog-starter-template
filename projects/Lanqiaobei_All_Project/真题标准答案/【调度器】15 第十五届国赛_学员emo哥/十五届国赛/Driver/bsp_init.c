#include "bsp_init.h"

void cls_per_init()
{
	P0=0XFF;
	P2=P2&0X1F|0X80;
	P2=P2&0X1F;
	
	P0=0X00;
	P2=P2&0X1F|0XA0;
	P2=P2&0X1F;
}
