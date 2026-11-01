#include <STC15F2K60S2.H>

#include "init.h"
#include "led.h"
#include "seg.h"
#include "key.h"

#include "ds1302.h"
#include "onewire.h"
#include "iic.h"
#include "ultrasound.h"
#include "uart.h"

#include "string.h"
#include "stdio.h"

// ==================== 全局变量定义区 ====================

// ---------- 系统时钟 ----------
idata unsigned long int uwTick; // 系统滴答计数器，1ms递增

// ---------- 任务调度 ----------
idata unsigned char task_num; // 任务数量

// ---------- LED控制 ----------
pdata unsigned char ucLed[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // LED状态数组，0-3：数据界面指示，4-5：错误指示，6-7：闪烁控制
bit Led6_Blinking;										 // LED7闪烁标志（代码里面是6），用于频率趋势异常指示
bit Led7_Blinking;										 // LED8闪烁标志（代码里面是7），用于频率突变异常指示

// ---------- 数码管显示 ----------
pdata unsigned char Seg_Buf[8] = {10, 10, 10, 10, 10, 10, 10, 10}; // 数码管显示缓冲区，10表示空，11表示'-'，12表示'P'，13表示'E'
idata unsigned char Seg_Pos = 0;								   // 数码管当前扫描位置（0-7）

// ---------- 按键处理 ----------
idata unsigned char Key_Val;  // 当前按键值
idata unsigned char Key_Old;  // 上一次按键值
idata unsigned char Key_Up;	  // 按键抬起标志
idata unsigned char Key_Down; // 按键按下标志

// ---------- RTC时间 ----------
pdata unsigned char ucRtc[3] = {23, 59, 50}; // 时分秒数组 [时, 分, 秒]

// ---------- 传感器数据 ----------
idata unsigned char Liquid_Height; // 液位高度（单位：cm）
idata unsigned int Freq;		   // 当前频率值（单位：Hz）
idata char Temperature_Integer;	   // 温度整数部分（单位：℃）
idata unsigned int Speed_10x;	   // 声速10倍值，用于超声波测距计算

// ---------- 串口通信 ----------
idata unsigned char Uart_Rx_Index;		   // 串口接收缓冲区索引
pdata unsigned char Uart_Rx_Buf[64] = {0}; // 串口接收缓冲区（64字节）
idata unsigned char Uart_Rx_Flag;		   // 串口接收标志
idata unsigned char Uart_Rx_Tick;		   // 串口接收超时计数

// ---------- 界面显示控制 ----------
// 主界面：0-数据界面  1-参数界面  2-标定界面
idata unsigned char Seg_Show_Mode;
// 子页面：
//   数据界面(Mode=0)：0-时间  1-温度和液位  2-空间和体积  3-温度和重量
//   参数界面(Mode=1)：0-H1  1-H2  2-F  3-S  4-r  5-L  6-W  7-H
//   标定界面(Mode=2)：0-0V  1-5V  2-10V  3-15V  4-20V
idata unsigned char Seg_Show_Type;

// ---------- 系统参数配置（可通过串口修改）----------
xdata unsigned int H1_Para_Cm = 100;	// 液位上限（单位：cm）
xdata unsigned int H2_Para_Cm = 10;		// 液位下限（单位：cm）
xdata unsigned int Freq_Para = 2000;	// 频率阈值参数（单位：Hz）
xdata unsigned char Container_Type = 2; // 容器类型：0-球体  1-圆柱体  2-长方体
xdata unsigned int R_Para_Cm = 100;		// 球体/圆柱体半径（单位：cm）
xdata unsigned int H_Para_Cm = 100;		// 圆柱体/长方体高度（单位：cm）
xdata unsigned int L_Para_Cm = 100;		// 长方体长度（单位：cm）
xdata unsigned int W_Para_Cm = 100;		// 长方体宽度（单位：cm）

// ---------- 计算结果 ----------
idata unsigned char Remaining_Space; // 剩余空间百分比（0-100）
idata unsigned int Volume_10x;		 // 液体体积的10倍值

// ---------- 电压校准与重量测量 ----------
idata int Voltage_Calibration_Mv = 0;									 // 电压校准值（单位：mV），ADC分辨率约20mV
idata unsigned int Weight_10x = 0;										 // 重量的10倍值（单位：10kg）
xdata unsigned char Calibration_Point_10x[5] = {0, 10, 20, 40, 50};		 // 重量-电压标定点10倍值（实际使用值）
xdata unsigned char Calibration_Point_Ctrl_10x[5] = {0, 10, 20, 40, 50}; // 重量-电压标定点10倍值（临时修改值）

// ---------- 频率异常检测 ----------
xdata unsigned int Freq_Last = 0;					 // 上一次频率值，用于趋势和突变检测
xdata unsigned char Freq_Trend_Type = 2;			 // 当前频率趋势类型：0-下降  1-上升  2-无趋势
xdata unsigned char Freq_Trend_Count = 0;			 // 连续趋势计数（0-4）
xdata unsigned char Freq_Trend_Last_Reported = 0xff; // 上次上报的趋势类型：0xFF-无  0-下降  1-上升

// ---------- 错误标志位 ----------
bit Error_Liquid_Overrun;			// 液位超限异常（错误代码500/501）
bit Error_Frequency_Numerical;		// 频率数值异常（错误代码502）
bit Error_Frequency_Trend;			// 频率趋势异常（错误代码503）
bit Error_Frequency_Drastic_Change; // 频率突变异常（错误代码504）

// ---------- 定时器变量 ----------
idata unsigned char Time_100ms; // 100ms定时器，用于LED6闪烁控制
idata unsigned char Time_200ms; // 200ms定时器，用于LED7闪烁控制
idata unsigned int Time_3s;		// 3秒定时器，用于频率突变异常锁定
idata unsigned int Time_1s;		// 1秒计数器，用于频率测量

bit Speed_10x_EN = 0; // 速度10倍值使能标志位，保证速度10倍更新了之后才进行距离测量

bit Send_Info_EN = 1; // 是否开启数据上报（这个主要是用于上位机观察，比赛可以不管）
// ==================== 全局变量定义结束 ====================

void Key_Proc()
{
	unsigned char i;
	bit is_valid = 1; // 是否有效
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Val ^ Key_Old);
	Key_Up = ~Key_Val & (Key_Val ^ Key_Old);
	Key_Old = Key_Val;
	switch (Key_Down)
	{
	case 4:

		// 离开标定界面，校验一次
		if (Seg_Show_Mode == 2)
		{
			for (i = 0; i < 4; i++)
			{
				// 不合法
				if (Calibration_Point_Ctrl_10x[i] > Calibration_Point_Ctrl_10x[i + 1])
				{
					is_valid = 0;
					break;
				}
			}
		}
		if (is_valid)
		{
			for (i = 0; i < 5; i++)
			{
				Calibration_Point_10x[i] = Calibration_Point_Ctrl_10x[i];
			}
		}
		// 主界面切换0~2，这里注意将子界面的下标设置为0
		Seg_Show_Mode = (Seg_Show_Mode == 2)
							? 0
							: (Seg_Show_Mode + 1);
		Seg_Show_Type = 0;

		// 进入标定界面，那么我们需要将真实值给控制值
		if (Seg_Show_Mode == 2)
		{
			for (i = 0; i < 5; i++)
			{
				Calibration_Point_Ctrl_10x[i] = Calibration_Point_10x[i];
			}
		}
		break;

	case 5:
		// 子界面切换
		switch (Seg_Show_Mode)
		{
		case 0:
			/* 数据界面 */
			// 0~3
			Seg_Show_Type = (Seg_Show_Type == 3)
								? 0
								: (Seg_Show_Type + 1);
			break;
		case 1:
			/* 参数界面 */
			// 0~7
			Seg_Show_Type = (Seg_Show_Type == 7)
								? 0
								: (Seg_Show_Type + 1);
			break;
		case 2:
			/* 标定界面 */
			// 0~4
			Seg_Show_Type = (Seg_Show_Type == 4)
								? 0
								: (Seg_Show_Type + 1);
			break;
		}
		break;

	case 8:
		// 处于标定界面下，这里我个人认为是锁死上限
		if (Seg_Show_Mode == 2)
		{
			if (Calibration_Point_Ctrl_10x[Seg_Show_Type] < 50)
			{
				Calibration_Point_Ctrl_10x[Seg_Show_Type]++;
			}
		}
		break;
	case 9:
		// 处于标定界面下，这里我个人认为是锁死下限
		if (Seg_Show_Mode == 2)
		{
			if (Calibration_Point_Ctrl_10x[Seg_Show_Type] > 0)
			{
				Calibration_Point_Ctrl_10x[Seg_Show_Type]--;
			}
		}
		break;
	}
}

