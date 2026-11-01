#include <STC15F2K60S2.H>
#include <intrins.h>

sbit sda = P2^1;
sbit scl = P2^0;

static void I2C_Delay(unsigned char n);
void I2CStart(void);
void I2CStop(void);
void I2CSendByte(unsigned char byt);
unsigned char I2CReceiveByte(void);
unsigned char I2CWaitAck(void);
void I2CSendAck(unsigned char ackbit);

unsigned char AD_Read(unsigned char port);
void DA_Output(float voltage);

void EEPROM_Clean(unsigned char addr);
void EEPROM_Write(unsigned char* dat, unsigned char addr, unsigned char len);
void EEPROM_Read(unsigned char* dat, unsigned char addr, unsigned char len);