#include "Key.h"

unsigned char Key_Read(void)
{
  unsigned char temp = 0;
	
	ET1 = 0;
	
	P44 = 0; P42 = 1; P35 = 1; P34 = 1;
  if(P33 == 0) temp = 4;
  if(P32 == 0) temp = 5;
  if(P31 == 0) temp = 6;
  if(P30 == 0) temp = 7;
	
	P44 = 1; P42 = 0; P35 = 1; P34 = 1;
  if(P33 == 0) temp = 4 + 4;
  if(P32 == 0) temp = 5 + 4;
  if(P31 == 0) temp = 6 + 4;
  if(P30 == 0) temp = 7 + 4;
  if(P33 == 0 && P32 == 0) temp = 89;
	
	P44 = 1; P42 = 1; P35 = 0; P34 = 1;
  if(P33 == 0) temp = 4 + 8;
  if(P32 == 0) temp = 5 + 8;
  if(P31 == 0) temp = 6 + 8;
  if(P30 == 0) temp = 7 + 8;
	
	P44 = 1; P42 = 1; P35 = 1; P34 = 0;
  if(P33 == 0) temp = 4 + 12;
  if(P32 == 0) temp = 5 + 12;
  if(P31 == 0) temp = 6 + 12;
  if(P30 == 0) temp = 7 + 12;
	
	P3 = 0xff; // 手动拉高 P3 防止和串口 1 冲突
	ET1 = 1;
  
  return temp;
}
