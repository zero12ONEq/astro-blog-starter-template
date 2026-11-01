/* ===================================头文件声明==================================== */
#include <STC15F2K60S2.H>
#include "Led.h"
#include "Init.h"
#include "Key.h"
#include "Seg.h"
#include "Ds1302.h"
#include "Ds18b20.h"
#include "I2C.h"
#include <stdio.h>
#include <string.h>
#include "Uart.h"

// 车辆记录条目结构体
typedef struct Car
{
  char type; // 车辆类型(A,B,C,D)
  int num; // 车辆编号(0~999)
  unsigned char location; // 车辆的车位(0~99)
  unsigned char time[3]; // 入库时间
} Car;

#define CMD_MAX_NUM 4 // 最大接收指令数量
#define UART_RX_SIZE CMD_MAX_NUM * 16// 串口最大接收字符数量
#define CARPORT_SIZE 100 // 车库最大容量（不能超过 100）

/* ====================================变量定义==================================== */
/* 核心变量 */
data unsigned char key_Val, key_Down, key_Up, key_Old, key_Scan_Slow;
data unsigned char seg_Scan_Slow, seg_Pos, seg_Buf[] = {10, 10, 10, 10, 10, 10, 10, 10};
data unsigned char led_Buf[] = {0, 0, 0, 0, 0, 0, 0, 0};
data unsigned char uart_recv_tick, uart_rx_data[UART_RX_SIZE], uart_rx_data_index;

/* 状态标志 */
unsigned char seg_display_mode = 0; // 0-熄灭，1-入库信息界面，2-出库信息界面，3-费率管理界面，4-车库查询界面，5-密码修改界面

bit input_car_flag; // 入库是否成功

bit output_car_flag; // 出库是否成功

bit illumination_flag; // 是否存在光照

bit seg_flag = 1; // 数码管闪烁

bit true_flag; // 密码是否修改成功

bit seg_lock_flag; // 锁定数码管显示上一次记录的信息

/* 相关变量 */
unsigned char i, j, n; // 公共循环变量
unsigned char rtc_time[3] = {0, 0, 0}; // DS1302 实时时钟

unsigned char carport_num = 100; // 目前剩余车位
Car now_car; // 当前入库/出库的车辆
Car first_now_cat; // 当前指令中第一次的车辆
xdata Car carport[CARPORT_SIZE]; // 车库数组
unsigned char carport_index = 0; // 车库记录指针，指向最近入库的条目的下一个空元素

unsigned int money_x10; // 停车费用 * 10 
unsigned int stop_rate_x100[4] = {1000, 2000, 3000, 4000}; // 四种类型车辆的费率（秒） * 100
unsigned char now_set_rate[4]; // 当前正在修改的费率，在保存之前不影响实际费率，对应四位
bit set_rate_choose; // 费率设置界面选择的单元格：0-车辆类型，1-停车费用
unsigned char set_rate_index; // 费率设置界面指针
unsigned char set_rate_bit_index = 0; // 设置费率每一位的指针（0 ~ 3）=（千位~个位）

unsigned char query_index; // 车库查询界面定位指针

float temperature; // 当前温度值

xdata char password[12] = "Admin.."; // 6~12 位的密码，默认为 Admin..

unsigned char rb2_num; // 0-255 对应数字 0-9 等比划分，25.6 ~~ 26

unsigned char key7_down_count; // S7 短按计数

xdata unsigned char seg_Buf_old[8] = {10, 10, 10, 10, 10, 10, 10, 10}; //存储上次数码管显示的信息

/* 计时变量 */
unsigned int seg_mode1or2_timer5000ms; // 模式 1/ 模式 2显示 5s 后熄灭
unsigned int key4_down_timer1000ms; // S4 按下超过 1s 后松手切换到车库查询界面
unsigned int key6_down_timer1000ms; // S6 长按超过 1s 后自动退出费率管理界面
unsigned int key7_down_timer1000ms; // S7 长按超过 1s 后自动退出密码修改界面
unsigned char seg_timer250ms; // 密码修改界面的所有以及费率界面下选中的单元格需要以 0.5s/次闪烁即以 250ms 为间隔闪烁
unsigned int buzz_timer3000ms; // 蜂鸣器使能 3s 后关闭并退出密码修改界面
unsigned int timer1000ms; // 超过一秒没有再次按下 S7 便清零 S7 的短按计数

