#include <key.h>

/*°´¼ü¶ÁÈ¡º¯Êý*/
unsigned char Key_Read()
{
    unsigned char tmp=0;
    if(P3_4==0) tmp=1;
    if(P3_5==0) tmp=2; 
    if(P3_6==0) tmp=3;
    if(P3_7==0) tmp=4;  
    return tmp;
}