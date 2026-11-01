#include "key.h"

unsigned char Key_Read()
{
	unsigned char temp=0x00;
	P44=0;P42=1;P35=1;P34=1;
	if(P30==0) temp=7;
	if(P31==0) temp=6;
	if(P32==0) temp=5;
	if(P33==0) temp=4;
	
	P44=1;P42=0;P35=1;P34=1;
	if(P30==0) temp=11;
	if(P31==0) temp=10;
	if(P32==0) temp=9;
	if(P33==0) temp=8;
	if(P32==0&&P33==0) temp=89;
	P44=1;P42=1;P35=0;P34=1;
	if(P30==0) temp=15;
	if(P31==0) temp=14;
	if(P32==0) temp=13;
	if(P33==0) temp=12;

	P44=1;P42=1;P35=1;P34=0;
	if(P30==0) temp=19;
	if(P31==0) temp=18;
	if(P32==0) temp=17;
	if(P33==0) temp=16;

	// --- 扫描结束，恢复端口状态 ---
	// 确保所有行线在函数退出时都恢复为高电平。
	P44=1;P42=1;P35=1;//P34=1;
	
	// 下面这行代码是为了在不影响P3.4（频率测量引脚）的情况下，将P3口的其他引脚设置为高阻输入状态。
	// 在8051架构中，向端口位写入'1'会使其配置为高阻输入，为下一次正确读取做准备。
	// 0xef = 1110 1111b，通过按位或操作，P3.4保持原样，其他位被置1。
	P3=P3|0xef;
	return temp;
}