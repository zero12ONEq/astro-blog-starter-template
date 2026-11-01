// 包含 STC15F2K60S2 单片机的头文件，提供了寄存器定义等
#include <STC15F2K60S2.H>

// 包含自定义的初始化、外设驱动和功能模块的头文件
#include "init.h" // 系统初始化
#include "led.h"  // LED 模块
#include "seg.h"  // 数码管模块
#include "key.h"  // 按键模块

#include "onewire.h"	// 单总线协议，用于温度传感器
#include "iic.h"		// I2C 协议，用于 ADC/DAC 和 EEPROM


// 包含标准库头文件
#include "string.h" // 字符串处理函数
#include "stdio.h"	// 标准输入输出函数，如 printf 和 sscanf

// 全局变量定义

// `idata` 存储在内部高速 RAM 中，访问速度快
// 系统滴答计时器，由 Timer1 中断每毫秒增加一次
idata unsigned long int uwTick;

// `pdata` 存储在外部 RAM 的分页区域，访问速度较快
// LED灯的状态数组，对应8个LED的亮灭
pdata unsigned char ucLed[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// 数码管显示缓冲区，存储要在8位数码管上显示的数字或符号
pdata unsigned char Seg_Buf[8] = {10, 10, 10, 10, 10, 10, 10, 10}; // 10代表不显示
// 当前正在刷新的数码管位置索引 (0-7)
idata unsigned char Seg_Pos = 0;

// 按键状态变量
idata unsigned char Key_Val;  // 当前按键值
idata unsigned char Key_Old;  // 上一次的按键值，用于检测边沿
idata unsigned char Key_Up;	  // 按键抬起事件标志
idata unsigned char Key_Down; // 按键按下事件标志

// `idata` 存储在内部高速 RAM 中
// 存储放大10倍的温度值，以处理一位小数
idata unsigned int Temperature_10x=0;//实时采取的温度
idata unsigned int Temperature_10xOld=0;//上一次采取的温度
idata unsigned int Correct_10x=0;//校准后的10x温度
idata unsigned char Temp_Count = 0;  // 温度采样稳定计数器

// AD转换通道1和通道3的数据，同样乘以100以处理两位小数
idata unsigned int AD_1_Data_100x, AD_3_Data_100x;

//题目要求的变量------------------------
idata unsigned char Seg_Mode=0;//0温度界面，1DAC输出控制界面，2校准值界面
idata unsigned int DAC_Data=100;//DAC储存值
idata char Correct_Data=0;//校准值参数
idata char Correct_Set=0;//设置校准值
idata unsigned int DAC_Mode_1=100;//手动控制的DAC数字量
idata float DAC_Mode_0=0;//温度控制的DAC数字量
idata unsigned int Count_1500ms=0;//1.5s计时
idata unsigned int Count_LED2=0;//LED2计时2s
idata unsigned int Count_LED3=0;//LED3计时2s
idata unsigned int Count_LED4=0;//LED4计时3s
idata unsigned char Count_200ms=0;//0.2s计时


//题目要求的标志位--------------------------
idata unsigned char DAC_Control=0;//0温度控制，1手动控制
idata unsigned char Correct_Flag=0;//0是正参数，1是负参数
idata unsigned char Lock=0;//继电器解锁标志位，0解锁，1上锁
idata unsigned char Key13=0;//按键13按下计时标志位
idata unsigned char Alarm_Flag=0;//温度突变报警标志位
idata unsigned char Rise_Flag = 0;//温度上升标志
idata unsigned char Drop_Flag = 0;//温度下降标志
idata unsigned char Led4_Flag=0;//LED4闪烁标志位
idata unsigned char L4_Blinking=0;//L4闪烁进行中标志 

/**
 * @brief LED处理函数
 * 设置LED的状态，这里是固定的交替亮灭模式。
 */
void Led_Proc()
{
	ucLed[0]=!DAC_Control;
	if(Rise_Flag == 1)
	{
		ucLed[1] = 1;  // 点亮L2
		if(Count_LED2 >= 2000)
		{
			ucLed[1] = 0;      // 2秒后熄灭
			Rise_Flag = 0;     // 清除标志
		}
	}
	else
		ucLed[1] = 0;
	if(Drop_Flag == 1)
	{
		ucLed[2] = 1;  // 点亮L3
		if(Count_LED3 >= 2000)
		{
			ucLed[2] = 0;      // 2秒后熄灭
			Drop_Flag = 0;     // 清除标志
		}
	}
	else
		ucLed[2] = 0;
	ucLed[3]=Led4_Flag;
	ucLed[7]=!Lock;
	Led_Disp(ucLed); 
}

/**
 * @brief 按键处理函数
 * 检测按键的按下和抬起事件，并根据按键执行相应操作。
 */
void Key_Proc()
{
	Key_Val = Key_Read(); // 读取当前按key的状态
	// 使用异或运算检测按键状态变化，& Key_Val 检测下降沿（按下）
	Key_Down = Key_Val & (Key_Val ^ Key_Old);
	// 使用异或运算检测按键状态变化，& ~Key_Val 检测上降沿（抬起）
	Key_Up = ~Key_Val & (Key_Val ^ Key_Old);
	Key_Old = Key_Val; // 更新旧的按键值

	switch(Key_Down)
	{
		case 12://切换界面
			if(Seg_Mode==1)
				Correct_Set=Correct_Data;
			Seg_Mode=(Seg_Mode+1)%3;
			if(Seg_Mode==0)
				Correct_Data=Correct_Set;
			Led_Proc();
		break;
		case 16://加按键
			switch(Seg_Mode)
			{
				case 0://温度
					if(Lock==0)
						Relay(1);
				break;
				
				case 1://DAC
					DAC_Mode_1+=5;
					if(DAC_Mode_1>255)
						DAC_Mode_1=255;
				break;
				
				case 2://校准值
					Correct_Set++;
					if(Correct_Set==10)
						Correct_Set=9;
				break;
			}
			Led_Proc();
		break;
		case 17://减按键
			switch(Seg_Mode)
			{
				case 0://温度
					if(Lock==0)
						Relay(0);
				break;
				
				case 1://DAC
					DAC_Mode_1-=5;
					if(DAC_Mode_1>255)
						DAC_Mode_1=0;
				break;
				
				case 2://校准值
					Correct_Set--;
					if(Correct_Set==-10)
						Correct_Set=-9;
				break;
			}
			Led_Proc();
		break;
		
	}
	
	if(Key_Down==13)
	{
		Key13=1;
		Led_Proc();
	}
	if(Key_Up==13)
	{
		if(Count_1500ms<1500)
		{
			DAC_Control^=1;
			Key13=Count_1500ms=0;
		}
		else
		{
			Lock^=1;
			Key13=Count_1500ms=0;
		}
		Led_Proc();
	}
	
}

/**
 * @brief 数码管显示处理函数
 * 根据当前的 Seg_Show_Mode，准备要显示在数码管上的数据。
 */
void Seg_Proc()
{
	idata unsigned char j=5;
	
	if(Correct_Set>=0)
		Correct_Flag=0;
	else
		Correct_Flag=1;
		
	switch(Seg_Mode)
	{
		case 0:
			Seg_Buf[0]=11;
			Seg_Buf[1]=10;
			Seg_Buf[2]=10;
			Seg_Buf[3]=10;
			Seg_Buf[4]=10;
			Seg_Buf[5]=10;
			Seg_Buf[6]=Correct_10x/100%10;
			Seg_Buf[7]=Correct_10x/10%10;
		break;
		
		case 1:
			Seg_Buf[0]=12;
			Seg_Buf[1]=10;
			Seg_Buf[2]=10;
			Seg_Buf[3]=10;
			Seg_Buf[4]=10;
			Seg_Buf[5]=DAC_Mode_1/100%10;
			Seg_Buf[6]=DAC_Mode_1/10%10;
			Seg_Buf[7]=DAC_Mode_1%10;
			while(Seg_Buf[j]==0)
			{
				Seg_Buf[j]=10;
				j++;
				if(j==7) break;
			}
		break;
		
		case 2:
			Seg_Buf[0]=13;
			Seg_Buf[1]=10;
			Seg_Buf[2]=10;
			Seg_Buf[3]=10;
			Seg_Buf[4]=10;
			Seg_Buf[5]=10;
			Seg_Buf[6]=(Correct_Flag==0)?10:14;
			Seg_Buf[7]=(Correct_Set<0)?(-Correct_Set):Correct_Set;
		break;
	}
}


/**
 * @brief 获取DS18B20温度
 */
void Get_Temperature()
{
	// 调用单总线驱动函数读取温度，并放大10倍
	Temperature_10xOld = Temperature_10x;     //先保存旧值
    Temperature_10x = rd_temperature() * 10;  //再读取新值
	Correct_10x=Temperature_10x+Correct_Data*10;
	
	 //采样稳定控制
    Temp_Count++;
    if(Temp_Count == 3)
		Temp_Count = 2;
	//只有采样稳定后才判断温度变化
	if(Temp_Count == 2)
    {
		if(Temperature_10x > Temperature_10xOld)
		{
			Rise_Flag = 1;
			Count_LED2 = 0;
		}
		if(Temperature_10x < Temperature_10xOld)
		{
			Drop_Flag = 1;
			Count_LED3 = 0;
		}
		if(((Temperature_10x-Temperature_10xOld)>=10)||((Temperature_10xOld-Temperature_10x)>=10))
		{
			Alarm_Flag=1;
			if(L4_Blinking==0)
			{
				L4_Blinking=1;
				Count_LED4=0;
				Count_200ms=0;
				Led4_Flag=1;
			}
		}
		else
			Alarm_Flag=0;
	}
}

/**
 * @brief ADC/DAC处理函数
 * 读取光敏电阻和电位器的AD值，并设置DAC输出。
 */
void AD_DA()
{
	if(DAC_Control==0)
	{
		idata signed char temp_celsius = (signed char)(Correct_10x / 10);
		if(temp_celsius<=10)
			DAC_Mode_0=(102);
		else if(temp_celsius>=40)
			DAC_Mode_0=(255);
		else
		{
			DAC_Mode_0=(5.1 * (temp_celsius - 10) + 102);
		}
		DAC_Data=(unsigned int)DAC_Mode_0;	
	}
	else
		DAC_Data=DAC_Mode_1;
	Da_Write(DAC_Data);
}



/**
 * @brief 定时器1初始化函数
 * 配置为1ms中断的定时器，作为系统的心跳。
 */
void Timer1_Init(void) // 1毫秒@12.000MHz
{
	AUXR &= 0xBF; // 定时器时钟12T模式
	TMOD &= 0x0F; // 设置定时器模式 (清空T1的设置)
	TL1 = 0x18;	  // 设置定时初始值 (65536-1000) -> FC18H
	TH1 = 0xFC;	  // 设置定时初始值
	TF1 = 0;	  // 清除TF1溢出标志
	TR1 = 1;	  // 定时器1开始计时
	ET1 = 1;	  // 使能定时器1中断
	EA = 1;		  // 开启总中断
}

/**
 * @brief 定时器1中断服务函数
 * 每1ms执行一次，处理需要定时执行的任务。
 */
void Timer1_Isr(void) interrupt 3
{
	uwTick++; // 系统滴答计时器加1

	// 数码管动态扫描
	Seg_Pos = (++Seg_Pos) % 8; // 切换到下一位数码管
	// 判断显示缓冲区的值是否需要显示小数点
	// (通过给数字加上一个大偏移量','作为标志)
	if (Seg_Buf[Seg_Pos] > 20)
		// 显示数字，并点亮小数点
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1);
	else
		// 显示数字，不点亮小数点
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);

	if(Key13==1)
	{
		Count_1500ms++;
		if(Count_1500ms>=1500)
			Count_1500ms=1500;
	}
	
	 // L2、L3计时器持续递增(带上限)
	if(Count_LED2 < 3000) Count_LED2++;
	if(Count_LED3 < 3000) Count_LED3++;
	
	if(L4_Blinking==1)
	{
		Count_LED4++;
		Count_200ms++;
		if(Count_200ms==200)
		{
			Count_200ms=0;
			Led4_Flag^=1;
		}
		if(Count_LED4==3000)
		{
			Count_LED4=0;
			L4_Blinking=0;
			Count_200ms=0;
			Led4_Flag=0;
		}
	}
}