/* ====================================自定义函数==================================== */
// 计算时间差（秒）
int timeDifference(unsigned char* timeA, unsigned char* timeB) {
    // 将 A 和 B 的时间转换为总秒数
    int totalSecondsA = timeA[0] * 3600 + timeA[1] * 60 + timeA[2];
    int totalSecondsB = timeB[0] * 3600 + timeB[1] * 60 + timeB[2];

    // 计算时间差
    int  diff = totalSecondsB - totalSecondsA;

    // 处理跨天情况（如果 B < A，则 B 是第二天的时间）
    if (diff < 0) {
        diff += 24 * 3600; // 加 1 天的秒数
    }

    return diff;
}

// 根据 RB2 的 ADC 0-255 对应数字 0-9 等比划分，25.6 ~~ 26
unsigned Get_AD_num(unsigned char rb2_num)
{
  if(rb2_num < 26) return 0;
  else if(rb2_num < 52) return 1;
  else if(rb2_num < 78) return 2;
  else if(rb2_num < 104) return 3;
  else if(rb2_num < 130) return 4;
  else if(rb2_num < 156) return 5;
  else if(rb2_num < 182) return 6;
  else if(rb2_num < 208) return 7;
  else if(rb2_num < 234) return 8;
  else 
    return 9;
}

// 判断密码是否满足 6-12 位长度 需要包含大小写以及特殊字符
bit Inspect_Password(char *password)
{
  bit is_len, is_daxie, is_xiaoxie, is_teshu; // 长度，大写字母，小写字母， 特殊字符的标志位
  
  // 判断是否符合长度，长度不足直接退出
  if(strlen(password) >= 6 && strlen(password) <= 12)
    is_len = 1;
  else
    return 0;
  
  for(i = 0; password[i] != '\0'; i++)
  {
    if((password[i] >= 48 && password[i] <= 57) || password[i] <= 32 || password[i] == 127) // 包含有数字或操作符，直接退出
      return 0;
    else if(password[i] >= 65 && password[i] <= 90) // 是否有大写字母
      is_daxie = 1;
    else if(password[i] >= 97 && password[i] <= 122) // 是否有小写字母
      is_xiaoxie = 1;
    else
      is_teshu = 1;
  }
  
  return (is_len & is_daxie & is_xiaoxie & is_teshu);
}

// 记录当前数码管（非熄灭）显示的信息
void Record_Seg(unsigned char* now_seg_Buf)
{
  unsigned char temp_i;
  if(now_seg_Buf[0] != 10)
  {
    for(temp_i = 0; temp_i < 8; temp_i++)
      seg_Buf_old[temp_i] = now_seg_Buf[temp_i];
  }
}

