#include <key.h>

unsigned char Key_Read()
{
    unsigned char tmp=0;
    P30=0;P31=1;P32=1;P33=1;//使能第一行
    if(P34==0) tmp=1; //第一行第一列的按键被按下，返回1，以此类推
    if(P35==0) tmp=2;
    if(P36==0) tmp=3;
    if(P37==0) tmp=4;

    P30=1;P31=0;P32=1;P33=1;//使能第二行
    if(P34==0) tmp=5; //第二行第一列的按键被按下，返回5，以此类推
    if(P35==0) tmp=6;
    if(P36==0) tmp=7;
    if(P37==0) tmp=8;        
    
    P30=1;P31=1;P32=0;P33=1;//使能第三行
    if(P34==0) tmp=9; //第三行第一列的按键被按下，返回9，以此类推
    if(P35==0) tmp=10;
    if(P36==0) tmp=11;
    if(P37==0) tmp=12;        

    P30=1;P31=1;P32=1;P33=0;//使能第四行
    if(P34==0) tmp=13; //第四行第一列的按键被按下，返回13，以此类推
    if(P35==0) tmp=14;
    if(P36==0) tmp=15;
    if(P37==0) tmp=16;        
    return tmp;
}