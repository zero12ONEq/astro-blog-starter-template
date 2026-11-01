#include <STC15F2K60S2.H>
#include "key.h"
#include "led.h"
#include "seg.h"
#include "iic.h"
#include "init.h"
#include "ultrasound.h"
#include "uart.h"
#include "stdio.h"
#include "string.h"
#include "math.h"

/*变量*/
idata unsigned long int uwTick = 0;	 // 系统滴答计时器
idata unsigned int Time_1s_Freq = 0; //
// 按键
idata unsigned char Key_Val, Key_Old, Key_Down, Key_Up;
// Led
pdata unsigned char ucLed[8] = {0, 0, 0, 0, 0, 0, 0, 0};
// 数码管
pdata unsigned char Seg_Buf[8] = {10, 10, 10, 10, 10, 10, 10, 10};
idata unsigned char Seg_Pos = 0;
// 串口
pdata unsigned char Uart_Rx_Buf[9] = 0; 
idata unsigned char Uart_Rx_Index = 0;	
idata unsigned char Uart_Rx_Tick = 0;	// 超时解析10ms计时器
idata bit Uart_Rx_Flag = 0;				
//参数
idata unsigned int Freq_Value = 0; //频率测量值

idata unsigned char Work_Mode = 0;			// 工作状态 0 空闲 1 运行 2 等待
idata unsigned int Speed_Value_10x = 0;		// 速度的十倍 频率计算得到
idata unsigned char Obstacle_Distance = 0;	// 障碍物的距离
pdata unsigned int Current_Location[2] = 0; // 当前位置
pdata unsigned int Target_Location[2] = 0;	// 目标位置

idata unsigned char R_Value_10x = 10;	  // R参数的10倍
idata char B_Value = 0;					  // B参数
idata unsigned char R_Value_Ctrl_10x = 0; // R参数控制值的10倍
idata char B_Value_Ctrl = 0;			  //B参数控制值
idata bit Dark_Work = 0;				  //是否处于黑暗状态 0 光照 1 黑暗

idata unsigned char Seg_Show_Mode = 0; // 显示界面 0 坐标 1 速度 2 参数
idata bit Para_Setting_Mode = 0;	   // 参数界面设置选择 0 R 1 B

idata unsigned char Time_100Ms_Wait = 0; //等待状态L1闪烁100计时
idata bit Led_Light_Wait_Flag = 0;		 // 等待状态闪烁标志位
idata unsigned int Time_3s = 0;			 // 到达的倒计时3s点亮
idata bit Led_Light_Reach_Flag = 0;		 // 到达后点亮标志

idata bit Reach_Flag = 0;				   //到达目的地的标志
pdata float Current_Location_Float[2] = 0; //当前位置浮点数
pdata float Target_Location_Float[2] = 0;  //目标位置浮点数

idata bit Get_Target_Flag = 0; //接受目的地坐标
idata bit First_Flag = 0;	   //第一次接收到坐标开始移动的标志
idata unsigned long int Last_Tick = 0;	  //最后一次运行的时间
idata unsigned long int Current_Tick = 0; // 当前时间
/*控制函数 */
void Ctrl_Proc()
{

	idata unsigned long int Dt_Ms = 0;		  //两次运行的时间差
	idata float Dt = 0;						  //时间差转换为s
	idata float Dx = 0, Dy = 0;				  // x和y方向的向量
	idata float Distance = 0;				  //当前位置离目的地的距离
	idata float Step = 0;					  //每次移动的距离
	idata float Move_x = 0, Move_y = 0;//每次移动的距离在x方向上的分量,y方向上的分量
	//在运行状态下，才会发生移动
	if (Work_Mode == 1)
	{

		Current_Tick = uwTick; //获取当前时间
		//第一次接收到目的地坐标，计算当前位置与目的地位置之间的位移
		if (First_Flag == 0)
		{
			Last_Tick = Current_Tick;
			First_Flag = 1;
			Current_Location_Float[0] = Current_Location[0];
			Current_Location_Float[1] = Current_Location[1];
			Target_Location_Float[0] = Target_Location[0];
			Target_Location_Float[1] = Target_Location[1];
			return;
		}
		Dt_Ms = Current_Tick - Last_Tick; 
		Last_Tick=Current_Tick;
		Dt = (float)Dt_Ms / 1000.0f;	  
		Dx = Target_Location_Float[0] - Current_Location_Float[0]; 
		Dy = Target_Location_Float[1] - Current_Location_Float[1]; 
		Distance = sqrt(Dx * Dx + Dy * Dy);

		if (Distance > 0)
		{
			Step = ((float)Speed_Value_10x / 10.0f) * Dt;

			if (Distance <= Step)
			{
				Current_Location_Float[0] = Target_Location_Float[0];
				Current_Location_Float[1] = Target_Location_Float[1];
				Current_Location[0] = Target_Location[0];
				Current_Location[1] = Target_Location[1];
				Reach_Flag = 1;
				Work_Mode = 0;
			}
			else
			{
				Move_x = (Dx / Distance) * Step;
				Move_y = (Dy / Distance) * Step;
				Current_Location_Float[0] += Move_x;
				Current_Location_Float[1] += Move_y;
				Current_Location[0] = Current_Location_Float[0];
				Current_Location[1] = Current_Location_Float[1];
			}
		}
	}
}

