#include "Led.h"

void Led_Display(unsigned char addr, enable)
{
  static unsigned char temp = 0x00;
  static unsigned char temp_Old = 0xFF;
  
  if(enable)
    temp |= 0x01 << addr;
  else
    temp &= ~(0x01 << addr);
  
  if(temp != temp_Old)
  {
    P0 = ~temp;
		P2 = P2 & 0x1F | 0x80;
		P2 &= 0x1F;
    temp_Old = temp;
  }
}