#include <STC15F2K60S2.H>
#include "stdio.h"
#include "string.h"
#include "led.h"
#include "init.h"
#include "seg.h"
#include "key.h"
#include "iic.h"
#include "ultrasound.h"
#include "uart.h"
#include "math.h"

#define PI 3.14

// 频率测量
idata unsigned int freq = 0;
idata unsigned int Time_1S = 0;

// 串口
idata unsigned char Uart_Rx_Flag = 0;
idata unsigned char Uart_Rx_Index = 0;
pdata unsigned char Uart_Rx_Buf[10] = {0,0,0,0,0,0,0,0,0,0};
idata unsigned char Uart_Rx_Tick = 0;
// 数码管
idata unsigned char Seg_Pos = 0;
pdata unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};
idata unsigned char Seg_Disp_Mode = 0;     // 0-坐标 1-速度 2-参数
idata unsigned char seg_slow_down;
// 状态机（空闲=0，运行=1，等待=2）
idata unsigned char State_Disp_Mode = 0;

// 按键
idata unsigned char Key_Val = 0;
idata unsigned char Key_Old = 0;
idata unsigned char Key_Down = 0;
idata unsigned char Key_Up = 0;
idata unsigned char key_slow_down;
// LED
pdata unsigned char ucled[8] = {0,0,0,0,0,0,0,0};
idata bit Led_Flash_Flag = 0;
idata unsigned char Led_Flash_Tick = 0;
idata unsigned int Time_3s = 0;
idata bit Reach_Flag = 0;
idata bit L3_Flag = 0;

// AD 光敏（光敏地址0x41，夜暗标志）
idata bit Dark_Work = 0;
idata unsigned char ad_slow_down;
// 超声波
idata unsigned char Barrier_Distance = 0;
idata unsigned char us_slow_down;
// 运动参数
pdata unsigned int Destination[2] = {0,0};      // 目的坐标
pdata unsigned int Current_Location[2] = {0,0}; // 当前位置
idata unsigned int Speed_10x = 0;               // 速度x10
idata unsigned char Para_R_10x = 10;            // Rx10 (1.0)
idata char Para_B = 0;                         // B
idata unsigned char Para_R_CT_10x = 0;         // 参数设置临时R
idata char Para_CT_B = 0;                     // 参数设置临时B
idata bit Para_Setting_Mode = 0;              // 0-R 1-B
idata bit Gotit_Flag = 0;                    // 已接收目的坐标

// 运动模型变量（线性插值方案，避免逐帧sqrt）
pdata float speed_x = 0, speed_y = 0;             // 速度分量
pdata unsigned int Start_Location[2] = {0, 0};    // 运动起始位置
pdata unsigned int Movement_Time = 0;                     // 运动累计时间（0.1s单位）
idata unsigned char cnt_movement_time = 0;          // ISR内100ms计数器
idata bit Speed_Update_Flag = 0;                   // 速度更新标志

idata unsigned char ctrl_slow_down;
idata unsigned char speed_slow_down;
/* ========== 控制函数（线性插值，200ms调度）========== */
void Ctrl_Proc()
{		
		if(ctrl_slow_down<200) return;
		ctrl_slow_down=0;
    if (State_Disp_Mode == 1)
    {
        float new_x, new_y;
        float t_sec = Movement_Time * 0.1f;  // 转为秒

        // 障碍物保护
        if (Barrier_Distance <= 30)
        {
            State_Disp_Mode = 2;
            return;
        }

        // 线性插值计算当前位置
        new_x = (float)Start_Location[0] + speed_x * t_sec;
        new_y = (float)Start_Location[1] + speed_y * t_sec;

        // 到达检测
        if (speed_x >= 0)
        {
            if (new_x >= (float)Destination[0]) new_x = (float)Destination[0];
        }
        else
        {
            if (new_x <= (float)Destination[0]) new_x = (float)Destination[0];
        }
        if (speed_y >= 0)
        {
            if (new_y >= (float)Destination[1]) new_y = (float)Destination[1];
        }
        else
        {
            if (new_y <= (float)Destination[1]) new_y = (float)Destination[1];
        }

        Current_Location[0] = (unsigned int)(new_x + 0.5f);//四舍五入
        Current_Location[1] = (unsigned int)(new_y + 0.5f);//四舍五入

        // 两轴都到达目的地
        if (Current_Location[0] == Destination[0] && Current_Location[1] == Destination[1])
        {
            State_Disp_Mode = 0;
            Reach_Flag = 1;
						Time_3s = 0;//初始化3s计时器
        }
    }
}