void Seg_Proc()
{
	unsigned char temp_temperature = 0; // 临时温度
	switch (Seg_Show_Mode)
	{
	case 0:
		/* 数据页面 */
		switch (Seg_Show_Type)
		{
		case 0:
			/* 时间界面 */
			Seg_Buf[0] = ucRtc[0] / 10 % 10;
			Seg_Buf[1] = ucRtc[0] % 10;
			Seg_Buf[2] = 11; //-
			Seg_Buf[3] = ucRtc[1] / 10 % 10;
			Seg_Buf[4] = ucRtc[1] % 10;
			Seg_Buf[5] = 11; //-
			Seg_Buf[6] = ucRtc[2] / 10 % 10;
			Seg_Buf[7] = ucRtc[2] % 10;
			break;
		case 1:
			/* 温度和液位界面 */
			Seg_Buf[0] = 10;
			// 因为芯片本身范围就是-40~85，所以我们正数的时候不用考虑最高位
			if (Temperature_Integer < 0)
			{
				Seg_Buf[1] = 11;						 //-
				temp_temperature = -Temperature_Integer; // 转成正数
			}
			else
			{
				Seg_Buf[1] = 10;
				temp_temperature = Temperature_Integer;
			}
			Seg_Buf[2] = ((temp_temperature >= 10))
							 ? temp_temperature / 10 % 10
							 : 10;

			Seg_Buf[3] = temp_temperature % 10;
			Seg_Buf[4] = 11; //-
			Seg_Buf[5] = Liquid_Height / 100 % 10 + ',';
			Seg_Buf[6] = Liquid_Height / 10 % 10;
			Seg_Buf[7] = Liquid_Height % 10;
			break;
		case 2:
			/* 空间和体积界面 */
			Seg_Buf[0] = (Remaining_Space >= 100)
							 ? Remaining_Space / 100
							 : 10;
			Seg_Buf[1] = (Remaining_Space >= 10)
							 ? Remaining_Space / 10 % 10
							 : 10;
			Seg_Buf[2] = Remaining_Space % 10;
			Seg_Buf[3] = 11; //-
			Seg_Buf[4] = (Volume_10x >= 1000)
							 ? Volume_10x / 1000 % 10
							 : 10;
			Seg_Buf[5] = (Volume_10x >= 100)
							 ? Volume_10x / 100 % 10
							 : 10;
			Seg_Buf[6] = Volume_10x / 10 % 10 + ',';
			Seg_Buf[7] = Volume_10x % 10;
			break;
		case 3:
			/* 温度和重量界面 */
			if (Temperature_Integer < 0)
			{
				Seg_Buf[0] = 11;						 //-
				temp_temperature = -Temperature_Integer; // 转成正数
			}
			else
			{
				Seg_Buf[0] = 10;
				temp_temperature = Temperature_Integer;
			}
			Seg_Buf[1] = ((temp_temperature >= 10))
							 ? temp_temperature / 10 % 10
							 : 10;
			Seg_Buf[2] = temp_temperature % 10;
			Seg_Buf[3] = 11; //-
			Seg_Buf[4] = (Weight_10x >= 1000)
							 ? Weight_10x / 1000 % 10
							 : 10;
			Seg_Buf[5] = (Weight_10x >= 100)
							 ? Weight_10x / 100 % 10
							 : 10;
			Seg_Buf[6] = Weight_10x / 10 % 10 + ',';
			Seg_Buf[7] = Weight_10x % 10;
			break;
		}
		break;
	case 1:
		/* 参数页面 */
		Seg_Buf[0] = 12;				// P
		Seg_Buf[1] = Seg_Show_Type + 1; // 1~8
		Seg_Buf[2] = 11;				//-
		switch (Seg_Show_Type)
		{
		case 0:
			/* H1 */
			Seg_Buf[3] = 10;
			Seg_Buf[4] = (H1_Para_Cm >= 10000)
							 ? H1_Para_Cm / 10000 % 10
							 : 10;
			Seg_Buf[5] = (H1_Para_Cm >= 1000)
							 ? H1_Para_Cm / 1000 % 10
							 : 10;
			Seg_Buf[6] = H1_Para_Cm / 100 % 10 + ',';
			Seg_Buf[7] = H1_Para_Cm / 10 % 10;
			break;
		case 1:
			/* H2 */
			Seg_Buf[3] = 10;
			Seg_Buf[4] = (H2_Para_Cm >= 10000)
							 ? H2_Para_Cm / 10000 % 10
							 : 10;
			Seg_Buf[5] = (H2_Para_Cm >= 1000)
							 ? H2_Para_Cm / 1000 % 10
							 : 10;
			Seg_Buf[6] = H2_Para_Cm / 100 % 10 + ',';
			Seg_Buf[7] = H2_Para_Cm / 10 % 10;
			break;
		case 2:
			/* F */
			Seg_Buf[3] = (Freq_Para >= 10000)
							 ? Freq_Para / 10000 % 10
							 : 10;
			Seg_Buf[4] = (Freq_Para >= 1000)
							 ? Freq_Para / 1000 % 10
							 : 10;
			Seg_Buf[5] = (Freq_Para >= 100)
							 ? Freq_Para / 100 % 10
							 : 10;
			Seg_Buf[6] = (Freq_Para >= 10)
							 ? Freq_Para / 10 % 10
							 : 10;
			Seg_Buf[7] = Freq_Para % 10;
			break;
		case 3:
			/* S */
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = 10;
			Seg_Buf[7] = Container_Type;
			break;
		case 4:
			/* r */
			Seg_Buf[3] = 10;
			Seg_Buf[4] = (R_Para_Cm >= 10000)
							 ? R_Para_Cm / 10000 % 10
							 : 10;
			Seg_Buf[5] = (R_Para_Cm >= 1000)
							 ? R_Para_Cm / 1000 % 10
							 : 10;
			Seg_Buf[6] = R_Para_Cm / 100 % 10 + ',';
			Seg_Buf[7] = R_Para_Cm / 10 % 10;
			break;
		case 5:
			/* L */
			Seg_Buf[3] = 10;
			Seg_Buf[4] = (L_Para_Cm >= 10000)
							 ? L_Para_Cm / 10000 % 10
							 : 10;
			Seg_Buf[5] = (L_Para_Cm >= 1000)
							 ? L_Para_Cm / 1000 % 10
							 : 10;
			Seg_Buf[6] = L_Para_Cm / 100 % 10 + ',';
			Seg_Buf[7] = L_Para_Cm / 10 % 10;
			break;
		case 6:
			/* W */
			Seg_Buf[3] = 10;
			Seg_Buf[4] = (W_Para_Cm >= 10000)
							 ? W_Para_Cm / 10000 % 10
							 : 10;
			Seg_Buf[5] = (W_Para_Cm >= 1000)
							 ? W_Para_Cm / 1000 % 10
							 : 10;
			Seg_Buf[6] = W_Para_Cm / 100 % 10 + ',';
			Seg_Buf[7] = W_Para_Cm / 10 % 10;
			break;
		case 7:
			/* H */
			Seg_Buf[3] = 10;
			Seg_Buf[4] = (H_Para_Cm >= 10000)
							 ? H_Para_Cm / 10000 % 10
							 : 10;
			Seg_Buf[5] = (H_Para_Cm >= 1000)
							 ? H_Para_Cm / 1000 % 10
							 : 10;
			Seg_Buf[6] = H_Para_Cm / 100 % 10 + ',';
			Seg_Buf[7] = H_Para_Cm / 10 % 10;
			break;
		}
		break;
	case 2:
		/* 标定页面 */
		Seg_Buf[0] = 13;						 // E
		Seg_Buf[1] = Seg_Show_Type >> 1;		 // *5/10等于/2，也就是右移一位
		Seg_Buf[2] = (Seg_Show_Type & 0x01) * 5; // 取余数然后*5
		Seg_Buf[3] = 10;
		Seg_Buf[4] = 10;
		Seg_Buf[5] = 10;
		Seg_Buf[6] = Calibration_Point_Ctrl_10x[Seg_Show_Type] / 10 % 10 + ',';
		Seg_Buf[7] = Calibration_Point_Ctrl_10x[Seg_Show_Type] % 10;
		break;
	}
}

