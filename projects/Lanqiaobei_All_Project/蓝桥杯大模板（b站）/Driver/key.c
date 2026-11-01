#include <key.h>

/*矩阵键盘底层函数*/
unsigned char Key_Read()
{
    unsigned char temp=0;//无按键按下temp置为0
    
    P44=0;P42=1;P35=1;P34=1;//选中第一列
    if(P33==0) temp=4;//s4被按下
    if(P32==0) temp=5;//s5被按下
    if(P31==0) temp=6;//s6被按下
    if(P30==0) temp=7;//s7被按下
        
    P44=1;P42=0;P35=1;P34=1;//选中第二列
    if(P33==0) temp=8;//s8被按下
    if(P32==0) temp=9;//s9被按下
    if(P31==0) temp=10;//s10被按下
    if(P30==0) temp=11;//s11被按下
    
    P44=1;P42=1;P35=0;P34=1;//选中第三列
    if(P33==0) temp=12;//s12被按下
    if(P32==0) temp=13;//s13被按下
    if(P31==0) temp=14;//s14被按下
    if(P30==0) temp=15;//s15被按下 

    P44=1;P42=1;P35=1;P34=0;//选中第四列
    if(P33==0) temp=16;//s16被按下
    if(P32==0) temp=17;//s17被按下
    if(P31==0) temp=18;//s18被按下
    if(P30==0) temp=19;//s19被按下
    
    return temp;
}

///*独立按键的底层函数*/
//unsigned char Key_Read()
//{
//    unsigned char temp=0;
//    if(P33==0) temp=4;//s4被按下
//    if(P32==0) temp=5;//s5被按下
//    if(P31==0) temp=6;//s6被按下
//    if(P30==0) temp=7;//s7被按下

//    return temp;
//}
    