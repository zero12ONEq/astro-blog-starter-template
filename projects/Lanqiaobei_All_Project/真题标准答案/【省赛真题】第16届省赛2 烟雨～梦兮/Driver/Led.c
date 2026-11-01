#include "Led.h"

void Led_Display(unsigned char* ucled, bit enable)
{
  unsigned char led_temp = 0x00;
  static unsigned char led_temp_old = 0x00;
  
  if(enable)
    led_temp = (ucled[0] << 0) | (ucled[1] << 1) | (ucled[2] << 2) | (ucled[3] << 3) |
               (ucled[4] << 4) | (ucled[5] << 5) | (ucled[6] << 6) | (ucled[7] << 7);
  
  if(led_temp != led_temp_old)
  {
    led_temp_old = led_temp;
    P2 &= 0x1F;
    P0 = ~led_temp;
    P2 |= 0x80;
    P2 &= 0x1F;
  }
}