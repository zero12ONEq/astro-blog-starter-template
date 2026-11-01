#include "key.h" // 包含了按键相关的头文件

/**
 * @brief   读取按键函数
 * @param   无
 * @return  unsigned char - 返回按键值。如果没有按键按下或检测到串口通信，返回0。
 * @note    此函数实现了对一个3x4（可配置为4x4）矩阵键盘的扫描。
 * 一个关键的设计是：它必须与串行通信和频率测量功能和谐共存。
 * P3.0口同时用作按键扫描的列线和UART的接收线(RXD)。
 * P3.4口被保留用于频率测量。
 * 为防止冲突，函数在扫描前会检查P3.0的状态。
 */
unsigned char Key_Read()
{
    
    unsigned char temp=0; // 初始化返回值为0 (无按键)

    if(P30==0)temp=7;
    if(P31==0)temp=6;
    if(P32==0)temp=5;
    if(P33==0)temp=4;

    
    return temp; // 返回检测到的按键值，或0（无按键/串口忙）。
}