/* ========== 速度计算任务（主循环，200ms）========== */
void Speed_Calc_Proc()
{		
		if(speed_slow_down<200) return;
		speed_slow_down=0;
    if (Speed_Update_Flag)
    {
        float Temp_Speed_10x;
        unsigned int Old_Speed_10x;
        float dx_f, dy_f, dist;
        Speed_Update_Flag = 0;

        Old_Speed_10x = Speed_10x;

        // 1. 速度公式
        Temp_Speed_10x = ((float)(PI * (Para_R_10x / 10.0) * freq) / 100.0f + Para_B) * 10;
        if (Temp_Speed_10x < 0)
            Speed_10x = 0;
        else
            Speed_10x = (unsigned int)Temp_Speed_10x;

        // 2. 运行态且速度实际变化时：锚定当前位置，重置时间，重算速度分量
        //    速度不变则不干预，避免无谓重置导致位置丢失
        if (State_Disp_Mode == 1 && Speed_10x != Old_Speed_10x)
        {
            Start_Location[0] = Current_Location[0];
            Start_Location[1] = Current_Location[1];
            Movement_Time = 0;
            cnt_movement_time = 0;

            dx_f = (float)Destination[0] - (float)Current_Location[0];
            dy_f = (float)Destination[1] - (float)Current_Location[1];
            dist = sqrt(dx_f * dx_f + dy_f * dy_f);
            if (dist > 0)
            {
                speed_x = ((float)Speed_10x / 10.0f) * dx_f / dist;
                speed_y = ((float)Speed_10x / 10.0f) * dy_f / dist;
            }
						else //如果已经到目的地，速度值置为0
						{
								speed_x=0;
								speed_y=0;
						}
        }
    }
}

/* ========== 按键处理 ========== */
void Key_Proc()
{	
		if(key_slow_down<10) return;
		key_slow_down=0;
    Key_Val = Key_Read();
    Key_Down = Key_Val & (Key_Val ^ Key_Old);
    Key_Up = ~Key_Val & (Key_Val ^ Key_Old);
    Key_Old = Key_Val;

    // S4：启动/暂停/恢复
    if (Key_Down == 4)
    {
        if (State_Disp_Mode == 0 && Gotit_Flag == 1)   // 空闲且有目标
        {
            float ts, dx_f, dy_f, dist;
            Gotit_Flag = 0;

            // 立即用当前 R/B/freq 刷新 Speed_10x，消除过时值
            ts = ((float)(PI * (Para_R_10x / 10.0) * freq) / 100.0f + Para_B) * 10;
            Speed_10x = (ts < 0) ? 0 : (unsigned int)ts;

            Start_Location[0] = Current_Location[0];
            Start_Location[1] = Current_Location[1];
            Movement_Time = 0;
            cnt_movement_time = 0;

            // 用最新 Speed_10x 计算速度分量
            dx_f = (float)Destination[0] - (float)Current_Location[0];
            dy_f = (float)Destination[1] - (float)Current_Location[1];
            dist = sqrt(dx_f * dx_f + dy_f * dy_f);
            if (dist > 0)
            {
                speed_x = ((float)Speed_10x / 10.0f) * dx_f / dist;
                speed_y = ((float)Speed_10x / 10.0f) * dy_f / dist;
            }
            else //如果已经到目的地，速度值置为0
            {
                speed_x = 0;
                speed_y = 0;
            }
            State_Disp_Mode = 1;                       // 启动
        }
        else if (State_Disp_Mode == 2 && Barrier_Distance >= 30) // 等待且障碍消失
        {
            float ts, dx_f, dy_f, dist;

            // 立即用当前 R/B/freq 刷新 Speed_10x
            ts = ((float)(PI * (Para_R_10x / 10.0) * freq) / 100.0f + Para_B) * 10;
            Speed_10x = (ts < 0) ? 0 : (unsigned int)ts;

            // 从等待恢复：以当前位置为新起点
            Start_Location[0] = Current_Location[0];
            Start_Location[1] = Current_Location[1];
            Movement_Time = 0;
            cnt_movement_time = 0;

            // 用最新 Speed_10x 重算速度分量
            dx_f = (float)Destination[0] - (float)Current_Location[0];
            dy_f = (float)Destination[1] - (float)Current_Location[1];
            dist = sqrt(dx_f * dx_f + dy_f * dy_f);
            if (dist > 0)
            {
                speed_x = ((float)Speed_10x / 10.0f) * dx_f / dist;
                speed_y = ((float)Speed_10x / 10.0f) * dy_f / dist;
            }
						else //如果已经到目的地dist==0，速度值置为0
						{
								speed_x=0;
								speed_y=0;
						}
            State_Disp_Mode = 1;
        }
        else if (State_Disp_Mode == 1)                 // 运行 → 等待
        {
            State_Disp_Mode = 2;
        }
    }

    // S5：重置当前位置（空闲态有效）
    if (Key_Down == 5)
    {
        if (State_Disp_Mode == 0)
        {
            Current_Location[0] = 0;
            Current_Location[1] = 0;
            // 注意：不改变 Gotit_Flag，不改变目的坐标
        }
    }

    // S8：界面切换
    if (Key_Down == 8)
    {
        switch (Seg_Disp_Mode)
        {
        case 0:
            Seg_Disp_Mode = 1;
            break;
        case 1:
            Para_CT_B = Para_B;              //进入参数界面前，把参数值赋值给设置值
            Para_R_CT_10x = Para_R_10x;
            Seg_Disp_Mode = 2;
            Para_Setting_Mode = 0;           // 默认选中R
            break;
        case 2:
            Para_B = Para_CT_B;              // 退出参数界面前把设置值赋值给参数值
            Para_R_10x = Para_R_CT_10x;
            Seg_Disp_Mode = 0;
            break;
        }
    }

    // 参数设置下的按键
    if (Seg_Disp_Mode == 2)
    {
        if (Key_Down == 9)                   // S9：切换R/B
            Para_Setting_Mode = !Para_Setting_Mode;

        if (Key_Down == 12)                 // S12：加
        {
            if (Para_Setting_Mode == 0)      // R
            {
                if (Para_R_CT_10x < 20) Para_R_CT_10x++;
            }
            else                            // B
            {
                if (Para_CT_B < 90) Para_CT_B += 5;
            }
        }

        if (Key_Down == 13)                 // S13：减
        {
            if (Para_Setting_Mode == 0)      // R
            {
                if (Para_R_CT_10x > 10) Para_R_CT_10x--;
            }
            else                            // B
            {
                if (Para_CT_B > -90) Para_CT_B -= 5;
            }
        }
    }
}

