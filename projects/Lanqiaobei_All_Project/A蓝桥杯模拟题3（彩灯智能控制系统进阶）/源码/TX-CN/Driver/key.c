#include <key.h>

unsigned char Key_Read()
{
    unsigned char tmp=0;
    P30=0;P31=1;P32=1;P33=1;
    if(P34==0) tmp=1;
    if(P35==0) tmp=2;
    if(P36==0) tmp=3;
    if(P37==0) tmp=4;
    
    P30=1;P31=0;P32=1;P33=1;
    if(P34==0) tmp=5;
    if(P35==0) tmp=6;
    if(P36==0) tmp=7;
    if(P37==0) tmp=8;

    P30=1;P31=1;P32=0;P33=1;
    if(P34==0) tmp=9;
    if(P35==0) tmp=10;
    if(P36==0) tmp=11;
    if(P37==0) tmp=12;

    P30=1;P31=1;P32=1;P33=0;
    if(P34==0) tmp=13;
    if(P35==0) tmp=14;
    if(P36==0) tmp=15;
    if(P37==0) tmp=16;  
    return tmp;
}