void Key_Proc()
{
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Val ^ Key_Old);
	Key_Up = ~Key_Val & (Key_Val ^ Key_Old);
	Key_Old = Key_Val;
	switch (Work_Mode)
	{
	case 0:

		if (Key_Down == 4)
		{
			if (Get_Target_Flag)
			{
				Work_Mode = 1;
				First_Flag = 0;
				Get_Target_Flag = 0;
			}
		}
		if (Key_Down == 5)
		{
			Current_Location[0] = 0;
			Current_Location[1] = 0;
			Get_Target_Flag = 1;
		}
		break;
	case 1:

		if (Key_Down == 4)
			Work_Mode = 2;
		break;
	case 2:

		if (Key_Down == 4)
		{
			if (Obstacle_Distance >= 30)
			{
				Work_Mode = 1;
				First_Flag = 0;
			}
		}
		break;
	}
	switch (Seg_Show_Mode)
	{
	case 0:

		if (Key_Down == 8)
			Seg_Show_Mode = 1;
		break;
	case 1:

		if (Key_Down == 8)
		{
			Seg_Show_Mode = 2;
			B_Value_Ctrl = B_Value;
			R_Value_Ctrl_10x = R_Value_10x;
			Para_Setting_Mode = 0;
		}
		break;
	case 2:

		if (Key_Down == 8)
		{
			Seg_Show_Mode = 0;
			B_Value = B_Value_Ctrl;
			R_Value_10x = R_Value_Ctrl_10x;
		}
		if (Key_Down == 9)
			Para_Setting_Mode ^= 1;

		if (Para_Setting_Mode == 0)
		{
			if (Key_Down == 12)
			{
				if (R_Value_Ctrl_10x < 20)
					R_Value_Ctrl_10x++;
			}
			if (Key_Down == 13)
			{
				if (R_Value_Ctrl_10x > 10)
					R_Value_Ctrl_10x--;
			}
		}

		else
		{
			if (Key_Down == 12)
			{
				if (B_Value_Ctrl < 90)
					B_Value_Ctrl += 5;
			}
			if (Key_Down == 13)
			{
				if (B_Value_Ctrl > -90)
					B_Value_Ctrl -= 5;
			}
		}
		break;
	}
}


