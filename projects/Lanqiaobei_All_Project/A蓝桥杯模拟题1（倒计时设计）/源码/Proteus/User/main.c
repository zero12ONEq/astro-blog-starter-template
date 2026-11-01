/*头文件声明区域*/
#include <REGX52.H>
#include <key.h>
#include <seg.h>

/*变量声明区域*/
unsigned char Key_Val=0,Key_Down=0,Key_Up=0,Key_Old=0; 
unsigned char Key_Slow_Down=0; //按键减速专用变量 10ms
unsigned char Seg_Pos=0;//数码管扫描变量
unsigned char Seg_Buf[]={16,16,16,16,16,16};//数码管显示数据存放数组，上电后所有数码管熄灭
unsigned char Seg_Mode=0;//数码管显示界面 上电后初值为0 0表示数码管为显示界面 1表示数码管为设置界面
unsigned short Timer_1000ms=0; //1000ms标志位
unsigned char Time_Count=30;//系统计时变量 上电后显示界面从30s开始倒计时
bit System_Flag=0;//系统标志位 上电后为0 0表示暂停 1表示开始 
unsigned char Seg_Dat[]={15,30,60};//数码管设置模式的参数数组
unsigned char Seg_Dat_Index=1;//数码管设置模式的参数数组的辅助变量，上电后默认为30s，所以Seg_Dat_Index的值赋1
unsigned short Timer_500ms=0;//500ms标志位
bit Seg_Flag=0;//数码管标志位 初值设置为0或1都可以 这里写0
/*按键处理函数*/
void Key_Proc()
{
    if(Key_Slow_Down>=10)  //每10ms检测一次按键是否被按下（其实就是起到消抖的目的）
    {   
        Key_Slow_Down=0;
        Key_Val=Key_Read();
        Key_Down=Key_Val&(Key_Val^Key_Old);
        Key_Up=~Key_Val&(Key_Val^Key_Old);
        Key_Old=Key_Val;
        
        switch(Key_Down)
        {
            case 1:
                if(Seg_Mode==0) System_Flag=1; //按键1在显示界面被按下时 系统标志位置为1
                break;
            case 2:
                if(Seg_Mode==0) 
                {
                    Time_Count=Seg_Dat[Seg_Dat_Index];//在显示界面按下按键2，复位
                    System_Flag=0;//复位完成后，System_Flag置0，停止整个系统
                }
                break;
            case 3:
                if(Seg_Mode==1)  //若原来是设置模式，按下按键3，先把在设置界面设置好的时间要赋值给Time_Count
                {
                    Time_Count=Seg_Dat[Seg_Dat_Index];
                }
                Seg_Mode=!Seg_Mode; //按键3按下时，数码管显示在显示界面和设置界面切换
                break;
            case 4:
                if(Seg_Mode==1)
                {
                 if(++Seg_Dat_Index==3) Seg_Dat_Index=0; //Seg_Dat_Index在0,1,2之间切换
                }
                break;     
        }
    }
    

}
/*数码管显示函数*/
void Seg_Proc()
{   
    Seg_Buf[0]=Seg_Mode+1; //Seg_Mode=0时，第一个数码管显示1 Seg_Mode=1时，第一个数码管显示2
    if(Seg_Mode==0)  //如果此时为显示界面
    {   
        if(System_Flag==1)  //当系统标志位为1，即系统工作时，才进行倒计时
        {
            if(Timer_1000ms>=1000)   //每过一秒钟，系统计时变量减1
            {
                Timer_1000ms=0;
                Time_Count--;
                if(Time_Count==255) Time_Count=0;//让倒计时结束后，一直显示0
            }
        }
        Seg_Buf[4]=Time_Count/10%10; //把Time_Count的十位显示在第五个数码管上
        Seg_Buf[5]=Time_Count/1%10;  //把Time_Count的个位显示在第六个数码管上
    }
    else
    {   
        if(Timer_500ms>=500)
        {   
            Timer_500ms=0;
            Seg_Flag=!Seg_Flag;//每500ms取反一次
        }
        if(Seg_Flag==1) //当Seg_Flag=1时，第五第六数码管显示
        {
            Seg_Buf[4]=Seg_Dat[Seg_Dat_Index]/10%10; //把要设置的时间的十位放在第五个数码管上
            Seg_Buf[5]=Seg_Dat[Seg_Dat_Index]/1%10;   //把要设置的时间的个位放在第六个数码管上
        }
        else  //当Seg_Flag=0时，第五第六数码管熄灭
        {
            Seg_Buf[4]=16;
            Seg_Buf[5]=16;
        }
        
    }
}

/*其他外设处理函数*/
void Led_Buzzer_Proc()  //led和蜂鸣器处理函数
{
    if(Time_Count==0&&System_Flag == 1) //只有当系统处于运行状态并且倒计时时间到了，才会led全亮，蜂鸣器响
    {
        P1=0x00; //倒计时到了的时候，往P1口写0x00，P1口输出低电平 led全亮
        P2_3=0;  //倒计时到了的时候，往P2_3写0，P2_3引脚输出低电平 蜂鸣器响
    }
    else
    {
        P1=0xff;//倒计时还没到的时候，往P1口写0xff,P1口输出高电平 关闭全部led
        P2_3=1; //倒计时还没到的时候，往P2_3写1，P2_3引脚输出高电平 蜂鸣器不响
    }
}
/*定时器0初始化函数*/
void Timer0Init(void)		//每1ms调用一次，那么也就是每1ms开启一次ET0中断 1毫秒@12.000MHz
{
	TMOD &= 0xF0;		//设置定时器模式
    TMOD |= 0x01; // 确保是模式1
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
    
    ET0=1;//打开ET0 （局部中断）
    EA=1;//打开EA （全局中断）
}

/*定时器0中断服务函数*/
void time0server() interrupt 1
{
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
    Key_Slow_Down++;
    Timer_1000ms++;
    Timer_500ms++;
    Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]);
    if(++Seg_Pos==6) Seg_Pos=0;
}

/*main函数区域*/
void main()
{  
   Timer0Init();
   while(1)
   {   
       Key_Proc();
       Seg_Proc();
       Led_Buzzer_Proc();
   }
}

