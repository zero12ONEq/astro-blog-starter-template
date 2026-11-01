#include "Key.h"

unsigned char Key_Read(void)
{
  unsigned char temp = 0;
	
	EA = 0;
  
  if(P33 == 0) temp = 4;
  if(P32 == 0) temp = 5;
  if(P31 == 0) temp = 6;
  if(P30 == 0) temp = 7;
	
	EA = 1;
  
  return temp;
}