void Seg_Proc()
{
	unsigned char Temp_B_Value_Ctrl = -B_Value_Ctrl;
	switch (Seg_Show_Mode)
	{
	case 0:

		Seg_Buf[0] = 11; // L
		Seg_Buf[4] = 12; //-

		if (Work_Mode == 0)
		{
			Seg_Buf[1] = (Current_Location[0] / 100 == 0)
							 ? 10
							 : Current_Location[0] / 100;
			Seg_Buf[2] = ((Current_Location[0] / 10 % 10 == 0) && (Seg_Buf[1] == 10))
							 ? 10
							 : Current_Location[0] / 10 % 10;
			Seg_Buf[3] = Current_Location[0] % 10;
			Seg_Buf[5] = (Current_Location[1] / 100 == 0)
							 ? 10
							 : Current_Location[1] / 100;
			Seg_Buf[6] = ((Current_Location[1] / 10 % 10 == 0) && (Seg_Buf[5] == 10))
							 ? 10
							 : Current_Location[1] / 10 % 10;
			Seg_Buf[7] = Current_Location[1] % 10;
		}
		else
		{
			Seg_Buf[1] = (Target_Location[0] / 100 == 0)
							 ? 10
							 : Target_Location[0] / 100;
			Seg_Buf[2] = ((Target_Location[0] / 10 % 10 == 0) && (Seg_Buf[1] == 10))
							 ? 10
							 : Target_Location[0] / 10 % 10;
			Seg_Buf[3] = Target_Location[0] % 10;
			Seg_Buf[5] = (Target_Location[1] / 100 == 0)
							 ? 10
							 : Target_Location[1] / 100;
			Seg_Buf[6] = ((Target_Location[1] / 10 % 10 == 0) && (Seg_Buf[5] == 10))
							 ? 10
							 : Target_Location[1] / 10 % 10;
			Seg_Buf[7] = Target_Location[1] % 10;
		}
		break;
	case 1:

		Seg_Buf[0] = 13; // E
		Seg_Buf[2] = 10;
		switch (Work_Mode)
		{
		case 0:
	
			Seg_Buf[1] = 2;
			Seg_Buf[3] = 12; //-
			Seg_Buf[4] = 12; //-
			Seg_Buf[5] = 12; //-
			Seg_Buf[6] = 12; //-
			Seg_Buf[7] = 12; //-
			break;
		case 1:

			Seg_Buf[1] = 1;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = (Speed_Value_10x / 1000 % 10 == 0)
							 ? 10
							 : Speed_Value_10x / 1000 % 10;
			Seg_Buf[5] = ((Speed_Value_10x / 100 % 10 == 0) && (Seg_Buf[4] == 10))
							 ? 10
							 : Speed_Value_10x / 100 % 10;
			Seg_Buf[6] = Speed_Value_10x / 10 % 10 + ',';
			Seg_Buf[7] = Speed_Value_10x % 10;
			break;
		case 2:

			Seg_Buf[1] = 3;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = ((Obstacle_Distance / 100 == 0) && (Seg_Buf[4] == 10))
							 ? 10
							 : Obstacle_Distance / 100;
			Seg_Buf[6] = ((Obstacle_Distance / 10 % 10 == 0) && (Seg_Buf[5] == 10))
							 ? 10
							 : Obstacle_Distance / 10 % 10;
			Seg_Buf[7] = Obstacle_Distance % 10;
			break;
		}
		break;
	case 2:

		Seg_Buf[0] = 14; // P
		Seg_Buf[1] = 10;
		Seg_Buf[2] = R_Value_Ctrl_10x / 10 + ',';
		Seg_Buf[3] = R_Value_Ctrl_10x % 10;
		Seg_Buf[4] = 10;
		if (B_Value_Ctrl <= -10)
		{
			Seg_Buf[5] = 12; //-
			Seg_Buf[6] = Temp_B_Value_Ctrl / 10;
			Seg_Buf[7] = Temp_B_Value_Ctrl % 10;
		}
		else if (B_Value_Ctrl < 0)
		{
			Seg_Buf[5] = 10;
			Seg_Buf[6] = 12; //-
			Seg_Buf[7] = Temp_B_Value_Ctrl;
		}
		else if (B_Value_Ctrl < 10)
		{
			Seg_Buf[5] = 10;
			Seg_Buf[6] = 10;
			Seg_Buf[7] = B_Value_Ctrl;
		}
		else
		{
			Seg_Buf[5] = 10;
			Seg_Buf[6] = B_Value_Ctrl / 10;
			Seg_Buf[7] = B_Value_Ctrl % 10;
		}
		break;
	}
}


void Led_Proc()
{
	switch (Work_Mode)
	{
	case 0:
		ucLed[0] = 0;
		ucLed[1] = 0;
		Relay(0);
		break;
	case 1:
		ucLed[0] = 1;
		ucLed[1] = Dark_Work;
		Relay(1);
		break;
	case 2:
		ucLed[0] = Led_Light_Wait_Flag;
		ucLed[1] = 0;
		Relay(0);
		break;
	}
	ucLed[2] = Reach_Flag;
	Led_Disp(ucLed);
}


void AD_DA()
{
	unsigned char temp_ad;
	temp_ad = Ad_Read(0x41);
	Dark_Work = (temp_ad <= 1.2 * 51);
}


void Get_Distance()
{
	Obstacle_Distance = Ut_Wave_Data();
	if (Obstacle_Distance == 0)
		Obstacle_Distance = 255;
	else
		Obstacle_Distance += 3;
	if (Obstacle_Distance <= 30 && Work_Mode == 1)
		Work_Mode = 2;
}