// 简单任务调度器结构体定义
typedef struct
{
	void (*task_func)(void);   // 任务函数指针
	unsigned long int rate_ms; // 任务执行周期（毫秒）
	unsigned long int last_ms; // 上次执行的时间戳
} task_t;

// 任务列表
idata task_t Scheduler_Task[] = {
	{Led_Proc, 10, 0},		   // LED任务，每10ms
	{Key_Proc, 10, 0},		   // 按键任务，每10ms
	{Seg_Proc, 20, 0},		   // 数码管任务，每20ms
	{Get_Temperature, 300, 0}, // 获取温度任务，每300ms
	{AD_DA, 150, 0}		   // AD/DA任务，每150ms
	
};

idata unsigned char task_num; // 任务数量

/**
 * @brief 调度器初始化
 * 计算任务列表中的任务数量。
 */
void Scheduler_Init()
{
	task_num = sizeof(Scheduler_Task) / sizeof(task_t);
}

/**
 * @brief 调度器运行函数
 * 循环检查每个任务是否到达执行时间。
 */
void Scheduler_Run()
{
	unsigned char i;
	for (i = 0; i < task_num; i++)
	{
		unsigned long int now_time = uwTick; // 获取当前时间
		// 判断是否到达任务执行时间 (当前时间 >= 上次执行时间 + 周期)
		if (now_time >= (Scheduler_Task[i].rate_ms + Scheduler_Task[i].last_ms))
		{
			Scheduler_Task[i].last_ms = now_time; // 更新上次执行时间
			Scheduler_Task[i].task_func();		  // 执行任务函数
		}
	}
}

/**
 * @brief 主函数
 */
void main()
{
	System_Init();	// 系统底层初始化
	while(rd_temperature() == 85);
	// 各模块初始化
	Scheduler_Init(); // 任务调度器初始化
	Timer1_Init();	  // 定时器1（系统心跳）初始化

	// 主循环
	while (1)
	{
		Scheduler_Run(); // 循环执行任务调度器
	}
}