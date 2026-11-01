#include <seg.h>

//把位码和段码放在代码存储器（ROM）里而不是放在RAM里，减少对RAM的占有
code unsigned char Seg_Wela[] = {0xfe,0xfd,0xfb,0xf7,0xef,0xdf};//数码管位码 
code unsigned char Seg_Dula[] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c,0x39,0x5e,0x79,0x71,0x00};//数码管段码
//注：把A的段码当成R的段码 把5的段码当成S的段码

void Seg_Disp(unsigned char wela,dula,point)
{   
    //1、消影
    P0=0x00;
    P2_6=1;
    P2_6=0;
    
    //2、位选
    P0=Seg_Wela[wela];
    P2_7=1;
    P2_7=0;
    
    //3、段选
    if(point==1)
    {
        P0=Seg_Dula[dula]|0x80;
    }
    else
    {
        P0=Seg_Dula[dula];
    }
    P2_6=1;
    P2_6=0;    
}