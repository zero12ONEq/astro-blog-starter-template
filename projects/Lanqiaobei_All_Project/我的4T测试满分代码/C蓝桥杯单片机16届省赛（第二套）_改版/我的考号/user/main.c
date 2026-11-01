#include <STC15F2K60S2.H>
#include "led.h"
#include "init.h"
#include "key.h"
#include "seg.h"
#include "onewire.h"
#include "iic.h"
#include "ultrasound.h"

/* ============ 按键 ============ */
idata unsigned char key_val, key_old, key_down, key_up;
idata unsigned char key_slow_down;

/* ============ 数码管 ============ */
idata unsigned char seg_pos;
pdata unsigned char seg_buf[] = {10,10,10,10,10,10,10,10};
idata unsigned char seg_slow_down;

/* ============ LED ============ */
pdata unsigned char ucled[] = {0,0,0,0,0,0,0,0};
idata unsigned char led_flash_tick;
bit led_flash_flag = 0;

/* ============ 界面模式 ============ */
idata unsigned char seg_disp_mode = 0;    // 0-测距界面 1-参数设置界面
idata unsigned char range_show_mode = 0;  // 0-初始/清除 1-单次 2-累加
idata unsigned char para_disp_mode = 0;   // 0-时间参数 1-次数参数

/* ============ 温度与声速 ============ */
idata unsigned char T_val;
idata unsigned int T_slow_down;
idata float speed = 340;

/* ============ 测距 ============ */
idata unsigned int dist_val;       // 单次测量结果
idata unsigned int dist_sum;       // 累加测量累计结果
bit signal_flag = 0;               // 单次测量待执行
bit summation_flag = 0;            // 累加测量执行中

/* ============ 参数 ============ */
pdata unsigned char time_apart[4] = {2, 4, 6, 4};
idata unsigned char time_apart_index = 0;
pdata unsigned char dist_count[4] = {3, 5, 7, 5};
idata unsigned char dist_count_index = 0;
idata unsigned char summation_index = 0;  // 累加已完成次数

/* ============ 累加测量定时 ============ */
idata unsigned int dist_test_timer;

/* ================================================
 *  测距执行（单次/累加共用）
 *  被 key_proc 和 dist_proc 调用
 * ================================================ */
void do_distance()
{
	unsigned int temp;
	if(signal_flag)
	{
		temp = ut_wave_data(speed);
		dist_val = temp;
		signal_flag = 0;  // 单次测量完成
	}
	else if(summation_flag)
	{
		temp = ut_wave_data(speed);
		dist_sum += temp;
		summation_index++;
		if(summation_index >= dist_count[dist_count_index])
		{
			summation_flag = 0;   // 累加测量全部完成
			summation_index = 0;
		}
	}
}

/* ================================================
 *  按键处理
 * ================================================ */
void key_proc()
{
	if(key_slow_down < 10) return;
	key_slow_down = 0;

	key_val = key_read();
	key_down = key_val & (key_val ^ key_old);
	key_up = ~key_val & (key_val ^ key_old);
	key_old = key_val;

	/* --- 累加测量执行中: S8/S12 无效 --- */
	if(summation_flag == 0)
	{
		// S8: 界面切换
		if(key_down == 8)
		{
			if(seg_disp_mode == 0)
			{
				seg_disp_mode = 1;
				para_disp_mode = 0;
			}
			else
			{
				if(para_disp_mode == 0)
					para_disp_mode = 1;
				else
					seg_disp_mode = 0;
			}
		}
		// S12: 清除（仅测距界面）
		if(seg_disp_mode == 0 && key_down == 12)
		{
			dist_val = 0;
			dist_sum = 0;
			range_show_mode = 0;  // 回到初始显示
		}
	}

	/* --- 测距界面下: S4/S5 --- */
	if(seg_disp_mode == 0)
	{
		// S4: 单次测量（可立即中断累加）
		if(key_down == 4)
		{
			signal_flag = 1;
			summation_flag = 0;
			summation_index = 0;
			dist_val = 0;
			dist_sum = 0;
			range_show_mode = 1;
			do_distance();
		}
		// S5: 累加测量
		if(key_down == 5)
		{
			summation_flag = 1;
			signal_flag = 0;
			summation_index = 0;
			dist_val = 0;
			dist_sum = 0;
			dist_test_timer = 0;
			range_show_mode = 2;
			do_distance();  // 立即执行首次测距
		}
	}

	/* --- 参数设置界面: S9 --- */
	if(seg_disp_mode == 1 && key_down == 9)
	{
		if(para_disp_mode == 0)
			time_apart_index = (++time_apart_index) % 4;
		else
			dist_count_index = (++dist_count_index) % 4;
	}
}

/* ================================================
 *  数码管显示
 * ================================================ */