void Led_Proc()
{
	if (Seg_Show_Mode == 0)
	{
		ucLed[0] = (Seg_Show_Type == 0);
		ucLed[1] = (Seg_Show_Type == 1);
		ucLed[2] = (Seg_Show_Type == 2);
		ucLed[3] = (Seg_Show_Type == 3);
	}
	else
	{
		ucLed[0] = 0;
		ucLed[1] = 0;
		ucLed[2] = 0;
		ucLed[3] = 0;
	}
	ucLed[4] = Error_Liquid_Overrun;
	ucLed[5] = Error_Frequency_Numerical;
	ucLed[6] = Led6_Blinking;
	ucLed[7] = Led7_Blinking;
	Led_Disp(ucLed);
	Relay(Error_Frequency_Drastic_Change ||
		  Error_Frequency_Numerical ||
		  Error_Frequency_Trend ||
		  Error_Liquid_Overrun);
}

void Get_Time()
{
	Read_Rtc(ucRtc);
}

void Get_Temperature()
{
	float temperature = rd_temperature();
	Speed_10x_EN = 1;
	Temperature_Integer = (char)temperature; // 温度整数部分
	Speed_10x = 3300 + 6 * 10 * temperature;
	Voltage_Calibration_Mv = (int)(4 * temperature * temperature / 100) - 10 * temperature + 255;
}

