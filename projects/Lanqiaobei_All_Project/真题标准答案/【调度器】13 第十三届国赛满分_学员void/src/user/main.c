// 包含 STC15F2K60S2 单片机的头文件，提供了寄存器定义等
#include <STC15F2K60S2.H>

// 包含自定义的初始化、外设驱动和功能模块的头文件
#include "init.h" // 系统初始化
#include "led.h"  // LED 模块
#include "seg.h"  // 数码管模块
#include "key.h"  // 按键模块

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



// 1秒的计时器，用于频率测量
idata unsigned int Time_1s;

//题目要求的变量-------------------------------------
idata unsigned char Seg_Mode=0;//0频率界面，1湿度界面，2测距界面，3参数界面
idata unsigned char Set_Data=0;//0频率参数，1湿度参数，2距离参数
idata unsigned int Freq=0;// 测量的频率值
idata unsigned char Humidity=0;//湿度值
idata unsigned char Distance=0;// 存储超声波测量的距离
idata unsigned int Para_Freq=9000;//默认频率参数 9.0kHz
idata unsigned char Para_Humi=40;//默认湿度参数 40%
idata unsigned char Para_Dist=60;//默认距离参数 0.6m
idata unsigned int Count_1000ms=0;//1s计时变量
idata float AD_3_Data=0;//RB2电压值
idata unsigned char Temp=0;//DAC折线计算
idata unsigned char Relay_Old = 0; // 继电器上一次状态，用于边沿检测
idata unsigned char Count_Relay=0;//统计继电器开关次数
idata unsigned char Count_Relay_Pre = 0;//旧次数
idata unsigned char Led1_100ms=0;//Led1的0.1s计时
idata unsigned char Led2_100ms=0;//Led2的0.1s计时
idata unsigned char Led3_100ms=0;//Led3的0.1s计时
idata unsigned char EEPROM_Lock = 13;//EEPROM加锁机制
idata unsigned char EEPROM_Temp;
idata unsigned char PWM_Count = 0;   // PWM计数器 0~9
idata unsigned char	Motor_Start = 0;           // 0=20%占空比, 1=80%占空比

//题目要求的标志位--------------------------------------
idata unsigned char Unit_Dist=0;//距离显示单位切换，0CM,1M
idata unsigned char Unit_Freq=0;//频率显示单位切换，0Hz,1kHz
idata unsigned char Key7_Flag=0;//按键7长按计时标志位
idata unsigned char Led1_Freq=0;//Led1闪烁计时标志位
idata unsigned char Led2_Humi=0;//Led2闪烁计时标志位
idata unsigned char Led3_Dist=0;//Led3闪烁计时标志位
idata unsigned char Led1_Flag=0;//Led1闪烁标志位
idata unsigned char Led2_Flag=0;//Led2闪烁标志位
idata unsigned char Led3_Flag=0;//Led3闪烁标志位



/**
 * @brief LED处理函数
 * 设置LED的状态，这里是固定的交替亮灭模式。
 */