/* ====================================任务函数==================================== */
/* 按键处理函数 */
void Key_Proc()
{
	if(key_Scan_Slow) return;
	key_Scan_Slow = 1;
	
	key_Val = Key_Read();
	key_Down = key_Val & (key_Val ^ key_Old);
	key_Up = ~key_Val & (key_Val ^ key_Old);
	key_Old = key_Val;
  
  // 长按 S4 超过 1s 后抬起，并重置查询界面指针为末尾记录条目（时间最近）
  if(key_Up == 4 && key4_down_timer1000ms > 1000)
  {
    query_index = carport_index - 1;
    seg_display_mode = 4;
    seg_lock_flag = 0;
    Record_Seg(seg_Buf);
  }
  
  // 长按 S6 没有达到 1s 便抬起视为短按
  if(key_Up == 6 && seg_display_mode == 3 && key6_down_timer1000ms != 1000)
  {
    seg_lock_flag = 0;
    Record_Seg(seg_Buf);
    if(set_rate_choose == 0) // 切换车辆类型
    {
      if(++set_rate_index == 4) set_rate_index = 0;
      set_rate_bit_index = 0;
      now_set_rate[0] = stop_rate_x100[set_rate_index] / 1000 % 10;
      now_set_rate[1] = stop_rate_x100[set_rate_index] / 100 % 10;
      now_set_rate[2] = stop_rate_x100[set_rate_index] / 10 % 10;
      now_set_rate[3] = stop_rate_x100[set_rate_index] % 10;
    }
    else // 调整对应车辆类型的费率
    {
      if(now_set_rate[0] != 5) // 千位为 5 时为最大的，后续必须为 0
        now_set_rate[set_rate_bit_index] = rb2_num;
      else
        now_set_rate[set_rate_bit_index] = 0;
      
      if(set_rate_bit_index == 0 && rb2_num > 5) // 千位
        now_set_rate[set_rate_bit_index] = 5;

      // 最后一位设置完后保存
      if(++set_rate_bit_index == 4) 
      {
        stop_rate_x100[set_rate_index] = now_set_rate[0] * 1000 + now_set_rate[1] * 100 + now_set_rate[2] * 10 + now_set_rate[3];
        set_rate_bit_index = 0;
        set_rate_choose = 0;
      }
    }
  }
  
  // 长按 S7 没有达到 1s 便抬起视为短按
  if(key_Up == 7 && seg_display_mode != 5 && key7_down_timer1000ms != 1000)
  {
    key7_down_count++;
    timer1000ms = 0;
    if(key7_down_count == 3)
    {
      seg_lock_flag = 0;
      seg_display_mode = 5;
      Record_Seg(seg_Buf);
    }
  }
	switch(key_Down)
	{
    case 4: // 车库查询
      key4_down_timer1000ms = 0;
      break;
    case 5: // 车库查询切换，参数切换
      if(seg_display_mode == 4 && --query_index == 255)
        query_index = carport_index - 1;
      else if(seg_display_mode == 3) // 切换选中的单元格
        set_rate_choose ^= 1;
      break;
    case 6:
      key6_down_timer1000ms = 0;
      break;
    case 7:
      key7_down_timer1000ms = 0;
      break;
    case 8: // 显示上一次记录的信息
      if(seg_display_mode != 5 && seg_Buf_old[0] != 10)
        seg_lock_flag = 1;
      break;
	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	/* 信息读取处理区域 */
  switch(seg_Scan_Slow)
  {
    case 10:
      Read_Rtc(rtc_time);
      break;
    case 30:
      rb2_num = Get_AD_num(AD_Read(1));
      break;
    case 60:
      temperature = Read_Temperature();
      break;
    case 90:
      illumination_flag = (AD_Read(3) > 100); 
      break;
  }

	/* 数据显示区域 */
  if(seg_Scan_Slow) return;
	seg_Scan_Slow = 1;
  
  if(seg_lock_flag == 1) // 锁定显示上一次记录的信息
  {
    seg_Buf[0] = seg_Buf_old[0];
    seg_Buf[1] = seg_Buf_old[1];
    seg_Buf[2] = seg_Buf_old[2];
    seg_Buf[3] = seg_Buf_old[3];
    seg_Buf[4] = seg_Buf_old[4];
    seg_Buf[5] = seg_Buf_old[5];
    seg_Buf[6] = seg_Buf_old[6];
    seg_Buf[7] = seg_Buf_old[7];
  }
  else
  {
    switch(seg_display_mode)
    {
      case 0: // 熄灭
        seg_Buf[0] = seg_Buf[1] = seg_Buf[2] = seg_Buf[3] = seg_Buf[4] = seg_Buf[5] = seg_Buf[6] = seg_Buf[7] = 10;
      break;
      case 1: // 入库信息界面
        seg_Buf[0] = now_car.type - 54;
        seg_Buf[1] = 13; // b
        seg_Buf[2] = 11; // A
        seg_Buf[3] = 15; // r
        seg_Buf[4] = now_car.location / 10 % 10;
        seg_Buf[5] = now_car.location % 10;
        seg_Buf[6] = carport_num / 10 % 10;
        seg_Buf[7] = carport_num % 10;
        break;
      case 2: // 出库信息界面
        seg_Buf[0] = now_car.type - 54;
        seg_Buf[1] = now_car.num / 100 % 10;
        seg_Buf[2] = now_car.num / 10 % 10;
        seg_Buf[3] = now_car.num % 10;
        seg_Buf[4] = money_x10 / 1000 % 10;
        seg_Buf[5] = money_x10 / 100 % 10;
        seg_Buf[6] = money_x10 / 10 % 10 + '.';
        seg_Buf[7] = money_x10 % 10;
        break;
      case 3: // 费率管理界面
        seg_Buf[0] = 10 + set_rate_index + 1;
        seg_Buf[1] = 13; // b
        seg_Buf[2] = 11; // A
        seg_Buf[3] = 15; // r
        seg_Buf[4] = now_set_rate[0];
        seg_Buf[5] = now_set_rate[1] + '.';
        seg_Buf[6] = now_set_rate[2];
        seg_Buf[7] = now_set_rate[3];
      
        seg_Buf[0 + set_rate_choose * 4] = seg_flag ? seg_Buf[0 + set_rate_choose * 4] : 10;
        seg_Buf[1 + set_rate_choose * 4] = seg_flag ? seg_Buf[1 + set_rate_choose * 4] : 10;
        seg_Buf[2 + set_rate_choose * 4] = seg_flag ? seg_Buf[2 + set_rate_choose * 4] : 10;
        seg_Buf[3 + set_rate_choose * 4] = seg_flag ? seg_Buf[3 + set_rate_choose * 4] : 10;
        break;
      case 4: // 车库查询界面
        seg_Buf[0] = carport[query_index].type - 54;
        seg_Buf[1] = carport[query_index].num / 100 % 10;
        seg_Buf[2] = carport[query_index].num / 10 % 10;
        seg_Buf[3] = carport[query_index].num % 10;
        seg_Buf[4] = carport[query_index].time[1] / 10 % 10;
        seg_Buf[5] = carport[query_index].time[1] % 10 + '.';
        seg_Buf[6] = carport[query_index].time[2] / 10 % 10;
        seg_Buf[7] = carport[query_index].time[2] % 10;
        break;
      case 5: // 密码修改界面
        seg_Buf[0] = seg_flag ? 16 : 10;
        seg_Buf[1] = seg_flag ? 16 : 10;
        seg_Buf[2] = seg_flag ? 16 : 10;
        seg_Buf[3] = seg_flag ? 16 : 10;
        seg_Buf[4] = seg_flag ? 16 : 10;
        seg_Buf[5] = seg_flag ? 16 : 10;
        seg_Buf[6] = seg_flag ? 16 : 10;
        seg_Buf[7] = 10;
        break;
    }
  }
}

/* 其他处理函数 */
void Led_Proc()
{

}

/* 定时器 1 初始化函数 */
void Timer1_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	EA = 1;
	ET1 = 1;
}

