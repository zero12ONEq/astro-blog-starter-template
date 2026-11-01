#ifndef __DS18B20_H
#define __DS18B20_H

#include <STC15F2K60S2.H>

sbit DQ = P1^4; // DS18B20 data port

bit Init_Ds18b20(void);
void Delay_OneWire(unsigned int t);
void Write_DS18B20(unsigned char dat);
unsigned char Read_DS18B20(void);
float Read_Temperature();
void Write_Accuracy(unsigned char accuracy);
void read_DS18B20_Config(unsigned char* config);
#endif