void Led_Proc()
{
	//继电器
	if((Distance > Para_Dist) && (Relay_Old == 0))
	{
		Relay(1);
		Relay_Old = 1;
		Count_Relay++;
	}
	else if((Relay_Old == 1) && (Distance <= Para_Dist))  
	{
		Relay(0);
		Relay_Old = 0;
		Count_Relay++;
	}
	
	//L1
	if(Seg_Mode==0)
		ucLed[0]=1;
	else if(Seg_Mode==3)
	{
		if(Set_Data==0)
		{
			Led1_Freq=1;
			ucLed[0]=Led1_Flag;
		}
		else
		{
			Led1_Freq=0;
			ucLed[0]=0;
		}
	}
	else
	{
		Led1_Freq=0;
		ucLed[0]=0;
	}
	
	//L2
	if(Seg_Mode==1)
		ucLed[1]=1;
	else if(Seg_Mode==3)
	{
		if(Set_Data==1)
		{
			Led2_Humi=1;
			ucLed[1]=Led2_Flag;
		}
		else
		{
			Led2_Humi=0;
			ucLed[1]=0;
		}
	}
	else
	{
		Led2_Humi=0;
		ucLed[1]=0;
	}
	
	//L3
	if(Seg_Mode==2)
		ucLed[2]=1;
	else if(Seg_Mode==3)
	{
		if(Set_Data==2)
		{
			Led3_Dist=1;
			ucLed[2]=Led3_Flag;
		}
		else
		{
			Led3_Dist=0;
			ucLed[2]=0;
		}
	}
	else
	{
		Led3_Dist=0;
		ucLed[2]=0;
	}
	
	//L4/L5/L6 
	ucLed[3]=(Freq>Para_Freq)?1:0;
	ucLed[4]=(Humidity>Para_Humi)?1:0;
	ucLed[5]=(Distance>Para_Dist)?1:0;
	
	//电机 脉冲输出
	Motor_Start = (Freq > Para_Freq) ? 1 : 0;
	
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
		case 4://界面切换
			Seg_Mode=(Seg_Mode+1)%4;
			if(Seg_Mode==3)
				Set_Data=0;
			Led_Proc();
		break;
		
		case 5://参数切换
			if(Seg_Mode==3)
				Set_Data=(Set_Data+1)%3;
			Led_Proc();
		break;
		
		case 6:
			if(Seg_Mode==3)
			{
				switch(Set_Data)
				{
					case 0://频率参数
						Para_Freq+=500;
						if(Para_Freq==12500)
							Para_Freq=1000;
					break;
					
					case 1://湿度参数
						Para_Humi+=10;
						if(Para_Humi==70)
							Para_Humi=10;
					break;
					
					case 2://距离参数
						Para_Dist+=10;
						if(Para_Dist==130)
							Para_Dist=10;
					break;
				}
			}
			else if(Seg_Mode==2)
				Unit_Dist^=1;
			Led_Proc();
		break;
	}
	
	if(Key_Down==7)//按键7按下
	{
		Key7_Flag=1;
		Led_Proc();
	}
	
	if(Key_Up==7)//按键7抬起
	{
		if(Count_1000ms<=1000)//短按
		{
			if(Seg_Mode==3)
			{
				switch(Set_Data)
				{
					case 0://频率参数
						Para_Freq-=500;
						if(Para_Freq==500)
							Para_Freq=12000;
					break;
					
					case 1://湿度参数
						Para_Humi-=10;
						if(Para_Humi==0)
							Para_Humi=60;
					break;
					
					case 2://距离参数
						Para_Dist-=10;
						if(Para_Dist==0)
							Para_Dist=120;
					break;
				}
			}
			else if(Seg_Mode==0)
				Unit_Freq^=1;
			Key7_Flag=0;
			Count_1000ms=0;
		}
		else//长按
		{
			if(Seg_Mode == 1) // 仅在湿度页面
				Count_Relay=0;
			Key7_Flag=0;
			Count_1000ms=0;
		}
	}
	
}

/**
 * @brief 数码管显示处理函数
 * 根据当前的 Seg_Show_Mode，准备要显示在数码管上的数据。
 */