void Get_AD()
{
	unsigned char i;
	unsigned int dV = 0;				   // 动态计算标定电压差
	unsigned int dV_Range = 0;			   // 动态计算标定电压差范围
	unsigned char voltage = Ad_Read(0x03); // 读取电压0~255->0~5V
	unsigned int current_voltage = (unsigned long)(voltage * 1000) / 51;
	// 因为我们这里是10倍存放的标定值，所以转换为mv只需要*100U U表示无符号
	if (current_voltage <= Calibration_Point_10x[0] * 100U)
		Weight_10x = 0;
	else if (current_voltage >= Calibration_Point_10x[4] * 100U)
		Weight_10x = 200;
	else
	{
		for (i = 0; i < 4; i++)
		{
			if (current_voltage < Calibration_Point_10x[i + 1] * 100U)
			{
				// 计算公式: Weight = W_start + (dV * dW_Range) / dV_Range
				// 1. 算电压差 dV = current - V_start
				dV = current_voltage - Calibration_Point_10x[i] * 100U;
				// 2. 算区间跨度
				dV_Range = (Calibration_Point_10x[i + 1] - Calibration_Point_10x[i]) * 100U;
				// 3. 核心计算 (用 long 承载乘积)
				Weight_10x = (i * 50) + (unsigned long)dV * 50 / dV_Range;
				break;
			}
		}
	}
}

