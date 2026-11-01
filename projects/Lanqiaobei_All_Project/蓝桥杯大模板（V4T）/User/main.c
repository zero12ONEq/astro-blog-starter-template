#include <STC15F2K60S2.H>
#include "led.h"
#include "init.h"
#include "seg.h"
#include "key.h"
#include "ds1302.h"
#include "onewire.h"
#include "iic.h"
#include "ultrasound.h"
#include "uart.h"
// 包含标准库头文件
#include "string.h" // 字符串处理函数
#include "stdio.h"	// 标准输入输出函数，如 printf 和 sscanf
pdata unsigned char ucled[8]={0,0,0,0,0,0,0,0};//led初始化为全灭
pdata unsigned char Seg_Buf[8]={16,16,16,16,16,16,16,16};//数码管显示配置数组，初始化为全灭
idata unsigned char Seg_Pos=0;//数码管扫描的位置
idata unsigned char Seg_Slow_Down;
idata unsigned char Key_Val,Key_Old,Key_Up,Key_Down;
idata unsigned char Key_Slow_Down;
pdata unsigned char ucRtc[]={11,12,13};
idata unsigned char Time_Slow_Down;
idata unsigned int Temperature_10x;
idata unsigned int Temperature_Slow_Down;
idata unsigned int AD_1_Data_100x,AD_3_Data_100x;//当连续读取时通道1和通道3会交换
idata unsigned char AD_DA_Slow_Down;
pdata unsigned char EEPROM_Data_W[8]={1,2,3,4,5,6,7,8};
pdata unsigned char EEPROM_Data_R[8]={0,0,0,0,0,0,0,0};
idata unsigned char Distance;//0-255cm 0-2.55m
idata unsigned char Distance_Slow_Down;
idata unsigned int Freq;//一秒钟之内捕获的外部脉冲的个数
idata unsigned int Time_1s;
idata unsigned char pwm_period;//pwm周期内的累加计数器
idata unsigned char pwm_compare=6;//pwm比较值，决定占空比，控制LED亮度 (0-9)

// 串口接收相关变量
idata unsigned char Uart_Rx_Index;									  // 串口接收缓冲区索引
pdata unsigned char Uart_Rx_Buf[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // 串口接收缓冲区。缓冲区的大小取决于我们要接收多大的东西
idata unsigned char Uart_Rx_Flag;									  // 串口接收到数据的标志
idata unsigned char Uart_Rx_Tick;									  // 串口接收超时解析计时器

//0:时间, 1:温度, 2:AD值, 3:超声波距离, 4:频率, 5:PWM参数
idata unsigned char Seg_Disp_Mode;
void Key_Proc()
{
	if(Key_Slow_Down<10) return;//按键消抖 每10ms才检测一次哪个按键被按下
	Key_Slow_Down=0;
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Val^Key_Old);
	Key_Up=~Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;
	if(Key_Down==4)
		pwm_compare=(++pwm_compare)%10;//按下按键4，pwm_compare在0-9之间循环，人为调节占空比
	// 如果有任何按键被按下，通过串口打印按键值 通过串口调试按键
	if (Key_Down != 0)
		printf("Key_Down: %bu", Key_Down);
	//按按键5在0-5个数码管界面之间进行切换
	if(Key_Down==5)
		Seg_Disp_Mode=(++Seg_Disp_Mode)%6;//0-5之间循环
}

