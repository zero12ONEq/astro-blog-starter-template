#include "bsp_seg.h"

void seg_tran(unsigned char *seg_string,unsigned char *seg_buf)
{
	unsigned char temp=0;
	unsigned char i=0;
	unsigned char j=0;
	
	for(i=0;i<8;i++,j++)
	{
		switch(seg_string[j])
		{
			case '0':temp=0xc0;break;
			case '1':temp=0xf9;break;
			case '2':temp=0xa4;break;
			case '3':temp=0xb0;break;
			
			case '4':temp=0x99;break;
			case '5':temp=0x92;break;
			case '6':temp=0x82;break;
			case '7':temp=0xf8;break;
			
			case '8':temp=0x80;break;
			case '9':temp=0x90;break;
			case '-':temp=0xbf;break;
			
			
			case 'F':temp=0x8e;break;
			case 'H':temp=0x89;break;
			case 'A':temp=0x88;break;
			case 'P':temp=0x8c;break;
			case 'L':temp=0xc7;break;
			case 'E':temp=0X86;break;
				
			default:temp=0xff;break;
		}
		
		if(seg_string[j+1]=='.')
		{
			temp&=0x7f;
			j++;
		}
		
		seg_buf[i]=temp;
	}
}

void seg_disp(unsigned char *seg_buf,unsigned char pos)
{
	P0=0XFF;
	P2=P2&0X1F|0XE0;
	P2=P2&0X1F;
	
	P0=0X01<<pos;
	P2=P2&0X1F|0XC0;
	P2=P2&0X1F;
	
	P0=seg_buf[pos];
	P2=P2&0X1F|0XE0;
	P2=P2&0X1F;
}
	