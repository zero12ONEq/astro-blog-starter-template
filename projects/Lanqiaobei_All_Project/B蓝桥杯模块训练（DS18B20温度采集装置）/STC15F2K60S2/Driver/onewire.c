#include "onewire.h"
#include "reg52.h"

sbit DQ=P1^4;

//单总线延时函数
void Delay_OneWire(unsigned int t)  
{
	unsigned char i;
	while(t--){
		for(i=0;i<12;i++);
	}
}

//DS18B20初始化函数
bit Init_DS18B20(void)
{
    bit init_flag=0;
    DQ=1;
    Delay_OneWire(12);
    DQ=0;
    Delay_OneWire(80);
    DQ=1;
    Delay_OneWire(10);
    init_flag=DQ;
    Delay_OneWire(5);
    
    return init_flag;
}

//写DS18B20 传入参数1个字节
void Write_DS18B20(unsigned char dat)
{
    unsigned char i;
    for(i=0;i<8;++i)
    {
        DQ=0;
        DQ=dat&0x01;
        Delay_OneWire(5);
        DQ=1;
        dat>>=1;
    
    }
    Delay_OneWire(5);
}

//读DS18B20 返回值1个字节
unsigned char Read_DS18B20(void)
{
    unsigned char i;
    unsigned char dat;
    for(i=0;i<8;++i)
    {
        DQ=0;
        dat>>=1;
        DQ=1; 
        if(DQ)
        {
            dat|=0x80;
        }
        Delay_OneWire(5);
    }
    return dat;
}

//温度读取函数 返回值温度
float Read_T(void)
{
    unsigned char low,high;
    Init_DS18B20();
    Write_DS18B20(0xcc);
    Write_DS18B20(0x44);

    Init_DS18B20();
    Write_DS18B20(0xcc);
    Write_DS18B20(0xbe);
    low=Read_DS18B20();
    high=Read_DS18B20();
    return ((high<<8)|low)/16.0;

}

//精度控制函数 传入参数是写入8位精度配置寄存器的值
void Config_Temper(unsigned char dat)
{
	Init_DS18B20();
	Write_DS18B20(0xcc);
	Write_DS18B20(0x4e);//写暂存寄存器命令
	Write_DS18B20(0x7f);//写过温温度报警寄存器
	Write_DS18B20(0x00);//写低温温度报警寄存器
	Write_DS18B20(dat);//写配置寄存器
	Init_DS18B20();
}