void Seg_Proc()
{
	if(Seg_Slow_Down<20)return;//每20ms刷新一下数码管的显示内容（20ms是随便给的数据，根据具体题目的要求来设定）
	Seg_Slow_Down=0;
	switch(Seg_Disp_Mode)
	{
		case 0: //时间界面
			Seg_Buf[0]=ucRtc[0]/10%10;
			Seg_Buf[1]=ucRtc[0]%10;
			Seg_Buf[2]=16+',';//显示小数点
			Seg_Buf[3]=ucRtc[1]/10%10;
			Seg_Buf[4]=ucRtc[1]%10;
			Seg_Buf[5]=16+',';//显示小数点
			Seg_Buf[6]=ucRtc[2]/10%10;
			Seg_Buf[7]=ucRtc[2]%10;
			break;
		case 1: //温度界面
			Seg_Buf[0]=Temperature_10x/100%10;
			Seg_Buf[1]=Temperature_10x/10%10+',';
			Seg_Buf[2]=Temperature_10x%10;
			Seg_Buf[3]=16;
			Seg_Buf[4]=16;
			Seg_Buf[5]=16;
			Seg_Buf[6]=16;
			Seg_Buf[7]=16;
			break;
		case 2: //AD界面
			Seg_Buf[0] = AD_1_Data_100x/100%10 + ','; // 通道1(个位)并附加小数点
			Seg_Buf[1] = AD_1_Data_100x/10%10;		  // 通道1(小数位)
			Seg_Buf[2] = AD_1_Data_100x%10;			  // 通道1(小数位)
			Seg_Buf[3] = 16;							  // 熄灭
			Seg_Buf[4] = 16;							  // 熄灭
			Seg_Buf[5] = AD_3_Data_100x/100%10 + ','; // 通道3(个位)并附加小数点
			Seg_Buf[6] = AD_3_Data_100x/10%10;		  // 通道3(小数位)
			Seg_Buf[7] = AD_3_Data_100x%10;			  // 通道3(小数位)
			break;
		
		case 3: //超声波测距
			Seg_Buf[0]=Distance/100%10;
			Seg_Buf[1]=Distance/10%10;
			Seg_Buf[2]=Distance%10;
			Seg_Buf[3]=16;
			Seg_Buf[4]=16;
			Seg_Buf[5]=16;
			Seg_Buf[6]=16;
			Seg_Buf[7]=16;			
			break;
		
		case 4: //频率界面
			Seg_Buf[0]=(Freq>10000000)?Freq/10000000%10:16;
			Seg_Buf[1]=(Freq>1000000)?Freq/1000000%10:16;
			Seg_Buf[2]=(Freq>100000)?Freq/100000%10:16;
			Seg_Buf[3]=(Freq>10000)?Freq/10000%10:16;
			Seg_Buf[4]=(Freq>1000)?Freq/1000%10:16;
			Seg_Buf[5]=(Freq>100)?Freq/100%10:16;
			Seg_Buf[6]=(Freq>10)?Freq/10%10:16;
			Seg_Buf[7]=Freq%10;			
			break;
		
		case 5: //PWM设置参数显示界面
			Seg_Buf[0]=pwm_compare;
			Seg_Buf[1]=16;
			Seg_Buf[2]=16;
			Seg_Buf[3]=16;
			Seg_Buf[4]=16;
			Seg_Buf[5]=16;
			Seg_Buf[6]=16;
			Seg_Buf[7]=16;			
			break;
	}
}

void Led_Proc()
{
	ucled[0]=1;
	ucled[1]=0;
	ucled[2]=1;
	ucled[3]=0;
	ucled[4]=1;
	ucled[5]=0;
	ucled[6]=1;
	ucled[7]=0;
	//Led_Disp(ucled);

}
//获取时间函数
void Get_Time()
{
	if(Time_Slow_Down<100) return;//每100ms读取一次新的时间值（120ms是随便给的数据，根据具体题目的要求来设定）
	Time_Slow_Down=0;
	Read_Rtc(ucRtc);
}

//获取温度的函数
void Get_Temperature()
{
	if(Temperature_Slow_Down<300) return;//每300ms读取一次温度（300ms是随便给的数据，根据具体题目的要求来设定）
	Temperature_Slow_Down=0;
	Temperature_10x=rd_temperature()*10;
}

//AD/DA转换函数
void AD_DA()
{
	if(AD_DA_Slow_Down<120) return;//每120ms读取一次转换得到的数据量 和传入数据量转化成模拟量（120ms是随便给的数据，根据具体题目的要求来设定）
	AD_DA_Slow_Down=0;
	AD_3_Data_100x=(float)Ad_Read(0x41)/51.0f*100;//数字量显示成模拟量的形式 连续读取时通道1变成滑动变阻器 通道3变成光敏电阻
	AD_1_Data_100x=(float)Ad_Read(0x43)/51.0f*100;
	Da_Write(3*51);//将3*51的数据量转化成3v的模拟量
}

//获取距离函数
void Get_Distance()
{
	if(Distance_Slow_Down<150) return; //每150ms测量一次距离（150ms是随便给的数据，根据具体题目的要求来设定）
	Distance_Slow_Down=0;
	Distance=Ut_Wave_Data();
}
//串口处理函数
void Uart_Proc()
{
	unsigned char x,y;
	if(Uart_Rx_Index==0) return;//缓冲区索引值没变化，说明缓冲区没存入数据 说明串口没接收到数据
	//检查是否超时（距上次接收超过10ms） 然后进行解析 (超时解析)
	if(Uart_Rx_Tick>=10)
	{
		Uart_Rx_Flag=0;//串口接收到数据标志位重置为0
		Uart_Rx_Tick=0;//计时器重置为0
		printf("%s", Uart_Rx_Buf); // 将收到的数据回显到串口，用于调试

		// 尝试按"(x,y)"的格式解析数据
		if (sscanf(Uart_Rx_Buf, "(%bu,%bu)", &x, &y) == 2)
			// 解析成功，打印解析出的x和y
			printf("\r\nI Get x=%bu,y=%bu\r\n", x, y);
		else
			// 解析失败，打印错误信息
			printf("\r\nERROR\r\n");

		// 清空接收缓冲区，为下次接收做准备
		memset(Uart_Rx_Buf, 0, Uart_Rx_Index);
		Uart_Rx_Index = 0;
	}

}	
	