/* ========== 数码管显示 ========== */
void Seg_Proc()
{		
		
		if(seg_slow_down<80) return;
		seg_slow_down=0;

    switch (Seg_Disp_Mode)
    {
    case 0:  // 坐标界面
        Seg_Buf[0] = 11;           // L
        if (State_Disp_Mode == 0)  // 空闲 → 当前位置
        {
            Seg_Buf[1] = (Current_Location[0] >= 100) ? Current_Location[0] / 100 % 10 : 10;
            Seg_Buf[2] = (Current_Location[0] >= 10)  ? Current_Location[0] / 10 % 10 : 10;
            Seg_Buf[3] = Current_Location[0] % 10;
            Seg_Buf[4] = 14;       // 分隔符（-）
            Seg_Buf[5] = (Current_Location[1] >= 100) ? Current_Location[1] / 100 % 10 : 10;
            Seg_Buf[6] = (Current_Location[1] >= 10)  ? Current_Location[1] / 10 % 10 : 10;
            Seg_Buf[7] = Current_Location[1] % 10;
        }
        else                       // 运行/等待 → 目的坐标
        {
            Seg_Buf[1] = (Destination[0] >= 100) ? Destination[0] / 100 % 10 : 10;
            Seg_Buf[2] = (Destination[0] >= 10)  ? Destination[0] / 10 % 10 : 10;
            Seg_Buf[3] = Destination[0] % 10;
            Seg_Buf[4] = 14;
            Seg_Buf[5] = (Destination[1] >= 100) ? Destination[1] / 100 % 10 : 10;
            Seg_Buf[6] = (Destination[1] >= 10)  ? Destination[1] / 10 % 10 : 10;
            Seg_Buf[7] = Destination[1] % 10;
        }
        break;

    case 1:  // 速度/障碍界面
        Seg_Buf[0] = 12;           // E
        Seg_Buf[2] = 10;          // 熄灭
        if (State_Disp_Mode == 1)  // 运行 → 速度
        {
            Seg_Buf[1] = 1;        // 1
            Seg_Buf[3] = (Speed_10x >= 10000) ? Speed_10x / 10000 % 10 : 10;
            Seg_Buf[4] = (Speed_10x >= 1000)  ? Speed_10x / 1000 % 10 : 10;
            Seg_Buf[5] = (Speed_10x >= 100)   ? Speed_10x / 100 % 10 : 10;
            Seg_Buf[6] = Speed_10x / 10 % 10 + ',';
            Seg_Buf[7] = Speed_10x % 10;
        }
        else if (State_Disp_Mode == 0) // 空闲 → E2 -----
        {
            Seg_Buf[1] = 2;        // 2
            Seg_Buf[3] = 14;       // -
            Seg_Buf[4] = 14;
            Seg_Buf[5] = 14;
            Seg_Buf[6] = 14;
            Seg_Buf[7] = 14;
        }
        else                       // 等待 → 障碍距离
        {
            Seg_Buf[1] = 3;        // 3
            Seg_Buf[3] = (Barrier_Distance >= 10000) ? Barrier_Distance / 10000 % 10 : 10;
            Seg_Buf[4] = (Barrier_Distance >= 1000)  ? Barrier_Distance / 1000 % 10 : 10;
            Seg_Buf[5] = (Barrier_Distance >= 100)   ? Barrier_Distance / 100 % 10 : 10;
            Seg_Buf[6] = (Barrier_Distance >= 10)    ? Barrier_Distance / 10 % 10 : 10;
            Seg_Buf[7] = Barrier_Distance % 10;
        }
        break;

    case 2:  // 参数界面
        Seg_Buf[0] = 13;           // P
        Seg_Buf[1] = 10;
        Seg_Buf[2] = (Para_R_CT_10x / 10 % 10) + ','; // R十位 + 小数点
        Seg_Buf[3] = Para_R_CT_10x % 10;              // R个位
        Seg_Buf[4] = 10;          // 熄灭

        if (Para_CT_B > 0)
        {
            Seg_Buf[5] = 10;
            Seg_Buf[6] = (Para_CT_B >= 10) ? Para_CT_B / 10 % 10 : 10;
            Seg_Buf[7] = Para_CT_B % 10;
        }
        else if (Para_CT_B < 0)
        {
            if ((-Para_CT_B) >= 10)
            {
                Seg_Buf[5] = 14;   // -
                Seg_Buf[6] = (-Para_CT_B) / 10 % 10;
                Seg_Buf[7] = (-Para_CT_B) % 10;
            }
            else
            {
                Seg_Buf[5] = 10;
                Seg_Buf[6] = 14;   // -
                Seg_Buf[7] = (-Para_CT_B) % 10;
            }
        }
        else  // Para_CT_B == 0
        {
            Seg_Buf[5] = 10;
            Seg_Buf[6] = 10;
            Seg_Buf[7] = 0;
        }
        break;
    }
}

