#include "ultrasound.h"
#include "intrins.h"
sbit US_TX=P1^0;//发射引脚
sbit US_RX=P1^1;//接收引脚
//void Delay12us()		//@12.000MHz 如果下方的那个延时测不准，可以用这个
//{
//	unsigned char i;

//	_nop_();
//	_nop_();
//	i = 33;//38
//	while (--i);
//}
void Delay12us()		//@12.000MHz 
{
	unsigned char i;

	_nop_();
	i = 3;
	while (--i);
}
//发送8个40kHz方波的函数
void Ut_Wave_Init()
{
	unsigned char i;
	EA=0;//发送前，关闭总中断，防止发送时被中断打断
	for(i=0;i<8;i++)
	{
		US_TX=1;//发送高电平
		Delay12us();//持续12.5微秒，这里用12微秒近似
		US_TX=0;//发送低电平
		Delay12us();//持续12.5微秒
	}
	EA=1;//发送后，重新开启总中断
}

//测距函数 返回值：测得的超声波模块与障碍物之间的距离
// --- 距离计算原理 ---
// 声速 V = 340 m/s = 34000 cm/s = 0.034 cm/us
// 测得的时间 T 是超声波来回的总时间
// 距离 S = (T * V) / 2 = T * (0.034 / 2) = T * 0.017 cm
// 其中T的单位是微秒(us)，S的单位是厘米(cm)。
unsigned char Ut_Wave_Data()
{
	unsigned int time;// 用于存储 从发出波到接收波所用的时间
	// 配置单片机内部的PCA（可编程计数器阵列）作为高精度计时器
  // CMOD = 0x00: 设置PCA的时钟源为系统时钟的12分频 (在12MHz下，每1us计数一次)
  CMOD = 0x00;
  // 清空PCA计数器
  CH = CL = 0;
	Ut_Wave_Init();//生成8个40kHz方波
	// CR是PCA的运行控制位。置1后，PCA计数器开始计数。
  CR = 1;
	
	// 等待，直到接收到回波（US_RX变为低电平）或计时器溢出 跳出while循环
  // CF是PCA的溢出标志位，如果计数器溢出了，CF会被硬件置1
  while((US_RX == 1) && (CF == 0));
	CR = 0;//停止PCA计时器
	//判断跳出while循环的原因
	if(CF == 0)//如果CF还为0，说明上方跳出while循环是由于接收到了回波 使US_RX==0
	{
		time=(CH<<8)|CL;// 读取PCA计数器的16位值
		return (time*0.017);
	}
	else //如果是因为计数器溢出导致退出while循环
	{
		CF=0;//重置CF标志位 把CF的值由1变为0，为下一次测算做准备
		return 0;//返回0表示我们距离的测算有问题
	}
}