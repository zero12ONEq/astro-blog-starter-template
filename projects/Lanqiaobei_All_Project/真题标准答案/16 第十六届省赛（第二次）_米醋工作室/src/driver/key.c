#include "key.h"

unsigned char Key_Read()
{
	unsigned char temp=0;
	
	P44=0;P42=1;P35=1;
	if(P32==0)temp=5;
	if(P33==0)temp=4;
	
	P44=1;P42=0;P35=1;
	if(P32==0)temp=9;
	if(P33==0)temp=8;
	
	P44=1;P42=1;P35=0;
	if(P33==0)temp=12;
	return temp;
}