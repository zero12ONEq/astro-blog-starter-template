#include "Ds18b20.h"

unsigned char accuracy_data[4] = {0x1F, 0x3F, 0x5F, 0x7F}; // ¾«¶ÈÊý¾Ý

//
void Delay_OneWire(unsigned int t)  
{
	unsigned char i;
	while(t--){
		for(i=0;i<12;i++);
	}
}

//
bit Init_Ds18b20(void)
{
  	bit initflag = 0;
  	
  	DQ = 1;
  	Delay_OneWire(12);
  	DQ = 0;
  	Delay_OneWire(80);
  	DQ = 1;
  	Delay_OneWire(10); 
    initflag = DQ;     
  	Delay_OneWire(5);
  
  	return initflag;
}



//
void Write_DS18B20(unsigned char dat)
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		DQ = 0;
		DQ = dat&0x01;
		Delay_OneWire(5);
		DQ = 1;
		dat >>= 1;
	}
	Delay_OneWire(5);
}

//
unsigned char Read_DS18B20(void)
{
	unsigned char i;
	unsigned char dat;
  
	for(i=0;i<8;i++)
	{
		DQ = 0;
		dat >>= 1;
		DQ = 1;
		if(DQ)
		{
			dat |= 0x80;
		}	    
		Delay_OneWire(5);
	}
	return dat;
}

float Read_Temperature()
{
	unsigned char low, high;
	Init_Ds18b20();
	Write_DS18B20(0xCC);
	Write_DS18B20(0x44);
	
	EA = 0;
  Delay_OneWire(40); // ¸ù¾ÝÏÖÏóÎ¢µ÷ÊýÖµ 
  EA = 1;

	Init_Ds18b20();
	Write_DS18B20(0xCC);
	Write_DS18B20(0xBE);

	low = Read_DS18B20();
	high = Read_DS18B20();

	return ((high << 8) | low) * 0.0625;
}

void Write_Accuracy(unsigned char accuracy)
{
	Init_Ds18b20();
	Write_DS18B20(0xcc);
	Write_DS18B20(0x4e);//Ð´ÔÝ´æ¼Ä´æÆ÷ÃüÁî
	
	Write_DS18B20(0x4B);//Ð´¹ýÎÂÎÂ¶È±¨¾¯¼Ä´æÆ÷
	Write_DS18B20(0x01);//Ð´µÍÎÂÎÂ¶È±¨¾¯¼Ä´æÆ÷
	Write_DS18B20(accuracy_data[accuracy]);//Ð´ÅäÖÃ¼Ä´æÆ÷
}

void read_DS18B20_Config(unsigned char* config)
{
    unsigned char i;
		Init_Ds18b20();

    Write_DS18B20(0xcc); 
    Write_DS18B20(0xBE);

    // ¶ÁÈ¡ÅäÖÃ¼Ä´æÆ÷
		for(i = 0; i < 9; i++)
		{
			config[i] = Read_DS18B20();
		}
}