void Get_Distance()
{
	float temp1;				 // 中间计算变量
	unsigned char temp_distance; // 单位是cm，液体高度为参数-这个值
	temp_distance = Ut_Wave_Data(Speed_10x);
	if (Speed_10x_EN) // 当温度测量后才有效
	{
		// printf("%bu\r\n", temp_distance);
		//  我们这里做个定义，H代表容器高度，r代表球的半径，h代表测距，d代表液体的高度
		//   不同类型不同计算
		switch (Container_Type)
		{
		case 0:
			/* 球体 */
			Liquid_Height = 2 * R_Para_Cm - temp_distance;
			// 现在根据我们推导得到的球缺公式可以计算液体体积
			/*
			 V=pi*d^2*(3r-d)/3
			这里我们对右边做个处理
			pi/3*d^2*(3r-d)，并且我们这里又知道
			本质上3r-d=3*R_Para_Cm-L_H=3*R_Para_Cm-2*R_Para_Cm+temp_distance
			=temp_distance+R_Para_Cm
			所以我们可以在这里直接走个计算
			V=pi*L_H^2*(temp_distance+R_Para_Cm)/3
			我们需要注意到这里的全部单位都是cm，所以计算出来的结果是cm^3，但是我们最终的
			V_10x需要的是0.1m^3=100000cm^3，所以我们需要将结果缩小100000倍
			V_10x=pi/300000*L_H^2*(temp_distance+R_Para_Cm)
			我们在这里可以一边算一边进行缩小，而不是最终的时候缩小
			并且我们又可以注意到，因为题目告诉我们pi是3.14，所以我们在这里先简化一下和pi相关的常数
			pi/300=3.14/300=1/95.54，我们这里可以直接用这个值来进行除法
			剩下的就直接进行运算即可，
			*/
			temp1 = Liquid_Height * Liquid_Height / 1000.0f * (temp_distance + R_Para_Cm);
			Volume_10x = temp1 / 95.54f;
			// 根据推导我们可以知道百分比n=h^2(3r-h)/4r^3
			// 在这里因为他们的单位都是cm，所以可以直接计算，但是要注意一下不要爆掉了
			//  n=temp_distance^2*(3*R_Para_Cm-temp_distance)/(4*R_Para_Cm^3)
			Remaining_Space = ((unsigned long int)temp_distance * temp_distance) *
							  (3 * R_Para_Cm - temp_distance) /
							  (4 * (unsigned long int)R_Para_Cm * R_Para_Cm * R_Para_Cm);
			break;

		case 1:
			/* 圆柱体 */
			Liquid_Height = H_Para_Cm - temp_distance;

			// 这里其实就是简单的圆柱体了，也就是V=pi*r^2*d，这里同样需要注意一下单位
			//  V_10x=pi*R_Para_Cm^2*Liquid_Height/100000
			//  我们这里可以一边计算一边缩小，而不是最终的时候缩小
			// 并且我们又可以注意到，因为题目告诉我们pi是3.14，所以我们在这里先简化一下和pi相关的常数
			// pi/300=3.14/300=1/95.54，我们这里可以直接用这个值来进行除法
			temp1 = R_Para_Cm * R_Para_Cm / 1000.0f * Liquid_Height;
			Volume_10x = temp1 / 95.54f;
			// 这里其实剩余百分比很好求，就是n=h/H，别忘记*100就行
			Remaining_Space = (unsigned int)temp_distance * 100 / H_Para_Cm;
			break;

		case 2:
			/* 长方体 */
			Liquid_Height = H_Para_Cm - temp_distance;
			// 这里就是经典的长宽高了，我们V=L*W*H，这里同样需要注意一下单位
			//  V_10x=L_Para_Cm*W_Para_Cm*Liquid_Height/100000
			temp1 = L_Para_Cm * W_Para_Cm / 100000.0f * Liquid_Height;
			Volume_10x = temp1;
			// 剩余百分比也是高度比，就是n=h/H，别忘记*100就行
			Remaining_Space = (unsigned int)temp_distance * 100 / H_Para_Cm;
			break;
		}
		// 检查液位高度和测距是否异常
		// printf("%bu,%bu\r\n", Liquid_Height, temp_distance);
		// printf("%bu.%02bu\r\n", Liquid_Height / 100, Liquid_Height % 100);
		// 液位异常检测和上报
		if (Liquid_Height > H1_Para_Cm)
		{
			// 500：超过液位上限
			Error_Liquid_Overrun = 1;
			printf("[500,%02bu%02bu%02bu,%bu.%02bu]\r\n",
				   ucRtc[0], ucRtc[1], ucRtc[2],
				   Liquid_Height / 100, Liquid_Height % 100);
		}
		else if (Liquid_Height < H2_Para_Cm)
		{
			// 501：低于液位下限
			Error_Liquid_Overrun = 1;
			printf("[501,%02bu%02bu%02bu,%bu.%02bu]\r\n",
				   ucRtc[0], ucRtc[1], ucRtc[2],
				   Liquid_Height / 100, Liquid_Height % 100);
		}
		else
		{
			// 液位正常
			Error_Liquid_Overrun = 0;
		}

		// 数据上报(用于上位机实时监测)
		if (Send_Info_EN)
		{
			// 格式: L:液位(cm),T:温度(℃),W:重量(t),F:频率(Hz),S:声速(m/s),V:体积(0.1L),R:剩余空间(%)
			printf("L:%bu,T:%bu,W:%u.%02u,F:%u,S:%u.%01u,V:%u.%01u,R:%bu\r\n",
				   Liquid_Height,
				   Temperature_Integer,
				   Weight_10x / 10, Weight_10x % 10,
				   Freq,
				   Speed_10x / 10, Speed_10x % 10,
				   Volume_10x / 10, Volume_10x % 10,
				   Remaining_Space);
		}
	}
}

// 比较两个字符串是否相等
bit str_equal(unsigned char *str1, unsigned char *str2)
{
	unsigned char i = 0;
	while (str1[i] != '\0' && str2[i] != '\0')
	{
		if (str1[i] != str2[i])
			return 0;
		i++;
	}
	return (str1[i] == str2[i]);
}