void seg_proc()
{
	if(seg_slow_down < 80) return;
	seg_slow_down = 0;

	switch(seg_disp_mode)
	{
		case 0: // 测距界面
			seg_buf[1] = 12; // C
			seg_buf[2] = T_val / 10 % 10;
			seg_buf[3] = T_val % 10;
			switch(range_show_mode)
			{
				case 0: // 初始/清除 -> 熄灭
					seg_buf[0] = 12; // C
					seg_buf[4] = 10;
					seg_buf[5] = 10;
					seg_buf[6] = 10;
					seg_buf[7] = 10;
					break;
				case 1: // 单次 -> A
					seg_buf[0] = 11;
					seg_buf[4] = (dist_val >= 1000) ? dist_val/1000%10 : 10;
					seg_buf[5] = (dist_val >= 100)  ? dist_val/100%10  : 10;
					seg_buf[6] = (dist_val >= 10)   ? dist_val/10%10   : 10;
					seg_buf[7] = dist_val % 10;
					break;
				case 2: // 累加 -> E
					seg_buf[0] = 13;
					seg_buf[4] = (dist_sum >= 1000) ? dist_sum/1000%10 : 10;
					seg_buf[5] = (dist_sum >= 100)  ? dist_sum/100%10  : 10;
					seg_buf[6] = (dist_sum >= 10)   ? dist_sum/10%10   : 10;
					seg_buf[7] = dist_sum % 10;
					break;
			}
			break;

		case 1: // 参数设置界面
			seg_buf[0] = 14; // P
			seg_buf[2] = 10;
			seg_buf[3] = 10;
			seg_buf[4] = 10;
			seg_buf[5] = 10;
			seg_buf[6] = 0;  // 高位补0
			if(para_disp_mode == 0)
			{
				seg_buf[1] = 1;
				seg_buf[7] = time_apart[time_apart_index];
			}
			else
			{
				seg_buf[1] = 2;
				seg_buf[7] = dist_count[dist_count_index];
			}
			break;
	}
}

/* ================================================
 *  LED 指示
 * ================================================ */
void led_proc()
{
	// L1: 测距界面点亮
	ucled[0] = (seg_disp_mode == 0);

	// L2: 结果超范围指示
	switch(range_show_mode)
	{
		case 0: // 初始/清除
			ucled[1] = 0;
			break;
		case 1: // 单次完成后判断
			if(signal_flag == 0)
				ucled[1] = (dist_val < 20 || dist_val > 100);
			break;
		case 2: // 累加完成后判断，执行中状态不变
			if(summation_flag == 0)
				ucled[1] = (dist_sum < 20 || dist_sum > 100);
			break;
	}

	// L3: 由中断控制闪烁标志，这里直接赋值
	ucled[2] = led_flash_flag;

}

/* ================================================
 *  温度采集
 * ================================================ */
void get_temperature()
{
	float temp;
	if(T_slow_down < 300) return;
	T_slow_down = 0;
	temp = rd_temperature();
	T_val = (unsigned char)temp;
	speed = 330 + 0.6 * temp;
}

/* ================================================
 *  累加测量间隔定时触发
 * ================================================ */
void dist_proc()
{
	if(summation_flag &&
	   dist_test_timer >= (unsigned int)time_apart[time_apart_index] * 1000)
	{
		dist_test_timer = 0;
		do_distance();
	}
}

/* ================================================
 *  DAC 输出
 * ================================================ */
void da_proc()
{
	float temp = 0;
	switch(range_show_mode)
	{
		case 0: // 未完成有效测量 -> 0V
			temp = 0;
			break;
		case 1: // 单次测量完成后更新
			if(signal_flag == 0)
			{
				if(dist_val <= 20)       temp = 1;
				else if(dist_val >= 100) temp = 5;
				else                     temp = (float)dist_val / 20.0;
			}
			break;
		case 2: // 累加测量完成后更新
			if(summation_flag == 0)
			{
				if(dist_sum <= 20)       temp = 1;
				else if(dist_sum >= 100) temp = 5;
				else                     temp = (float)dist_sum / 20.0;
			}
			break;
	}
	da_write((unsigned char)(temp * 51));
}

/* ================================================
 *  定时器1: 1ms @ 12.000MHz
 * ================================================ */
void Timer1_Init(void)
{
	AUXR &= 0xBF;  // 定时器时钟12T模式
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
	T_slow_down++;

	// 数码管扫描
	seg_disp(seg_pos, seg_buf[seg_pos]);
	seg_pos = (++seg_pos) % 8;

	// 累加测量: 计时 + L3闪烁
	if(summation_flag)
	{
		dist_test_timer++;
		if(++led_flash_tick >= 100)
		{
			led_flash_tick = 0;
			led_flash_flag = !led_flash_flag;
		}
	}
	else
	{
		dist_test_timer = 0;
		led_flash_tick = 0;
		led_flash_flag = 0;
	}

	led_disp(ucled);
}

/* ================================================
 *  主函数
 * ================================================ */
void main()
{
	system_init();
	Timer1_Init();
	while(1)
	{
		key_proc();
		seg_proc();
		led_proc();
		get_temperature();
		dist_proc();
		da_proc();
	}
}
