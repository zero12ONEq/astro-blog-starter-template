#include <STC15F2K60S2.H>
#include "led.h"
#include "seg.h"
#include "key.h"
#include "init.h"
#include "iic.h"
#include "onewire.h"
#include "ul.h"

idata unsigned char key_val, key_old, key_down, key_up;

pdata unsigned char seg_buf[8] = {10, 10, 10, 10, 10, 10, 10, 10};
idata unsigned char seg_pos = 0;

pdata unsigned char ucLed[8] = {0, 0, 0, 0, 0, 0, 0, 0};

idata unsigned long int uwtick = 0;

idata unsigned char temperature;
idata unsigned int distance;
idata unsigned int distance_sum;

idata unsigned char seg_show_mode = 0;	 // 0 测距界面 1 参数设置界面
idata unsigned char range_show_mode = 0; // 0 首次/复位 1 单次 2 累加
idata unsigned char para_show_mode = 0;	 // 0 时间参数 1 测量参数

idata unsigned char para_time_arr[4] = {2, 4, 6, 4}; // 时间参数数组
idata unsigned char para_time_index = 0;			 // 时间参数数组下标
idata unsigned char para_num_arr[4] = {3, 5, 7, 5};	 // 次数参数数组
idata unsigned char para_num_index = 0;				 // 次数参数数组下标
idata unsigned char summation_index = 0;			 // 累加次数
bit summation_flag = 0;								 // 累加测量执行中
bit signal_flag = 0;								 // 单次测量执行中

idata unsigned char time_100ms = 0;
bit led_blink_flag = 0;

float speed = 340; // 声速

void Get_Distance();
void Key_Proc();
void Led_Proc();
void Seg_Proc();
void Get_Temperature();
void DA_Proc();

typedef struct
{
	void (*task_func)(void);
	unsigned long int rate_ms; // 任务执行周期
	unsigned long int last_ms; // 上一次任务执行时间
} task_t;

idata task_t scheduler_task[] = {
	{Led_Proc, 1, 0},
	{Key_Proc, 10, 0},
	{Seg_Proc, 20, 0},
	{Get_Temperature, 300, 0},
	{Get_Distance, 150, 0},
	{DA_Proc, 100, 0}};
void Key_Proc()
{
	key_val = Key_Read();
	key_down = key_val & (key_val ^ key_old);
	key_up = ~key_val & (key_val ^ key_old);
	key_old = key_val;
	if (summation_flag == 0)
	{
		// 当前没在累加测量
		if (key_down == 8)
		{
			if (seg_show_mode == 0)
			{
				seg_show_mode = 1;
				para_show_mode = 0;
			}
			else
			{
				if (para_show_mode == 0)
					para_show_mode = 1;
				else
					seg_show_mode = 0;
			}
		}

		if ((seg_show_mode == 0) && (key_down == 12))
		{
			distance = 0;
			distance_sum = 0;
			range_show_mode = 0;
		}
	}
	switch (seg_show_mode)
	{
	case 0:
		/* 测距界面 */
		if (key_down == 4)
		{
			signal_flag = 1;
			summation_flag = 0;
			summation_index = 0;
			distance = 0;
			distance_sum = 0;
			scheduler_task[4].last_ms = uwtick;
			range_show_mode = 1;
			Get_Distance();
		}
		if (key_down == 5)
		{
			summation_flag = 1;
			signal_flag = 0;
			summation_index = 0;
			distance = 0;
			distance_sum = 0;
			// 更新执行周期与最后一次执行时间
			scheduler_task[4].rate_ms = (unsigned int)para_time_arr[para_time_index] * 1000;
			scheduler_task[4].last_ms = uwtick;
			range_show_mode = 2;
			Get_Distance();
		}
		break;
	case 1:
		/* 参数设置界面 */
		if (key_down == 9)
		{
			switch (para_show_mode)
			{
			case 0:
				/* 时间参数 */
				para_time_index = (++para_time_index) % 4;
				break;
			case 1:
				/* 次数参数 */
				para_num_index = (++para_num_index) % 4;
				break;
			}
		}

		break;
	}
}