// 解析带小数点的数值字符串为cm（输入单位m，输出单位cm）
// 例如: "2.0" -> 200cm,  "1.5" -> 150cm
unsigned int parse_m_to_cm(unsigned char *str)
{
	unsigned int integer_part = 0;
	unsigned int decimal_part = 0;
	unsigned char i = 0;
	unsigned char decimal_pos = 0xFF;

	// 找小数点位置
	while (str[i] != '\0')
	{
		if (str[i] == '.')
		{
			decimal_pos = i;
			break;
		}
		i++;
	}

	// 解析整数部分
	i = 0;
	if (decimal_pos != 0xFF)
	{
		// 有小数点
		while (i < decimal_pos)
		{
			integer_part = integer_part * 10 + (str[i] - '0');
			i++;
		}

		// 解析小数部分（只取1位）
		i++; // 跳过小数点
		if (str[i] >= '0' && str[i] <= '9')
		{
			decimal_part = (str[i] - '0') * 10; // 0.X米 = X*10厘米
		}
	}
	else
	{
		// 没有小数点
		while (str[i] != '\0')
		{
			integer_part = integer_part * 10 + (str[i] - '0');
			i++;
		}
	}

	// 转换: 整数部分*100 + 小数部分*10
	return integer_part * 100 + decimal_part;
}

// 解析纯整数（用于频率F和容器类型S）
unsigned int parse_integer(unsigned char *str)
{
	unsigned int result = 0;
	unsigned char i = 0;

	while (str[i] >= '0' && str[i] <= '9')
	{
		result = result * 10 + (str[i] - '0');
		i++;
	}

	return result;
}

// 提取单个参数对 "(H1,2.0)"
// 返回右括号的位置，失败返回0xFF
unsigned char extract_one_param(unsigned char start,
								unsigned char *id_buf,
								unsigned char *val_buf)
{
	unsigned char i = start;
	unsigned char j = 0;

	// 找左括号
	while (i < Uart_Rx_Index && Uart_Rx_Buf[i] != '(')
		i++;

	if (i >= Uart_Rx_Index)
		return 0xFF;

	i++; // 跳过 '('

	// 提取ID (到逗号或冒号)
	j = 0;
	while (i < Uart_Rx_Index && Uart_Rx_Buf[i] != ',' && Uart_Rx_Buf[i] != ':')
	{
		id_buf[j++] = Uart_Rx_Buf[i++];
	}
	id_buf[j] = '\0';

	if (i >= Uart_Rx_Index)
		return 0xFF;

	i++; // 跳过 ',' 或 ':'

	// 提取值 (到右括号)
	j = 0;
	while (i < Uart_Rx_Index && Uart_Rx_Buf[i] != ')')
	{
		val_buf[j++] = Uart_Rx_Buf[i++];
	}
	val_buf[j] = '\0';

	if (i >= Uart_Rx_Index)
		return 0xFF;

	return i; // 返回 ')' 的位置
}

// 处理单个参数（通过全局标志标记结果）
void handle_param(unsigned char *id, unsigned char *val,
				  unsigned char *has_ok, unsigned char *has_err)
{
	unsigned int temp;

	// 查询命令
	if (val[0] == '?')
	{
		if (str_equal(id, "H1"))
		{
			printf("(H1,%u.%u)\r\n", H1_Para_Cm / 100, (H1_Para_Cm % 100) / 10);
			*has_ok = 1;
		}
		else if (str_equal(id, "H2"))
		{
			printf("(H2,%u.%u)\r\n", H2_Para_Cm / 100, (H2_Para_Cm % 100) / 10);
			*has_ok = 1;
		}
		else if (str_equal(id, "F"))
		{
			printf("(F,%u)\r\n", Freq_Para);
			*has_ok = 1;
		}
		else if (str_equal(id, "S"))
		{
			printf("(S,%u)\r\n", (unsigned int)Container_Type);
			*has_ok = 1;
		}
		else if (str_equal(id, "r"))
		{
			printf("(r,%u.%u)\r\n", R_Para_Cm / 100, (R_Para_Cm % 100) / 10);
			*has_ok = 1;
		}
		else if (str_equal(id, "L"))
		{
			printf("(L,%u.%u)\r\n", L_Para_Cm / 100, (L_Para_Cm % 100) / 10);
			*has_ok = 1;
		}
		else if (str_equal(id, "W"))
		{
			printf("(W,%u.%u)\r\n", W_Para_Cm / 100, (W_Para_Cm % 100) / 10);
			*has_ok = 1;
		}
		else if (str_equal(id, "H"))
		{
			printf("(H,%u.%u)\r\n", H_Para_Cm / 100, (H_Para_Cm % 100) / 10);
			*has_ok = 1;
		}
		else
		{
			*has_err = 1; // 未知参数
		}
		return;
	}

	// 时间设置 (T:HHMMSS)
	if (str_equal(id, "T"))
	{
		unsigned char h, m, s;
		// 必须是6位数字
		if (val[0] < '0' || val[0] > '9' ||
			val[1] < '0' || val[1] > '9' ||
			val[2] < '0' || val[2] > '9' ||
			val[3] < '0' || val[3] > '9' ||
			val[4] < '0' || val[4] > '9' ||
			val[5] < '0' || val[5] > '9' ||
			val[6] != '\0')
		{
			*has_err = 1;
			return;
		}

		h = (val[0] - '0') * 10 + (val[1] - '0');
		m = (val[2] - '0') * 10 + (val[3] - '0');
		s = (val[4] - '0') * 10 + (val[5] - '0');

		if (h > 23 || m > 59 || s > 59)
		{
			*has_err = 1;
			return;
		}

		ucRtc[0] = h;
		ucRtc[1] = m;
		ucRtc[2] = s;
		Set_Rtc(ucRtc);
		*has_ok = 1;
		return;
	}

	// H1 液位上限 (输入m，存储cm)
	if (str_equal(id, "H1"))
	{
		temp = parse_m_to_cm(val);
		// 验证上限不能低于当前下限
		if (temp < H2_Para_Cm)
		{
			*has_err = 1; // 上限不能低于下限
			return;
		}
		H1_Para_Cm = temp;
		*has_ok = 1;
		return;
	}

	// H2 液位下限
	if (str_equal(id, "H2"))
	{
		temp = parse_m_to_cm(val);
		// 验证下限不能高于当前上限
		if (temp > H1_Para_Cm)
		{
			*has_err = 1; // 下限不能高于上限
			return;
		}
		H2_Para_Cm = temp;
		*has_ok = 1;
		return;
	}

	// F 频率
	if (str_equal(id, "F"))
	{
		temp = parse_integer(val);
		Freq_Para = temp;
		*has_ok = 1;
		return;
	}

	// S 容器类型
	if (str_equal(id, "S"))
	{
		temp = parse_integer(val);
		if (temp > 2) // 只能0,1,2
		{
			*has_err = 1;
			return;
		}
		Container_Type = (unsigned char)temp;
		*has_ok = 1;
		return;
	}

	// r 半径
	if (str_equal(id, "r"))
	{
		temp = parse_m_to_cm(val);
		R_Para_Cm = temp;
		*has_ok = 1;
		return;
	}

	// L 长度
	if (str_equal(id, "L"))
	{
		temp = parse_m_to_cm(val);
		L_Para_Cm = temp;
		*has_ok = 1;
		return;
	}

	// W 宽度
	if (str_equal(id, "W"))
	{
		temp = parse_m_to_cm(val);
		W_Para_Cm = temp;
		*has_ok = 1;
		return;
	}

	// H 高度
	if (str_equal(id, "H"))
	{
		temp = parse_m_to_cm(val);
		H_Para_Cm = temp;
		*has_ok = 1;
		return;
	}

	// 未知参数标识
	*has_err = 1;
}