/* 定时器 1 中断处理函数 */
void Timer1_Server() interrupt 3
{
	if(++key_Scan_Slow == 10) key_Scan_Slow = 0;
	if(++seg_Scan_Slow == 100) seg_Scan_Slow = 0;
  
  // 模式 1/ 模式 2 停留 5s 后熄灭
  if((seg_display_mode == 1 || seg_display_mode == 2)&& ++seg_mode1or2_timer5000ms == 5000)
  {
    seg_display_mode = 0;
    Record_Seg(seg_Buf);
  }
  
  // 车辆有记录的情况下，按下 S4 超过 1s 后将时间变量控制在 1100（防止变量溢出）
  if(carport_index != 0 && key_Old == 4 && ++key4_down_timer1000ms >= 1100)
    key4_down_timer1000ms = 1100;
  
  // 长按 S6 超过 1s 自动退出费率管理界面
  if(seg_display_mode == 3 && key_Old == 6 && ++key6_down_timer1000ms == 1000)
  {
    seg_display_mode = 0;
    Record_Seg(seg_Buf);
  }
  
  // 长按 S7 超过 1s 自动退出密码修改界面
  if(seg_display_mode == 5 && key_Old == 7 && ++key7_down_timer1000ms == 1000)
  {
    seg_display_mode = 0;
    Record_Seg(seg_Buf);
  }
  
  // 超过 1s 没有再次按下 S7 则清零短按计数
  if(++timer1000ms >= 1000)
  {
    timer1000ms = 1000;
    key7_down_count = 0;
  }
  
  // 密码修改界面下以及费率界面下选中的单元格需要以 250ms 为间隔闪烁
  if((seg_display_mode == 3 || seg_display_mode == 5) && ++seg_timer250ms == 250)
  {
    seg_timer250ms = 0;
    seg_flag ^= 1;
  }
  
  // 密码修改成功时，蜂鸣器使能 3s 后关闭，并熄灭数码管
  if(true_flag == 1 && ++buzz_timer3000ms == 3000)
  {
    buzz_timer3000ms = 0;
    seg_display_mode = 0;
    Record_Seg(seg_Buf);
    true_flag = 0;
    Buzz(0);
  }
	
	if(++seg_Pos == 8) seg_Pos = 0;
	if(seg_Buf[seg_Pos] > 20) // 判断有无小数点，ASCII 为 46
		Seg_Display(seg_Pos, seg_Buf[seg_Pos] - '.', 1);
	else
		Seg_Display(seg_Pos, seg_Buf[seg_Pos], 0);
	
	Led_Display(led_Buf, 1);
	uart_recv_tick++;		
}