void Uart_Proc()
{
	unsigned int Temp_x, Temp_y;

	if (Uart_Rx_Index == 0)
		return;

	if (Uart_Rx_Tick >= 10)
	{
		Uart_Rx_Tick = 0; 
		Uart_Rx_Flag = 0;

		if (strcmp(Uart_Rx_Buf, "#") == 0)
			printf("(%u,%u)", Current_Location[0], Current_Location[1]);

		else if (strcmp(Uart_Rx_Buf, "?") == 0)
		{
			switch (Work_Mode)
			{
			case 0:

				printf("Idle");
				break;
			case 1:

				printf("Busy");
				break;
			case 2:

				printf("Wait");
				break;
			}
		}

		else if (Uart_Rx_Buf[0] == '(' && Uart_Rx_Buf[Uart_Rx_Index - 1] == ')')
		{

			if (sscanf(Uart_Rx_Buf, "(%u,%u)", &Temp_x, &Temp_y) == 2)
			{

				if (Work_Mode == 0)
				{
					Target_Location[0] = Temp_x;
					Target_Location[1] = Temp_y;
					Get_Target_Flag = 1;
					printf("Got it");
				}
				else if (Work_Mode == 2)
					printf("Wait");
				else
					printf("Busy");
			}
			else
				printf("Error");
		}
		else
			printf("Error");
		memset(Uart_Rx_Buf, 0, 9);
		Uart_Rx_Index = 0;
	}
}

void Uart1_Isr() interrupt 4
{
	if (RI)
	{
		Uart_Rx_Flag = 1;
		Uart_Rx_Tick = 0; 
		Uart_Rx_Buf[Uart_Rx_Index] = SBUF;
		Uart_Rx_Index++;
		RI = 0;

		if (Uart_Rx_Index > 9)
		{
			Uart_Rx_Flag = 0;
			Uart_Rx_Tick = 0;
			Uart_Rx_Index = 0;
			memset(Uart_Rx_Buf, 0, 9);
		}
	}
}

//定时器0初始化为计数器
void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			//设置定时器模式
	TL0 = 0x00;				//设置定时初始值
	TH0 = 0x00;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
}
//定时器1的初始化
void Timer1_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	ET1 = 1;				//使能定时器1中断
	EA=1;//使能全局中断
}

void Timer1_Isr(void) interrupt 3
{
	unsigned char Temp_B_Value = -B_Value;
	uwTick++;
	Seg_Pos = (++Seg_Pos) % 8;
	if (Seg_Buf[Seg_Pos] > 20)
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1);
	else
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);
	if (Uart_Rx_Flag)
		Uart_Rx_Tick++;
	if (++Time_1s_Freq == 1000)
	{
		Time_1s_Freq = 0;
		Freq_Value = TH0 << 8 | TL0;

		if (B_Value < 0)
			Speed_Value_10x = (float)(3.14 * R_Value_10x * Freq_Value) / 100.0f - Temp_B_Value * 10;
		else
			Speed_Value_10x = (float)(3.14 * R_Value_10x * Freq_Value) / 100.0f + B_Value * 10;
		TH0 = TL0 = 0;
	}
	if (Work_Mode == 2)
	{
		if (++Time_100Ms_Wait == 100)
		{
			Time_100Ms_Wait = 0;
			Led_Light_Wait_Flag ^= 1;
		}
	}
	else
	{
		Time_100Ms_Wait = 0;
		Led_Light_Wait_Flag = 0;
	}

	if (Reach_Flag)
	{

		if (++Time_3s == 3000)
		{
			Time_3s = 0;
			Reach_Flag = 0;
		}
	}
	else
		Time_3s = 0;
}


typedef struct
{
	void (*task_func)(void);   
	unsigned long int rate_ms; 
	unsigned long int last_ms; 
} task_t;

idata task_t Scheduler_Task[] =
	{
		{Led_Proc, 1, 0},
		{Key_Proc, 10, 0},
		{Seg_Proc, 80, 0},
		{Uart_Proc, 10, 0},
		{Ctrl_Proc, 1, 0},
		{AD_DA, 160, 0},
		{Get_Distance, 100, 0}};

idata unsigned char task_num;

void Scheduler_Init()
{
	task_num = sizeof(Scheduler_Task) / sizeof(task_t);
}

void Scheduler_Run()
{
	unsigned char i;
	for (i = 0; i < task_num; i++)
	{
		unsigned long int now_time = uwTick;
		if (now_time >= Scheduler_Task[i].last_ms + Scheduler_Task[i].rate_ms)
		{
			Scheduler_Task[i].last_ms = now_time;
			Scheduler_Task[i].task_func();
		}
	}
}

void main()
{
	System_Init();
	Timer0_Init();
	Scheduler_Init();
	Uart1_Init();
	Timer1_Init();
	while (1)
	{
		Scheduler_Run();
	}
}