// ==================== 串口处理主函数 ====================
void Uart_Proc()
{
	unsigned char pos = 0;
	unsigned char id[5];
	unsigned char val[10];
	unsigned char has_ok = 0;
	unsigned char has_err = 0;

	if (Uart_Rx_Index == 0)
		return;

	if (Uart_Rx_Tick >= 10)
	{
		Uart_Rx_Flag = 0;
		Uart_Rx_Tick = 0;

		// 解析所有参数对
		while (pos < Uart_Rx_Index)
		{
			pos = extract_one_param(pos, id, val);

			if (pos == 0xFF) // 解析失败
			{
				has_err = 1;
				break;
			}

			// 处理参数
			handle_param(id, val, &has_ok, &has_err);

			pos++; // 跳过当前的')'
		}

		// 发送响应
		if (has_err && has_ok)
		{
			printf("WARN\r\n");
		}
		else if (has_err)
		{
			printf("ERROR\r\n");
		}
		else if (has_ok)
		{
			printf("OK\r\n");
		}

		// 清空缓冲区
		Uart_Rx_Index = 0; // 直接重置索引即可，不需要清空数据
	}
}

void Freq_Error_Check()
{

	// 502: 频率数值异常检测
	if (Freq > Freq_Para)
	{
		if (!Error_Frequency_Numerical)
		{
			Error_Frequency_Numerical = 1;
			printf("[502,%02bu%02bu%02bu,%u]\r\n",
				   ucRtc[0], ucRtc[1], ucRtc[2],
				   Freq);
		}
	}
	else
	{
		Error_Frequency_Numerical = 0;
	}

	// 504: 频率剧烈变化检测
	// 第一次采样时Freq_Last为0，跳过检测
	if (Freq_Last != 0)
	{
		// 判断2秒内（相邻两次）变化是否>1000Hz
		if (((Freq > Freq_Last) && ((Freq - Freq_Last) > 1000)) ||
			((Freq_Last > Freq) && ((Freq_Last - Freq) > 1000)))
		{
			Error_Frequency_Drastic_Change = 1;
			printf("[504,%02bu%02bu%02bu,%u]\r\n",
				   ucRtc[0], ucRtc[1], ucRtc[2],
				   Freq);
		}
	}

	// 503: 频率趋势异常检测
	if (Freq_Last != 0)
	{ // 需要至少有一次历史数据
		if (Freq > Freq_Last)
		{
			// 当前是上升
			if (Freq_Trend_Type == 1)
			{
				// 延续上升趋势，五次就锁定一下，避免加爆
				Freq_Trend_Count = (Freq_Trend_Count >= 5)
									   ? 5
									   : Freq_Trend_Count + 1;
			}
			else
			{
				// 新的上升趋势开始
				Freq_Trend_Type = 1;
				Freq_Trend_Count = 1;
			}
		}
		else if (Freq < Freq_Last)
		{
			// 当前是下降
			if (Freq_Trend_Type == 0)
			{
				// 延续下降趋势
				Freq_Trend_Count = (Freq_Trend_Count >= 5)
									   ? 5
									   : Freq_Trend_Count + 1;
			}
			else
			{
				// 新的下降趋势开始
				Freq_Trend_Type = 0;
				Freq_Trend_Count = 1;
			}
		}
		else
		{
			// Freq == Freq_Last，趋势中断
			Freq_Trend_Type = 2;
			Freq_Trend_Count = 0;
			Error_Frequency_Trend = 0;
			Freq_Trend_Last_Reported = 0xFF;
		}

		// 判断是否达到趋势异常条件（连续4次 = 5个点）
		if (Freq_Trend_Count >= 4)
		{
			if (Freq_Trend_Type == 0)
			{
				// 连续下降
				if (Freq_Trend_Last_Reported != 0)
				{
					Error_Frequency_Trend = 1;
					Freq_Trend_Last_Reported = 0;
					printf("[503,%02bu%02bu%02bu,0]\r\n",
						   ucRtc[0], ucRtc[1], ucRtc[2]);
				}
			}
			else if (Freq_Trend_Type == 1)
			{
				// 连续上升
				if (Freq_Trend_Last_Reported != 1)
				{
					Error_Frequency_Trend = 1;
					Freq_Trend_Last_Reported = 1;
					printf("[503,%02bu%02bu%02bu,1]\r\n",
						   ucRtc[0], ucRtc[1], ucRtc[2]);
				}
			}
		}
	}

	// 更新历史值
	Freq_Last = Freq;
}