/* ========== LED 与继电器 ========== */
void Led_Proc()
{
    switch (State_Disp_Mode)
    {
    case 0:                 // 空闲
        ucled[0] = 0;
        ucled[1] = 0;
        Relay(0);
        break;
    case 1:                 // 运行
        ucled[0] = 1;
        ucled[1] = Dark_Work;   // 夜间亮灯
        Relay(1);
        break;
    case 2:                 // 等待
        ucled[0] = Led_Flash_Flag;
        ucled[1] = 0;
        Relay(0);
        break;
    }
    ucled[2] = L3_Flag;
    
}

void Get_AD()
{
		unsigned char temp_ad;
		if(ad_slow_down<100) return;
		ad_slow_down=0;
    temp_ad = Ad_Read(0x41);           // PCF8591 光敏电阻通道
    Dark_Work = (temp_ad <= 1.2 * 51); // 1.2V 阈值，低暗为1
}
/* ========== 超声波测距 ========== */
void Get_Distance()
{		
		if(us_slow_down<100) return;
		us_slow_down=0;
	
    Barrier_Distance = Ut_Wave_Data();
    if (Barrier_Distance == 0)
        Barrier_Distance = 255;

    if (State_Disp_Mode == 1 && Barrier_Distance <= 30)
        State_Disp_Mode = 2;           // 运行遇障 → 等待
}

