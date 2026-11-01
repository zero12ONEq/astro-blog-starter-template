#include <seg.h>

code unsigned char Seg_Wela[] = {0xfe,0xfd,0xfb,0xf7,0xef,0xdf};//数码管位码
code unsigned char Seg_Dula[] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c,0x39,0x5e,0x79,0x71,0x00};//数码管段码
void Seg_Disp(unsigned char wela,dula,point)
{
    //1、消影
    P0=0x00;
    P26=1;
    P26=0;
    
    //2、位选
    P0=Seg_Wela[wela];
    P27=1;
    P27=0;
    
    //3、段选
    if(point==0)  //point的值为0，就不点亮小数点
    {
        P0=Seg_Dula[dula];
    }
    else //point的值为1，就点亮小数点
    {
        P0=Seg_Dula[dula]|0x80; //原来的段码值位或上0x80（1000 0000）就把D7位置为了1，D7位为1，小数点亮
    }
    P26=1;
    P26=0;


}