void Timer0_Init(void)
{
	AUXR &= 0x7F;
	TMOD &= 0xF0;
	TMOD |= 0x05;
	TL0 = 0x00;
	TH0 = 0x00;
	TF0 = 0;
	TR0 = 1;
}

void Timer1_Init(void)
{
	AUXR &= 0xBF;
	TMOD &= 0x0F;
	TL1 = 0x18;
	TH1 = 0xFC;
	TF1 = 0;
	TR1 = 1;
	ET1 = 1;
	EA = 1;
}

void Timer1_Isr(void) interrupt 3
{
	uwTick++;

	Seg_Pos = (++Seg_Pos) % 8;
	if (Seg_Buf[Seg_Pos] > 20)
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1);
	else
		Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);

	if (++Time_1s == 1000)
	{
		Time_1s = 0;
		Freq = (TH0 << 8) | TL0;
		TH0 = TL0 = 0;
	}
	if (Uart_Rx_Flag)
		Uart_Rx_Tick++;
	// 如果频率趋势异常
	if (Error_Frequency_Trend)
	{
		if (++Time_100ms >= 100)
		{
			Led6_Blinking ^= 1;
			Time_100ms = 0;
		}
	}
	else
	{
		Time_100ms = 0;
		Led6_Blinking = 0;
	}
	// 如果频率剧烈变化，那么锁定3s
	if (Error_Frequency_Drastic_Change)
	{
		Time_3s = 3000;
	}
	// 开始指定倒计时3s
	if (Time_3s > 0)
	{
		Time_3s--;
		if (++Time_200ms >= 200)
		{
			Led7_Blinking ^= 1;
			Time_200ms = 0;
		}
	}
	// 倒计时结束
	else
	{
		Time_200ms = 0;
		Led7_Blinking = 0;
	}
}

void Uart1_Isr(void) interrupt 4
{
	if (RI)
	{
		Uart_Rx_Flag = 1;
		Uart_Rx_Tick = 0;
		Uart_Rx_Buf[Uart_Rx_Index++] = SBUF;
		RI = 0;
		if (Uart_Rx_Index >= 64) // 修改为64
		{
			Uart_Rx_Index = 0; // 溢出保护，直接重置
		}
	}
}

typedef struct
{
	void (*task_func)(void);
	unsigned long int rate_ms;
	unsigned long int last_ms;
} task_t;

xdata task_t Scheduler_Task[] = {
	{Led_Proc, 1, 0},
	{Key_Proc, 10, 0},
	{Seg_Proc, 20, 0},
	{Get_Time, 100, 0},
	{Get_Temperature, 300, 0},
	{Get_AD, 150, 0},
	{Freq_Error_Check, 1000, 0},
	{Get_Distance, 120, 0},
	{Uart_Proc, 10, 0},
};

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
		if (now_time >= (Scheduler_Task[i].rate_ms + Scheduler_Task[i].last_ms))
		{
			Scheduler_Task[i].last_ms = now_time;
			Scheduler_Task[i].task_func();
		}
	}
}
void Delay750ms(void) //@12.000MHz
{
	unsigned char data i, j, k;

	_nop_();
	_nop_();
	i = 35;
	j = 51;
	k = 182;
	do
	{
		do
		{
			while (--k)
				;
		} while (--j);
	} while (--i);
}

void main()
{
	System_Init();
	Set_Rtc(ucRtc);
	Timer0_Init();
	rd_temperature();
	Delay750ms();
	Scheduler_Init();
	Uart1_Init();
	Timer1_Init();

	while (1)
	{
		Scheduler_Run();
	}
}
