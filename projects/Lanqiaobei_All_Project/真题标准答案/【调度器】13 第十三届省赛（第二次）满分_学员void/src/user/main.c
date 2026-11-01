// 包含 STC15F2K60S2 单片机的头文件，提供了寄存器定义等
#include <STC15F2K60S2.H>

// 包含自定义的初始化、外设驱动和功能模块的头文件
#include "init.h" // 系统初始化
#include "led.h"  // LED 模块
#include "seg.h"  // 数码管模块
#include "key.h"  // 按键模块

#include "ds1302.h"		// DS1302 实时时钟模块
#include "onewire.h"	// 单总线协议，用于温度传感器
#include "iic.h"		// I2C 协议，用于 ADC/DAC 和 EEPROM
#include "ultrasound.h" // 超声波模块
#include "uart.h"		// 串口通信模块

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

// `pdata` 存储在外部 RAM 的分页区域
// RTC实时时钟数据数组 [时, 分, 秒]
pdata unsigned char ucRtc[3] = {11, 12, 13};

// `idata` 存储在内部高速 RAM 中
// 存储放大10倍的温度值，以处理一位小数
idata unsigned int Temperature_10x;

// AD转换通道1和通道3的数据，同样乘以100以处理两位小数
idata unsigned int AD_3_Data_100x;

// `idata` 存储在内部高速 RAM 中
// 存储超声波测量的距离
idata unsigned char Distance;

//题目要求变量----------------------
idata unsigned char Seg_Mode=0;//0电压界面，1测距界面，2参数界面
pdata unsigned char Parem[2]={45,5};//上下限参数*10
pdata unsigned char Parem_Set[2]={45,5};//上下限参数设置*10
idata unsigned char P_Index=0;//参数索引
idata unsigned char Detect_Flag=0;//超声波测距标志位，0停止测距，1启动连续测距
idata unsigned char Time_100ms=0;//0.1s计时
idata unsigned char Led8_Flag=0;//Led8闪烁标志位
idata float temp=0;//折线计算值临时存储
/**
 * @brief LED处理函数
 * 设置LED的状态，这里是固定的交替亮灭模式。
 */
void Led_Proc()
{
	switch(Seg_Mode)
	{
		case 0:
			ucLed[0]=1;
			ucLed[1]=0;
			ucLed[2]=0;
		break;
		
		case 1:
			ucLed[1]=1;
			ucLed[0]=0;
			ucLed[2]=0;
		break;
		
		case 2:
			ucLed[2]=1;
			ucLed[0]=0;
			ucLed[1]=0;
		break;
	}
	ucLed[7]=Led8_Flag;
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
		case 4:
			if(Seg_Mode==1)
			{
				P_Index=0;
				Parem_Set[0]=Parem[0];
				Parem_Set[1]=Parem[1];
			}
			if(Seg_Mode==2)
			{
				if(Parem_Set[0]>Parem_Set[1])
				{
					Parem[0]=Parem_Set[0];
					Parem[1]=Parem_Set[1];
				}
				else
				{
					Parem[0]=45;
					Parem[1]=5;
				}
			}
			Seg_Mode=(Seg_Mode+1)%3;
			Led_Proc();
		break;
		
		
		case 5:
			if(Seg_Mode==2)
				P_Index^=1;
			Led_Proc();
		break;
			
		case 6:
			if(Seg_Mode==2)
			{
				Parem_Set[P_Index]+=5;
				Parem_Set[P_Index]=(Parem_Set[P_Index]==55)?5:Parem_Set[P_Index];
			}
			Led_Proc();
		break;
		
		case 7:
			if(Seg_Mode==2)
			{
				Parem_Set[P_Index]-=5;
				Parem_Set[P_Index]=(Parem_Set[P_Index]==0)?50:Parem_Set[P_Index];
			}
			Led_Proc();
		break;
	}
}

/**
 * @brief 数码管显示处理函数
 * 根据当前的 Seg_Show_Mode，准备要显示在数码管上的数据。
 */
void Seg_Proc()
{
	idata unsigned char j=5;
	switch(Seg_Mode)
	{
		case 0://电压界面
			Seg_Buf[0]=11;
			Seg_Buf[1]=10;
			Seg_Buf[2]=10;
			Seg_Buf[3]=10;
			Seg_Buf[4]=10;
			Seg_Buf[5]=AD_3_Data_100x/100%10+',';
			Seg_Buf[6]=AD_3_Data_100x/10%10;
			Seg_Buf[7]=AD_3_Data_100x%10;
		break;
		
		case 1://测距界面
			Seg_Buf[0]=13;
			Seg_Buf[1]=10;
			Seg_Buf[2]=10;
			Seg_Buf[3]=10;
			Seg_Buf[4]=10;
			switch(Detect_Flag)
			{
				case 0:
					Seg_Buf[5]=14;
					Seg_Buf[6]=14;
					Seg_Buf[7]=14;
				break;
				
				case 1:
					Seg_Buf[5]=Distance/100%10;
					Seg_Buf[6]=Distance/10%10;
					Seg_Buf[7]=Distance%10;
				    while(Seg_Buf[j]==0)
					{
						Seg_Buf[j]=10;
						j++;
						if(j==7) break;
					}
				break;
			}
		break;
		
		case 2://参数界面
			Seg_Buf[0]=12;
			Seg_Buf[1]=10;
			Seg_Buf[2]=10;
			Seg_Buf[3]=Parem_Set[0]/10%10+',';
			Seg_Buf[4]=Parem_Set[0]%10;
			Seg_Buf[5]=10;
			Seg_Buf[6]=Parem_Set[1]/10%10+',';
			Seg_Buf[7]=Parem_Set[1]%10;
		break;
	}
}


/**
 * @brief ADC/DAC处理函数
 * 读取光敏电阻和电位器的AD值，并设置DAC输出。
 */
void AD_DA()
{
	Ad_Read(0x43);
	AD_3_Data_100x = (float)Ad_Read(0x43) / 51.0f * 100;
	
	temp=(100+(Distance-20)*(400/60.0f))/100.0f;
	
	if(Detect_Flag==1)
	{
		if(Distance<20)
			Da_Write(1*51);
		else if(Distance>80)
			Da_Write(5*51);
		else 
			Da_Write(temp*51);
	}
	else
		Da_Write(0);
}

/**
 * @brief 获取超声波测量的距离
 */
void Get_Distance()
{
	Distance = Ut_Wave_Data(); // 调用超声波驱动函数
	if((AD_3_Data_100x<(Parem[0]*10))&&(AD_3_Data_100x>(Parem[1]*10)))
		Detect_Flag=1;
	else
		Detect_Flag=0;
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

	if(Detect_Flag==1)
	{
		Time_100ms++;
		if(Time_100ms==100)
		{
			Time_100ms=0;
			Led8_Flag^=1;
		}
	}
	else  // 添加这个else分支
	{
		Time_100ms=0;
		Led8_Flag=0;  // 停止时熄灭L8
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
	{Led_Proc, 1, 0},		   // LED任务，每1ms
	{Key_Proc, 10, 0},		   // 按键任务，每10ms
	{Seg_Proc, 20, 0},		   // 数码管任务，每20ms
	{AD_DA, 150, 0},		   // AD/DA任务，每150ms
	{Get_Distance, 120, 0}	   // 获取距离任务，每120ms
	
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
	
	// 各模块初始化
	Scheduler_Init(); // 任务调度器初始化
	Timer1_Init();	  // 定时器1（系统心跳）初始化

	// 主循环
	while (1)
	{
		Scheduler_Run(); // 循环执行任务调度器
	}
}