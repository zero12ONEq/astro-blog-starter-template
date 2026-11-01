#include <key.h>

/*°´¼ü¶ÁÈ¡º¯Êý*/
unsigned char Key_Read()
{
    unsigned char tmp=0;
    if(P34==0) tmp=1;
    if(P35==0) tmp=2; 
    if(P36==0) tmp=3;
    if(P37==0) tmp=4;  
    return tmp;
}