/* ========== 串口通信（安全缓冲区格式）========== */
void Uart_Proc()
{		
		unsigned int x, y;
    int result,num;
    if (Uart_Rx_Index == 0) return;
    if (Uart_Rx_Tick >= 10)
    {
        Uart_Rx_Flag = 0;
        Uart_Rx_Tick = 0;

        // 坐标指令 (x,y)
        if (Uart_Rx_Buf[0] == '(' && Uart_Rx_Buf[Uart_Rx_Index - 1] == ')')
        {		
						result=sscanf(Uart_Rx_Buf, "(%u,%u)%n", &x, &y,&num);
            if (result==2&&strlen(Uart_Rx_Buf)==num)
            {
                if (State_Disp_Mode == 0)          // 空闲
                {
                    printf("Got it");
                    Gotit_Flag = 1;
                    Destination[0] = x;
                    Destination[1] = y; 
                }
                else if (State_Disp_Mode == 2)     // 等待
                {
                    printf("Wait");
                }
                else                               // 运行
                {
                    printf("Busy");
                }
            }
            else
            {
                printf("Error");
            }
        }
        // 查询状态 ?
        else if (strcmp(Uart_Rx_Buf, "?") == 0)
        {
            if (State_Disp_Mode == 0)
                printf("Idle");
            else if (State_Disp_Mode == 2)
                printf("Wait");
            else
                printf("Busy");
        }
        // 查询位置 #
        else if (strcmp(Uart_Rx_Buf, "#") == 0)
        {
            printf("(%u,%u)", Current_Location[0], Current_Location[1]);
        }
        else
        {
            printf("Error");
        }

        // 清空缓冲区
        memset(Uart_Rx_Buf, 0, 10);
        Uart_Rx_Index = 0;
    }
}

/* ========== 串口中断（安全缓冲区）========== */
void Uart1_Isr(void) interrupt 4
{
    if (RI)
    {
        Uart_Rx_Flag = 1;
        Uart_Rx_Tick = 0;
        if (Uart_Rx_Index < 10)
				{
            Uart_Rx_Buf[Uart_Rx_Index++] = SBUF;
				}
        else  // 溢出检查（仅在RI内部，TI不再触发）
        {
            Uart_Rx_Index = 0;
						Uart_Rx_Flag=0;
						Uart_Rx_Tick=0;
            memset(Uart_Rx_Buf, 0, 10);
        }
        RI = 0;
    }
}

/* ========== 定时器0（外部计数，测频率）========== */
void Timer0_Init(void)
{
    AUXR &= 0x7F;
    TMOD &= 0xF0;
    TMOD |= 0x05;        // 16位计数器
    TL0 = 0x00;
    TH0 = 0x00;
    TF0 = 0;
    TR0 = 1;
}

/* ========== 定时器1（1ms 系统时钟）========== */
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
		key_slow_down++;
		seg_slow_down++;
		ctrl_slow_down++;
		speed_slow_down++;
		us_slow_down++;
		ad_slow_down++;
    // 数码管动态扫描
    if (Seg_Buf[Seg_Pos] > 20)
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1);
    else
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);
    Seg_Pos = (++Seg_Pos) % 8;

    // 等待态 LED 闪烁 (0.1s)
    if (State_Disp_Mode == 2)
    {
        if (++Led_Flash_Tick == 100)
        {
            Led_Flash_Tick = 0;
            Led_Flash_Flag = !Led_Flash_Flag;
        }
    }
    else
    {
        Led_Flash_Tick = 0;
        Led_Flash_Flag = 0;
    }

    // 到达后 L3 亮3秒
    if (Reach_Flag)
    {
        L3_Flag = 1;
        if (++Time_3s == 3000)
        {
            Time_3s = 0;
            L3_Flag = 0;
            Reach_Flag = 0;
        }
    }

    // 运动时间累加：每100ms递增（仅运行态，纯整数操作）
    if (State_Disp_Mode == 1)
    {
        if (++cnt_movement_time == 100)
        {
            cnt_movement_time = 0;
            Movement_Time++;
        }
    }

    // 每秒读频率，设标志（浮点计算移到主循环）
    if (++Time_1S == 1000)
    {
        Time_1S = 0;
        freq = (TH0 << 8) | TL0;
        TH0 = TL0 = 0;
        Speed_Update_Flag = 1;
    }

    // 串口超时计时
    if (Uart_Rx_Flag) Uart_Rx_Tick++;
		Led_Disp(ucled);
}

/* ========== 主函数 ========== */
void main()
{
    System_Init();
		Ad_Read(0x41);//预读ad
    Timer0_Init();
    Uart1_Init();          // 波特率9600（配置在uart.c中）
    Timer1_Init();
    while (1)
    {
        Ctrl_Proc();
				Speed_Calc_Proc();
				Key_Proc();
				Seg_Proc();
				Led_Proc();
				Get_AD();
				Get_Distance();
				Uart_Proc();
    }
}
