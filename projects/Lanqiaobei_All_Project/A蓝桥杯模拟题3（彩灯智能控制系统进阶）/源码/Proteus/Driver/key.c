#include <key.h>

unsigned char Key_Read()
{
    unsigned char tmp=0;
    P3_0=0;P3_1=1;P3_2=1;P3_3=1;
    if(P3_4==0) tmp=1;
    if(P3_5==0) tmp=2;
    if(P3_6==0) tmp=3;
    if(P3_7==0) tmp=4;
    
    P3_0=1;P3_1=0;P3_2=1;P3_3=1;
    if(P3_4==0) tmp=5;
    if(P3_5==0) tmp=6;
    if(P3_6==0) tmp=7;
    if(P3_7==0) tmp=8;

    P3_0=1;P3_1=1;P3_2=0;P3_3=1;
    if(P3_4==0) tmp=9;
    if(P3_5==0) tmp=10;
    if(P3_6==0) tmp=11;
    if(P3_7==0) tmp=12;

    P3_0=1;P3_1=1;P3_2=1;P3_3=0;
    if(P3_4==0) tmp=13;
    if(P3_5==0) tmp=14;
    if(P3_6==0) tmp=15;
    if(P3_7==0) tmp=16;  
    return tmp;
}