/* 串口 1 中断服务函数 */
void Uart1_Server(void) interrupt 4
{
	if (RI)				//检测串口1接收中断
	{
		uart_recv_tick = 0;
		uart_rx_data[uart_rx_data_index] = SBUF;
		RI = 0;			//清除串口1接收中断请求位
		if(++uart_rx_data_index == UART_RX_SIZE) uart_rx_data_index = 0;
	}
}

/* 串口处理函数 */
void Uart_Proc()
{
  int temp_int;
  char temp_char;
  char temp_str[16];
  char *token1, *token2, *token3;
  char *command_str;
  
	if(uart_rx_data_index == 0) return; // 没有数据则退出
	if(uart_recv_tick >= 10) // 超时解析
	{
		uart_recv_tick = 0;
		/* 串口数据解析区域 */
    command_str = strtok(uart_rx_data, "-");
    for(n = 0; command_str != NULL && i < CMD_MAX_NUM; n++)
    {
      if(seg_display_mode != 5 && sscanf(command_str, "%cCAR:%1s%d:CAR%d", &now_car.type, &temp_char, &now_car.num, &temp_int) == 4) // 指令 1
      {
        // 判断输入是否合法
        if(now_car.type >= 65 && now_car.type <= 68 && now_car.type == temp_char && now_car.num >= 0 && now_car.num <= 999 && temp_int >= 0 && temp_int <= 99)
        {
          now_car.location = (unsigned char)temp_int;
          if(illumination_flag == 1) // 入库
          {
            input_car_flag = 1;
            // 遍历车库，判断编号是否唯一以及车位是否被占用
            for(i = 0; i < carport_index; i++)
            {
              if(carport[i].type != 0 && ((carport[i].type ==  now_car.type && carport[i].num == now_car.num) || carport[i].location == now_car.location))
              {
                input_car_flag = 0;
                break;
              }
            }
            
            if(carport_index != CARPORT_SIZE && input_car_flag)
            {
              now_car.time[0] = rtc_time[0];
              now_car.time[1] = rtc_time[1];
              now_car.time[2] = rtc_time[2];
              printf("[%cCAR:%c%03d:CAR%d:%d-%d-%d]", now_car.type, now_car.type, now_car.num, (int)now_car.location, (int)now_car.time[0], (int)now_car.time[1], (int)now_car.time[2]);
              carport[carport_index++] = now_car;
              carport_num--;
              seg_mode1or2_timer5000ms = 0;
              seg_display_mode = 1;
            }
          }
          else // 出库
          {
            output_car_flag = 0;
            // 遍历车库，判断是否存在编号和车位对应的车辆记录
            for(i = 0; i < carport_index; i++)
            {
              if(carport[i].type != 0 && carport[i].num == now_car.num && carport[i].location == now_car.location)
              {
                // 将出库的记录左移覆盖，保持车库记录指针，始终指向最近入库的条目的下一个空元素
                if(carport_index == 1)
                  carport[i].type = 0; // 类型变为 0，即标记为删除
                else
                {
                  for(j = i; j < carport_index - 1; j++)
                    carport[j] = carport[j + 1];
                }
                carport_index--;
                output_car_flag = 1;
                break;
              }
            }
            
            if(output_car_flag)
            {
              temp_int = timeDifference(now_car.time, rtc_time);
              money_x10 = (stop_rate_x100[(now_car.type - 54) % 10 - 1] / 100.0 * temp_int + 0.05) * 10;
              printf("[%cCAR:%c%03d:CAR%d:%d:%.1f]", now_car.type, now_car.type, now_car.num, (int)now_car.location, temp_int, money_x10 / 10.0);
              carport_num++;
              seg_mode1or2_timer5000ms = 0;
              seg_display_mode = 2;
            }
          }
          
        }
      }
      // 保存第一条记录
      seg_lock_flag = 0;
      Record_Seg(seg_Buf);
      if(n == 0) first_now_cat = now_car;
      
      command_str = strtok(NULL, "-");
    }
    now_car = first_now_cat; // 显示第一条指令的信息
    
    if(seg_display_mode == 4 && sscanf(uart_rx_data, "Find:[%c]", &temp_char) == 1) // 指令 3，搜索车库中的对应条目
    {
      
      if(temp_char == '1' && sscanf(uart_rx_data, "Find:[%c][%cCAR]", &temp_char, &temp_char, &temp_int) == 2 && temp_char >= 65 && temp_char <= 68 && temp_int == (int)strlen(uart_rx_data))
      {
        // 遍历车库，寻找相同类型的车辆记录（由近到远）
        for(i = carport_index - 1; i < 250; i--)
        {
          if(carport[i].type != 0 && carport[i].type == temp_char)
          {
            query_index = i; // 切换到查询条目
            printf("[%cCAR:%c%03d:CAR%d:%d-%d-%d:%.2f]", carport[i].type, carport[i].type, carport[i].num, (int)carport[i].location, (int)carport[i].time[0], (int)carport[i].time[1], (int)carport[i].time[2], temperature);
          }
        }
      }
      else if(temp_char == '2' && sscanf(uart_rx_data, "Find:[%c][%1s%d]", &temp_char, &temp_char, &temp_int) == 3 && temp_char >= 65 && temp_char <= 68)
      {
        // 遍历车库，寻找相同编号的车辆记录，由于编号的唯一性，所以找到后可以直接跳出循环
        for(i = 0; i < carport_index; i++)
        {
          if(carport[i].type != 0 && carport[i].type == temp_char && carport[i].num == temp_int)
          {
            query_index = i; // 切换到查询条目
            printf("[%cCAR:%c%03d:CAR%d:%d-%d-%d:%.2f]", carport[i].type, carport[i].type, carport[i].num, (int)carport[i].location, (int)carport[i].time[0], (int)carport[i].time[1], (int)carport[i].time[2], temperature);
            break;
          }
        }
      }
    }
    else if(seg_display_mode != 5 && seg_display_mode != 3 && sscanf(uart_rx_data, "Password:[%s]", temp_str) == 1) // 输入正确的密码进入费率界面
    {
      temp_str[strlen(temp_str) - 1] = '\0'; // 去除结尾多余的 ] 字符
      // 判断密码输入是否成功，成功则进入费率界面
      if(strcmp(temp_str, password) == 0)
      {
        set_rate_bit_index = 0;
        set_rate_choose = 0;
        set_rate_index = 0;
        now_set_rate[0] = stop_rate_x100[set_rate_index] / 1000 % 10;
        now_set_rate[1] = stop_rate_x100[set_rate_index] / 100 % 10;
        now_set_rate[2] = stop_rate_x100[set_rate_index] / 10 % 10;
        now_set_rate[3] = stop_rate_x100[set_rate_index] % 10;
        seg_display_mode = 3;
        seg_lock_flag = 0;
        Record_Seg(seg_Buf);
      }
    }
    else if(seg_display_mode == 5) // 指令 2，修改密码
    {
      token1 = strtok(uart_rx_data, "[]"); // 指令符
      token2 = strtok(NULL, "[]"); // 旧密码
      token3 = strtok(NULL, "[]"); // 新密码
      if(strcmp(token1, "Password:") == 0 && strcmp(token2, password) == 0 && Inspect_Password(token3) == 1)
      {
        if(strlen(token3) <= 8)
        {
          memcpy(password, token3, strlen(token3));
          EEPROM_Write(password, 0, strlen(password));
        }
        else
        {
          memcpy(password, token3, strlen(token3));
          EEPROM_Write(password, 0, 8);
          EEPROM_Write(&password[8], 8, strlen(password) - 8);
        }
        
        i = strlen(password);
        EEPROM_Write(&i, 16, 1);
        
        true_flag = 1;
        buzz_timer3000ms = 0;
        Buzz(1);
      }
    }
   
		memset(uart_rx_data, 0, uart_rx_data_index); // 清空接收数据
		uart_rx_data_index = 0;
	}
}

/* ====================================主循环函数=================================== */
void main(void)
{
//  char temp_str1[6];
	/* 初始化区域 */
	System_Init();
  while(Read_Temperature() == 85);
// 上电读取保存在 eeprom 中的密码
//  EEPROM_Read(&i, 16, 1);
//  if(i <= 8)
//    EEPROM_Read(password, 0, i);
//  else
//  {
//    EEPROM_Read(password, 0, 8);
//    EEPROM_Read(temp_str1, 8, i - 8);
//    
//    for(j = 0; j < i - 8; j++)
//      password[8 + j] = temp_str1[j];
//  }
  Set_Rtc(rtc_time);
	Uart1_Init();
	Timer1_Init();

  
	/* 主循环 */
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Uart_Proc(); 
	}
}
