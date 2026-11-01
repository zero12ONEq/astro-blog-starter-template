/*	#   I2C代码片段说明
	1. 	本文件夹中提供的驱动代码供参赛选手完成程序设计参考。
	2. 	参赛选手可以自行编写相关代码或以该代码为基础，根据所选单片机类型、运行速度和试题
		中对单片机时钟频率的要求，进行代码调试和修改。
*/
#include "iic.h"
#include "intrins.h"
#define DELAY_TIME	5   //这个值官方给的是10，最好改成5
sbit scl=P2^0;
sbit sda=P2^1;
//
static void I2C_Delay(unsigned char n)
{
    do
    {
        _nop_();_nop_();_nop_();_nop_();_nop_();
        _nop_();_nop_();_nop_();_nop_();_nop_();
        _nop_();_nop_();_nop_();_nop_();_nop_();		
    }
    while(n--);      	
}

//
void I2CStart(void)
{
    sda = 1;
    scl = 1;
	I2C_Delay(DELAY_TIME);
    sda = 0;
	I2C_Delay(DELAY_TIME);
    scl = 0;    
}

//
void I2CStop(void)
{
    sda = 0;
    scl = 1;
	I2C_Delay(DELAY_TIME);
    sda = 1;
	I2C_Delay(DELAY_TIME);
}

//
void I2CSendByte(unsigned char byt)
{
    unsigned char i;
	
    for(i=0; i<8; i++){
        scl = 0;
		I2C_Delay(DELAY_TIME);
        if(byt & 0x80){
            sda = 1;
        }
        else{
            sda = 0;
        }
		I2C_Delay(DELAY_TIME);
        scl = 1;
        byt <<= 1;
		I2C_Delay(DELAY_TIME);
    }
	
    scl = 0;  
}

//
unsigned char I2CReceiveByte(void)
{
	unsigned char da;
	unsigned char i;
	for(i=0;i<8;i++){   
		scl = 1;
		I2C_Delay(DELAY_TIME);
		da <<= 1;
		if(sda) 
			da |= 0x01;
		scl = 0;
		I2C_Delay(DELAY_TIME);
	}
	return da;    
}

//
unsigned char I2CWaitAck(void)
{
	unsigned char ackbit;
	
    scl = 1;
	I2C_Delay(DELAY_TIME);
    ackbit = sda; 
    scl = 0;
	I2C_Delay(DELAY_TIME);
	
	return ackbit;
}

//
void I2CSendAck(unsigned char ackbit)
{
    scl = 0;
    sda = ackbit; 
	I2C_Delay(DELAY_TIME);
    scl = 1;
	I2C_Delay(DELAY_TIME);
    scl = 0; 
	sda = 1;
	I2C_Delay(DELAY_TIME);
}
/*
	0-255 <-> 0-5v
	D6置为1就代表开启DA
	0x40/0x00 外部输入
	0x41/0x01 光敏电阻
	0x42/0x02 差分电路
	0x43/0x03 滑动变阻器
*/
unsigned char Ad_Read(unsigned char addr)
{
	unsigned char temp;
	I2CStart();//发送起始位
	I2CSendByte(0x90);//寻址PCF8591，进行写操作
	I2CWaitAck();//等待从机（PCF8591）应答
	I2CSendByte(addr);//发送0-3某个通道的地址，告诉PCF8591去哪个通道读取模拟量并转化为数字量
	I2CWaitAck();//等待从机（PCF8591）应答
	
	I2CStart();//发送起始位
	I2CSendByte(0x91);//寻址PCF8591，进行读操作
	I2CWaitAck();//等待从机（PCF8591）应答
	temp=I2CReceiveByte();//将PCF8591转化后得到的数字量读取出来
	
	I2CSendAck(1);//主机发送NACK，非应答
	I2CStop();//发送停止位
	return temp;
}


void Da_Write(unsigned char dat)
{
	I2CStart();//发送起始位
	I2CSendByte(0x90);//寻址PCF8591，进行写操作
	I2CWaitAck();//等待从机（PCF8591）应答
	I2CSendByte(0x41);//发送0x40-0x43中任意一个数，目的是把D6置为1使能DA
	I2CWaitAck();//等待从机（PCF8591）应答
	
	I2CSendByte(dat);//向PCF8591发送数字量
	I2CWaitAck();//等待从机（PCF8591）应答
	I2CStop();//发送停止位
}

//传入参数：写入的数组 写入的起始地址 写入的数量
void EEPROM_Write(unsigned char*str,unsigned char addr,unsigned char num)
{
	I2CStart();//发送起始位
	I2CSendByte(0xa0);//寻址AT24C02，进行写操作
	I2CWaitAck();//等待从机（AT24C02）应答
	
	I2CSendByte(addr);//写入起始地址，后面要写入的数据从这一个起始地址开始往后填充
	I2CWaitAck();//等待从机（AT24C02）应答
	
	while(num--)
	{
		I2CSendByte(*str++);
		I2CWaitAck();
		I2C_Delay(200);//延时，保证完全写入
	}
	I2CStop();//发送停止位
	I2C_Delay(255);//10个delay保证时序完全正常
	I2C_Delay(255);
	I2C_Delay(255);
	I2C_Delay(255);
	I2C_Delay(255);
	I2C_Delay(255);
	I2C_Delay(255);
	I2C_Delay(255);
	I2C_Delay(255);
	I2C_Delay(255);
}

//传入参数：读取数据存放的数组 读取位置的起始地址 读取的数量
void EEPROM_Read(unsigned char*str,unsigned char addr,unsigned char num)
{
	I2CStart();//发送起始位
	I2CSendByte(0xa0);//寻址AT24C02，进行写操作
	I2CWaitAck();//等待从机（AT24C02）应答
	
	I2CSendByte(addr);//读取的起始地址
	I2CWaitAck();//等待从机（AT24C02）应答
	
	I2CStart();//发送起始位
	I2CSendByte(0xa1);//寻址AT24C02，进行读操作
	I2CWaitAck();//等待从机（AT24C02）应答
	
	//EA=0; 如果在读EEPROM时是频繁要发生中断的地方时，如有按键操作时，要关闭全局中断，保证读取数据的完整
	while(num--)
	{
		*str++ = I2CReceiveByte();//读取的数据依次存放到数组里
		if(num)
			I2CSendAck(0);//如果没读取完，主机发送ACK应答信号，继续读取
		else
			I2CSendAck(1);//读取完了，主机发送NACK非应答信号，停止读取
	}
	I2CStop();
}