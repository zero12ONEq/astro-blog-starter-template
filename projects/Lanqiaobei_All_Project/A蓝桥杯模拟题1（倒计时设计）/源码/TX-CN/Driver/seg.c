#include <seg.h>

unsigned char Seg_Wela[] = {0xfe,0xfd,0xfb,0xf7,0xef,0xdf};//数码管位码
unsigned char Seg_Dula[] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c,0x39,0x5e,0x79,0x71,0x00};//数码管段码

void Seg_Disp(unsigned char wela,dula)
{
    //#1、消影 
    //每次位选之前，往段选锁存器里写0x00，熄灭上一个数码管，可以使得下一个数码管被点亮时，不会窜留上一个的影子
    P0=0x00; //往段选锁存器里写0x00，关闭所有段 
    //往P2_6先写1，再写0 锁存住刚刚写进段选锁存器写的0x00
    P26=1;
    P26=0;
    
    //#2、位选
    P0=Seg_Wela[wela];
    P27=1;
    P27=0;
    
    //#3、段选
    P0=Seg_Dula[dula];
    P26=1;
    P26=0;    
}