void Seg_Proc()
{
	switch (seg_show_mode)
	{
	case 0:
		/* 测距界面 */
		seg_buf[1] = 12; // C
		seg_buf[2] = temperature / 10 % 10;
		seg_buf[3] = temperature % 10;
		switch (range_show_mode)
		{
		case 0:
			/* 首次/复位 */
			seg_buf[0] = 12; // C
			seg_buf[4] = 10;
			seg_buf[5] = 10;
			seg_buf[6] = 10;
			seg_buf[7] = 10;
			break;
		case 1:
			/* 单次 */
			seg_buf[0] = 11; // A
			seg_buf[4] = (distance >= 1000)
							 ? distance / 1000 % 10
							 : 10;
			seg_buf[5] = (distance >= 100)
							 ? distance / 100 % 10
							 : 10;
			seg_buf[6] = (distance >= 10)
							 ? distance / 10 % 10
							 : 10;
			seg_buf[7] = distance % 10;
			break;
		case 2:
			/* 多次 */
			seg_buf[0] = 13; // E
			seg_buf[4] = (distance_sum >= 1000)
							 ? distance_sum / 1000 % 10
							 : 10;
			seg_buf[5] = (distance_sum >= 100)
							 ? distance_sum / 100 % 10
							 : 10;
			seg_buf[6] = (distance_sum >= 10)
							 ? distance_sum / 10 % 10
							 : 10;
			seg_buf[7] = distance_sum % 10;
			break;
		}
		break;

	case 1:
		/* 参数设置界面 */
		seg_buf[0] = 14; // P
		seg_buf[1] = para_show_mode + 1;
		seg_buf[2] = 10;
		seg_buf[3] = 10;
		seg_buf[4] = 10;
		seg_buf[5] = 10;
		seg_buf[6] = 0;
		switch (para_show_mode)
		{
		case 0:
			/* 时间参数 */
			seg_buf[7] = para_time_arr[para_time_index];
			break;
		case 1:
			/* 次数参数 */
			seg_buf[7] = para_num_arr[para_num_index];
			break;
		}
		break;
	}
}

void Led_Proc()
{
	ucLed[0] = (seg_show_mode == 0);
	switch (range_show_mode)
	{
	case 0:
		/* 首次/复位 */
		ucLed[1] = 0;
		break;
	case 1:
		/* 单次 */
		if (signal_flag == 0)
			ucLed[1] = (distance > 100) || (distance < 20);
		break;
	case 2:
		/* 多次 */
		if (summation_flag == 0)
			ucLed[1] = (distance_sum > 100) || (distance_sum < 20);
		break;
	}
	ucLed[2] = led_blink_flag;
	Led_Disp(ucLed);
}

void DA_Proc()
{
	float temp = 0;
	switch (range_show_mode)
	{
	case 0:
		/* 首次/复位 */
		temp = 0;
		break;
	case 1:
		/* 单次 */
		if (signal_flag == 0)
		{
			if (distance <= 20)
				temp = 1;
			else if (distance >= 100)
				temp = 5;
			else
				temp = (float)distance / 20.0f;
		}
		break;
	case 2:
		/* 多次 */
		if (summation_flag == 0)
		{
			if (distance_sum <= 20)
				temp = 1;
			else if (distance_sum >= 100)
				temp = 5;
			else
				temp = (float)distance_sum / 20.0f;
		}
		break;
	}
	Da_Write(temp * 51);
}
void Get_Temperature()
{
	float temp = 0;
	temp = Rd_Temperature();
	temperature = temp;
	speed = 330 + 0.6f * temp;
}

void Get_Distance()
{
	unsigned int temp;
	if (signal_flag)
	{
		// 单次测量
		temp = Ut_Wave_Data(speed);
		distance = temp;
		signal_flag = 0;
	}
	else if (summation_flag)
	{
		// 多次测量
		temp = Ut_Wave_Data(speed);
		distance_sum += temp;
		summation_index++;
		if (summation_index >= para_num_arr[para_num_index])
		{
			summation_flag = 0;
			summation_index = 0;
		}
	}
}

idata unsigned char task_num;

void Scheduler_Init()
{
	task_num = sizeof(scheduler_task) / sizeof(task_t);
}

void Scheduler_Run()
{
	unsigned char i;
	for (i = 0; i < task_num; i++)
	{
		unsigned long int now_time = uwtick;
		if (now_time >= scheduler_task[i].rate_ms + scheduler_task[i].last_ms)
		{
			scheduler_task[i].last_ms = now_time;
			scheduler_task[i].task_func();
		}
	}
}

void Timer1_Isr(void) interrupt 3
{
	uwtick++;

	seg_pos = (++seg_pos) % 8;
	if (seg_buf[seg_pos] > 20)
		Seg_Disp(seg_pos, seg_buf[seg_pos] - ',', 1);
	else
		Seg_Disp(seg_pos, seg_buf[seg_pos], 0);
	// 累加测量执行中
	if (summation_flag)
	{
		if (++time_100ms >= 100)
		{
			time_100ms = 0;
			led_blink_flag ^= 1;
		}
	}
	else
	{
		time_100ms = 0;
		led_blink_flag = 0;
	}
}

void Timer1_Init(void) // 1毫秒@12.000MHz
{
	AUXR &= 0xBF; // 定时器时钟12T模式
	TMOD &= 0x0F; // 设置定时器模式
	TL1 = 0x18;	  // 设置定时初始值
	TH1 = 0xFC;	  // 设置定时初始值
	TF1 = 0;	  // 清除TF1标志
	TR1 = 1;	  // 定时器1开始计时
	ET1 = 1;	  // 使能定时器1中断
	EA = 1;
}

void main()
{
	System_Init();
	Scheduler_Init();
	Timer1_Init();
	while (1)
	{
		Scheduler_Run();
	}
}
