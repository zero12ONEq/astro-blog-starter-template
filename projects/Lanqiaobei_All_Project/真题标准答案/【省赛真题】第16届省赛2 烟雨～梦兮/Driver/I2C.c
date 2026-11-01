#include "I2C.h"


#define DELAY_TIME	10

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

void I2CStart(void)
{
    sda = 1;
    scl = 1;
	I2C_Delay(DELAY_TIME);
    sda = 0;
	I2C_Delay(DELAY_TIME);
    scl = 0;    
}

void I2CStop(void)
{
    sda = 0;
    scl = 1;
	I2C_Delay(DELAY_TIME);
    sda = 1;
	I2C_Delay(DELAY_TIME);
}

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

float AD_Read(unsigned char port) // 通道 0（外部输入）通道 1（光敏电阻）， 通道 3（滑动变阻器）
{
	unsigned char temp = 0;

	I2CStart();
	I2CSendByte(0x90); //写地址
	I2CWaitAck();
	I2CSendByte(0x40 | port); //控制字节
	I2CWaitAck();
	
//	EA = 0;
//	I2C_Delay(40); // 根据现象微调数值 
//	EA = 1;
  
  I2CStart();
	I2CSendByte(0x91); //读地址
	I2CWaitAck();
	temp = I2CReceiveByte();
	
	I2CStart();
	I2CSendByte(0x91); //读地址
	I2CWaitAck();
	temp = I2CReceiveByte();
	I2CSendAck(1);
	I2CStop();

	return temp / 51.0;
}

void DA_Output(float voltage)
{
	I2CStart();
	I2CSendByte(0x90); //写地址
	I2CWaitAck();
	I2CSendByte(0x40); //控制字节
	I2CWaitAck();
  
  EA = 0;
	I2C_Delay(40); // 根据现象微调数值 
	EA = 1;
  
	I2CSendByte((unsigned char)(voltage * 51)); //数字量
	I2CWaitAck();
	I2CStop();
}

void EEPROM_Clean(unsigned char addr)
{
	unsigned char i;
	I2CStart();
	I2CSendByte(0xA0); //写地址
	I2CWaitAck();
	I2CSendByte(addr); //需要写入的目标地址
	I2CWaitAck();
	for(i = 0; i < 8; i++)
	{
		I2CSendByte(0x00); //写数据
		I2CWaitAck();
		I2C_Delay(200);
	}
	I2CStop();
	I2C_Delay(200);
}

void EEPROM_Write(unsigned char* dat, unsigned char addr, unsigned char len)
{
	I2CStart();
	I2CSendByte(0xA0); //写地址
	I2CWaitAck();
	I2CSendByte(addr); //需要写入的目标地址
	I2CWaitAck();
	while(len--)
	{
		I2CSendByte(*dat++); //写数据
		I2CWaitAck();
		I2C_Delay(200);
	}
	I2CStop();
	I2C_Delay(200);
  I2C_Delay(200);
  I2C_Delay(200);
  I2C_Delay(200);
  I2C_Delay(200);
  I2C_Delay(200);
}

void EEPROM_Read(unsigned char* dat, unsigned char addr, unsigned char len)
{
	I2CStart();
	I2CSendByte(0xA0); //写地址
	I2CWaitAck();
	I2CSendByte(addr); //需要读取的目标地址
	I2CWaitAck();
	
	I2CStart();
	I2CSendByte(0xA1); //读地址
	I2CWaitAck();
	
	while(len--)
	{
		*dat++ = I2CReceiveByte(); //读数据
		if(len)
			I2CSendAck(0);
		else
			I2CSendAck(1);
	}
	I2CStop();
	I2C_Delay(200);
  I2C_Delay(200);
  I2C_Delay(200);
  I2C_Delay(200);
  I2C_Delay(200);
  I2C_Delay(200);
}