void Seg_Proc()
{
	idata unsigned char j;
	switch(Seg_Mode)
	{
		case 0://频率界面
			Seg_Buf[0]=11;
			Seg_Buf[1]=10;
			switch(Unit_Freq)
			{
				case 0://Hz
					Seg_Buf[2]=Freq/100000%10;
					Seg_Buf[3]=Freq/10000%10;
					Seg_Buf[4]=Freq/1000%10;
					Seg_Buf[5]=Freq/100%10;
					Seg_Buf[6]=Freq/10%10;
					Seg_Buf[7]=Freq%10;
				break;
				
				case 1://kHz
					Seg_Buf[2]=Freq/10000000%10;
					Seg_Buf[3]=Freq/1000000%10;
					Seg_Buf[4]=Freq/100000%10;
					Seg_Buf[5]=Freq/10000%10;
					Seg_Buf[6]=Freq/1000%10+',';
					Seg_Buf[7]=Freq/100%10;
				break;
			}
			j=2;
			while(Seg_Buf[j]==0)
			{
				Seg_Buf[j]=10;
				j++;
				if(j==7) break;
			}
		break;
		
		case 1://湿度界面
			Seg_Buf[0]=12;
			Seg_Buf[1]=10;
			Seg_Buf[2]=10;
			Seg_Buf[3]=10;
			Seg_Buf[4]=10;
			Seg_Buf[5]=10;
			Seg_Buf[6]=Humidity/10%10;
			Seg_Buf[7]=Humidity%10;
		break;
		
		case 2://测距界面
			Seg_Buf[0]=13;
			Seg_Buf[1]=10;
			Seg_Buf[2]=10;
			Seg_Buf[3]=10;
			Seg_Buf[4]=10;
			switch(Unit_Dist)
			{
				case 0://CM
					Seg_Buf[5]=Distance/100%10;
					Seg_Buf[6]=Distance/10%10;
					Seg_Buf[7]=Distance%10;
				break;
				
				case 1://M
					Seg_Buf[5]=Distance/100%10+',';
					Seg_Buf[6]=Distance/10%10;
					Seg_Buf[7]=Distance%10;
				break;
			}
			j=5;
			while(Seg_Buf[j]==0)
			{
				Seg_Buf[j]=10;
				j++;
				if(j==7) break;
			}
		break;
		
		case 3://参数界面
			switch(Set_Data)
			{
				case 0://频率参数
					Seg_Buf[0]=14;
					Seg_Buf[1]=1;
					Seg_Buf[2]=10;
					Seg_Buf[3]=10;
					Seg_Buf[4]=10;
					Seg_Buf[5]=Para_Freq/10000%10;
					Seg_Buf[6]=Para_Freq/1000%10+',';
					Seg_Buf[7]=Para_Freq/100%10;
					j=5;
					while(Seg_Buf[j]==0)
					{
						Seg_Buf[j]=10;
						j++;
						if(j==7) break;
					}
				break;
				
				case 1://湿度参数
					Seg_Buf[0]=14;
					Seg_Buf[1]=2;
					Seg_Buf[2]=10;
					Seg_Buf[3]=10;
					Seg_Buf[4]=10;
					Seg_Buf[5]=10;
					Seg_Buf[6]=Para_Humi/10%10;
					Seg_Buf[7]=Para_Humi%10;
				break;
				
				case 2://距离参数
					Seg_Buf[0]=14;
					Seg_Buf[1]=3;
					Seg_Buf[2]=10;
					Seg_Buf[3]=10;
					Seg_Buf[4]=10;
					Seg_Buf[5]=10;
					Seg_Buf[6]=Para_Dist/100%10+',';
					Seg_Buf[7]=Para_Dist/10%10;
					break;
			}
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
	AD_3_Data=(float)Ad_Read(0x43)/51.0f;
	
	//湿度值
	if(AD_3_Data>=5)
		Humidity=100;
	else
		Humidity=(unsigned char)(20*AD_3_Data);
	
	//DAC输出
	if(Humidity<=Para_Humi)
		Da_Write(51);
	else if(Humidity>=80)
		Da_Write(255);
	else
	{
		Temp=(unsigned char)((1.0+4.0*(Humidity-Para_Humi)/(float)(80-Para_Humi))*51);
		Da_Write(Temp);
	}
}

/**
 * @brief 获取超声波测量的距离
 */
void Get_Distance()
{
	Distance = Ut_Wave_Data(); // 调用超声波驱动函数
}

void EEPROM_Proc()
{
	if(Count_Relay != Count_Relay_Pre)
	{
		Count_Relay_Pre = Count_Relay;//保存旧值
		EEPROM_Read(&EEPROM_Temp, 22, 1);      // 读校验位
		if(EEPROM_Temp == EEPROM_Lock)          // 校验通过
			EEPROM_Write(&Count_Relay, 0, 1);
	}
}

/**
 * @brief 定时器0初始化函数
 * 配置为计数器模式，用于测量外部脉冲频率。
 */
void Timer0_Init(void) // 1毫秒@12.000MHz
{
	AUXR &= 0x7F; // 定时器时钟12T模式
	TMOD &= 0xF0; // 清除T0的模式位
	TMOD |= 0x05; // 设置为计数器模式(M1=0, M0=1, C/T=1)，外部引脚T0(P3.4)的下降沿计数
	TL0 = 0x00;	  // 设置计数器初始值
	TH0 = 0x00;	  // 设置计数器初始值
	TF0 = 0;	  // 清除TF0溢出标志
	TR0 = 1;	  // 定时器0开始工作
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

void Timer2_Init(void)    // 100微秒@12.000MHz
{
    AUXR &= 0xFB;    // 定时器时钟12T模式
    T2L = 0x9C;      // 设置定时初值
    T2H = 0xFF;      // 设置定时初值
    AUXR |= 0x10;    // 定时器2开始计时
    IE2 |= 0x04;     // 使能定时器2中断
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

	// 频率测量，每1000ms (1s) 计算一次
	if (++Time_1s == 1000)
	{
		Time_1s = 0; // 计时器清零
		// 读取T0的计数值，即为过去1秒内的脉冲数(Hz)
		Freq = (TH0 << 8) | TL0;
		TH0 = TL0 = 0; // 清空T0计数器，开始下一个周期的测量
	}

	if(Key7_Flag==1)
	{
		Count_1000ms++;
		if(Count_1000ms>=1001)
		{
			Count_1000ms=1001;
		}
	}
	
	//Led1闪烁计时
	if(Led1_Freq==1)
	{
		Led1_100ms++;
		if(Led1_100ms==100)
		{
			Led1_100ms=0;
			Led1_Flag^=1;
		}
	}
	else
	{
		Led1_100ms=0;
		Led1_Flag=0;
	}
	
	//Led2闪烁计时
	if(Led2_Humi==1)
	{
		Led2_100ms++;
		if(Led2_100ms==100)
		{
			Led2_100ms=0;
			Led2_Flag^=1;
		}
	}
	else
	{
		Led2_100ms=0;
		Led2_Flag=0;
	}
	
	//Led3闪烁计时
	if(Led3_Dist==1)
	{
		Led3_100ms++;
		if(Led3_100ms==100)
		{
			Led3_100ms=0;
			Led3_Flag^=1;
		}
	}
	else
	{
		Led3_100ms=0;
		Led3_Flag=0;
	}
}

void Timer2_Isr(void) interrupt 12
{
    if(Motor_Start == 1)       // 频率 > 参数 → 80%占空比
    {
        if(PWM_Count < 8)
            Motor(1);          // 0~7: 高电平（8份）
        else
            Motor(0);          // 8~9: 低电平（2份）
    }
    else                       // 频率 <= 参数 → 20%占空比
    {
        if(PWM_Count < 8)
            Motor(0);          // 0~7: 低电平（8份）
        else
            Motor(1);          // 8~9: 高电平（2份）
    }
	PWM_Count++;
    if(PWM_Count == 10)
        PWM_Count = 0;         // 10份一个周期，回到0
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
	{Get_Distance, 120, 0},	   // 获取距离任务，每120ms
	{EEPROM_Proc, 500, 0}    // 加上这行！500ms一次足够了
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
	EEPROM_Read(&EEPROM_Temp, 22, 1);      // 读校验位
    if(EEPROM_Temp == EEPROM_Lock)          // 校验通过
    {
		EEPROM_Read(&Count_Relay, 0, 1);   // 恢复继电器计数
		Count_Relay_Pre = Count_Relay;      // 同步pre值，防止上电立刻写入
    }
    else  // 首次上电
    {
		Count_Relay = 0;
		EEPROM_Write(&EEPROM_Lock, 22, 1); // 写校验位
		EEPROM_Temp = EEPROM_Lock;  // ← 同步内存变量！
    }
	// 各模块初始化
	Timer0_Init();	  // 定时器0（频率计数）初始化
	Scheduler_Init(); // 任务调度器初始化
	Timer1_Init();	  // 定时器1（系统心跳）初始化
	Timer2_Init();    //定时器2，用于电机脉冲输出
	// 主循环
	while (1)
	{
		Scheduler_Run(); // 循环执行任务调度器
	}
}