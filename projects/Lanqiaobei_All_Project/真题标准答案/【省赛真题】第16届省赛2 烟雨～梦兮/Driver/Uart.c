#include "Uart.h"

void Uart1_Init(void)	//9600bps@12.000MHz
{
	SCON = 0x50;		//8位数据,可变波特率
	AUXR |= 0x01;		//串口1选择定时器2为波特率发生器
	AUXR |= 0x04;		//定时器时钟1T模式
	T2L = 0xC7;			//设置定时初始值
	T2H = 0xFE;			//设置定时初始值
	AUXR |= 0x10;		//定时器2开始计时
	ES = 1; // 开启串口中断
}

extern char putchar(char ch)
{
	SBUF = ch;
	while(TI == 0);
	TI = 0;
	return ch;
}

///* 串口 1 中断服务函数 */
//void Uart1_Server(void) interrupt 4
//{
//	if (RI)				//检测串口1接收中断
//	{
//		uart_recv_tick = 0;
//		uart_rx_data[uart_rx_data_index] = SBUF;
//		RI = 0;			//清除串口1接收中断请求位
//		if(++uart_rx_data_index == UART_RX_SIZE) uart_rx_data_index = 0;
//	}
//}

///* 串口处理函数 */
//void Uart_Proc()
//{
//	if(uart_rx_data_index == 0) return; // 没有数据则退出
//	if(uart_recv_tick >= 10) // 超时解析
//	{
//		uart_recv_tick = 0;
//		/* 串口数据解析区域 */

//		memset(uart_rx_data, 0, uart_rx_data_index); // 清空接收数据
//		uart_rx_data_index = 0;
//	}
//}