//定时器0的初始化函数
//配置为计数器模式，用于测量外部脉冲频率。频率测量只能用定时器0，因为定时器0对应P34引脚
void Timer0Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TMOD |= 0x05;   //开启计数器模式，且不自动重装载
	TL0 = 0x00;		//设置定时初始值
	TH0 = 0x00;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
}



//定时器1的初始化函数
void Timer1Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;		//定时器时钟12T模式
	TMOD &= 0x0F;		//设置定时器模式
	TL1 = 0x18;		//设置定时初始值
	TH1 = 0xFC;		//设置定时初始值
	TF1 = 0;		//清除TF1标志
	TR1 = 1;		//定时器1开始计时
	ET1 = 1;    //使能定时器1中断
	EA = 1;     //使能全局中断
}

//定时器1的服务中断函数
void Timer1_Isr(void) interrupt 3
{
	Seg_Slow_Down++;
	Key_Slow_Down++;
	Time_Slow_Down++;
	Temperature_Slow_Down++;
	AD_DA_Slow_Down++;
	Distance_Slow_Down++;
	//如果我们的底层段码没有20的下标（超出上限了），也就代表着我们+了一个','，显示小数点
	//如我们在Seg_Buf数组里写46（44+2）表示显示'2.' 写49（44+5）表示显示'5.'
	if(Seg_Buf[Seg_Pos]>20)
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-',',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);//正常显示且不显示小数点
	
	Seg_Pos=(++Seg_Pos)%8;//数码管扫描0-7位 
	if(++Time_1s==1000) //频率测量，每1s统计一次捕获的个数
	{
		Time_1s=0;
		Freq=(TH0<<8)|TL0;// 读取T0的计数值，即为过去1秒内的脉冲数(Hz)
		TH0=TL0=0;// 清空T0计数器，开始下一个周期的测量
	}
	
	//调光
	//每1ms执行一次定时器1的中断服务函数，那么当pwm_period=0到5时，led亮，也就是亮6ms 
	//然后当pwm_period=6-9时，led不亮，这样就达到了10ms内 6ms亮 4ms不亮，这是一种亮度等级
	//pwm_compare的值越大，亮所占的时间就越长，最终亮度等级也就越高
	if(pwm_period<pwm_compare)
		Led_Disp(ucled);
	else
		Led_Off();//关闭所有led
	pwm_period=(++pwm_period)%10;//pwm_period在0-9之间循环
	
	if(Uart_Rx_Flag) Uart_Rx_Tick++;//如果串口正在接收数据，启动超时计时器
}
//串口1中断服务函数 当串口接收到数据时触发。
void Uart1_Isr(void) interrupt 4
{
	if (RI)				//检测串口1接收中断
	{
		Uart_Rx_Flag=1;//将标志位置为1，表示串口接收到数据
		Uart_Rx_Tick=0;//重置超时计时器
		Uart_Rx_Buf[Uart_Rx_Index++]=SBUF;//我们将串口接收到的数据存入缓冲区
		RI = 0;			//清除串口1接收中断请求位
		if(Uart_Rx_Index>10)//如果溢出了
		{
			Uart_Rx_Index=0;//归零索引值
			memset(Uart_Rx_Buf,0,10);//批量清空缓冲区
		}
	}
}

void main()
{   
    System_Init();
		Set_Rtc(ucRtc);
		//EEPROM的读取也要在定时器1之前，否则会导致莫名其妙的问题
		EEPROM_Read(EEPROM_Data_R,0,8);//从第0位开始读，读8个数据 （没写入之前读一次）
		EEPROM_Write(EEPROM_Data_W,0,8);//从第0位开始写，写8个数据
		EEPROM_Read(EEPROM_Data_R,0,8);//从第0位开始读，读8个数据 （写入之后再读一次，看看是否真的写入了）
		Timer0Init();//定时器0的初始化要在定时器1之前，否则采集到的频率会偏小
		Uart1_Init();//串口初始化 一般放在定时器0和定时器1的初始化之间
		Timer1Init();
    while(1)
    {
			Key_Proc();
			Seg_Proc();
			Led_Proc();
			Get_Time();
			Get_Temperature();
			AD_DA();
			Get_Distance();
			Uart_Proc();
		}

}