/*头文件声明区域*/
#include <REGX52.H>
#include <key.h>
#include <seg.h>
/*变量声明区域*/
unsigned char Seg_Pos=0;//数码管位选索引值
unsigned char Seg_Buf[]={16,16,16,16,16,16};//数码管显示内容设置数组，一开始默认全部数码管熄灭
unsigned char Seg_Disp_Mode=0;//数码管模式 0-时钟显示模式 1-时钟设置模式 2-闹钟设置模式
unsigned char Clock_Disp[]={23,59,55};//时钟显示界面数组
unsigned char Clock_Set[3];//时钟设置界面数组
unsigned char Seg_Flash_Index=0;//数码管闪烁选择标志位 0-小时位 1-分钟位 2-秒钟位
unsigned char Alarm[]={0,0,0};//闹钟设置最终值数组
unsigned char Alarm_Set[3];//闹钟设置界面数组
bit Alarm_Flag=1;//闹钟功能开启或关闭标志位
unsigned short Timer0_500ms=0;//数码管 500ms计时变量
unsigned short Timer1_500ms=0;//led 500ms计时变量
bit Seg_Flash_Flag=0;//数码管闪烁标志位
unsigned short Timer_1000ms=0;//1000ms计时专用变量
unsigned char Seg_Point[]={0,1,0,1,0,1};//数码管小数点显示专用数组，让第2,4,6个数码管显示小数点
unsigned char Key_Slow_Down=0;//按键减速专用变量
unsigned char Key_Val=0,Key_Down=0,Key_Up=0,Key_Old=0;//按键检测相关变量
bit Alarm_Enable_Flag=0;
bit Led_Flash_State=0; // 定义led闪烁标志位
/*按键处理函数区域*/
void Key_Proc()
{   
    if(Key_Slow_Down>=10)
    {
        Key_Slow_Down=0;
        Key_Val=Key_Read();
        Key_Down=Key_Val&(Key_Val^Key_Old);
        Key_Up=~Key_Val&(Key_Val^Key_Old);
        Key_Old=Key_Val;
        if (Alarm_Enable_Flag==1 && Key_Down!=0)
        {
            Alarm_Enable_Flag=0; // 关闭报警
        }
        switch(Key_Down)
        {
            case 1:
               Seg_Flash_Index=0;//每次按下按键1，把时钟和闹钟设置切换标志位置0，使得每次进入设置都默认是先选中小时位，小时位闪烁。
               Clock_Set[0]=Clock_Disp[0];
               Clock_Set[1]=Clock_Disp[1];
               Clock_Set[2]=Clock_Disp[2];
               Seg_Disp_Mode=1;
               Timer0_500ms=0; //确保每次进入时钟设置模式，数码管从“亮”开始进行闪烁,并且闪烁的频率固定为500ms，使得用户立刻知道选中的数码管开始闪烁了
               Seg_Flash_Flag=1;
               break;
            case 2:
                Seg_Flash_Index=0;//每次按下按键2，把时钟和闹钟设置切换标志位置0，使得每次进入设置都默认是先选中小时位，小时位闪烁
                Alarm_Set[0]=Alarm[0];
                Alarm_Set[1]=Alarm[1];
                Alarm_Set[2]=Alarm[2];
                Seg_Disp_Mode=2;
                Timer0_500ms =0; //确保每次进入时钟设置模式，数码管从“亮”开始进行闪烁,并且闪烁的频率固定为500ms，使得用户立刻知道选中的数码管开始闪烁了
                Seg_Flash_Flag=1;
                break;
            case 3:
                if(Seg_Disp_Mode==1||Seg_Disp_Mode==2)
                {
                    Seg_Flash_Index++;
                    if(Seg_Flash_Index>=3) Seg_Flash_Index=0;
                }
                        
                Timer0_500ms = 0;
                Seg_Flash_Flag = 1;//切换选择位置时，也立即显示反馈
                break;
            case 4:
                Alarm_Flag=!Alarm_Flag;//每次按下按键4，开启或关闭闹钟功能
                break;
            case 5:
            case 6:
                if(Seg_Disp_Mode==1||Seg_Disp_Mode==2)
                {
                    unsigned char* target=(Seg_Disp_Mode==1)?Clock_Set:Alarm_Set;
                    unsigned char limit=(Seg_Flash_Index==0)?24:60;
                    
                    if(Key_Down==5) // 参数+
                    {
                        if(++target[Seg_Flash_Index]>=limit)
                            target[Seg_Flash_Index]=0;
                    }
                    else // 参数-
                    {
                        if(--target[Seg_Flash_Index]==255)
                            target[Seg_Flash_Index]=limit-1;
                    }
                }                 
                break;
            case 7:
                if(Seg_Disp_Mode==1)
                {
                   Clock_Disp[0]=Clock_Set[0];
                   Clock_Disp[1]=Clock_Set[1];
                   Clock_Disp[2]=Clock_Set[2];
                }
                if(Seg_Disp_Mode==2) //按下按键7，如果此时为闹钟设置界面，就把闹钟设置值赋值给闹钟显示值，下一次进入闹钟设置界面，值为上次保存的值
                {
                   Alarm[0]=Alarm_Set[0];
                   Alarm[1]=Alarm_Set[1];
                   Alarm[2]=Alarm_Set[2];
                }
                Seg_Disp_Mode=0;
                break;
            case 8:   //退出之后，返回显示界面，从显示界面上次的时间接着往后走
                Seg_Disp_Mode=0;
                break; 
            default: //处理无效按键值的防御性措施
                break;            
        }
    }
}

/*数码管显示设置区域*/
void Seg_Proc()
{   
    unsigned char i;//定义局部变量，用于case 0的for循环
    unsigned char n;
    unsigned char* pData;//定义局部指针，用于把数组名（数组的首元素的地址）赋值过来，从而利用指针实现数组的相关功能
    switch(Seg_Disp_Mode)
    {
        case 0:
            if(Timer_1000ms>=1000) 
            {
                Timer_1000ms=0; //每过1s就把Timer_1000ms置0
                Clock_Disp[2]++;//并且把秒钟位加一
                if(Clock_Disp[2]==60) //如果秒钟位到60往前进1
                {
                    Clock_Disp[2]=0;
                    Clock_Disp[1]++;
                    if(Clock_Disp[1]==60) //如果分钟位到60就往前进1
                    {
                        Clock_Disp[1]=0;
                        Clock_Disp[0]++;
                        if(Clock_Disp[0]==24) //如果小时位到24就置0
                        {
                            Clock_Disp[0]=0;
                        }
                    }
                    
                }
                // 2. 在时间改变的这一瞬间，进行闹钟触发检测 (事件触发)
                if (Alarm_Flag==1&&Alarm_Enable_Flag==0)
                {
                    if (Clock_Disp[0]==Alarm[0] && Clock_Disp[1] == Alarm[1] && Clock_Disp[2] == Alarm[2])
                    {
                        Alarm_Enable_Flag=1; // 触发报警
                        Timer1_500ms=0;      // 立即重置LED 500ms闪烁计时器
                        Led_Flash_State=1;//强制从“亮”的状态开始闪烁，保证一触发报警，就立马能看到led开始闪烁
                    }
                }                
            }
            for(i=0;i<3;++i)
            {   
            Seg_Buf[0+2*i]=Clock_Disp[i]/10; //i=0时，第一个数码管显示23的十位 i=1时，第三个数码管显示59的十位...
            Seg_Buf[1+2*i]=Clock_Disp[i]%10;//i=0时，第二个数码管显示23的个位 i=1时，第四个数码管显示59的个位...
            }
            break;
            
        case 1:
        case 2:
            pData=(Seg_Disp_Mode==1)?Clock_Set:Alarm_Set;
            if(Timer0_500ms>=500)
            {
                Timer0_500ms=0;
                Seg_Flash_Flag=!Seg_Flash_Flag;
            }
            for(n=0;n<3;n++)
            {
                if(Seg_Flash_Index==n&&!Seg_Flash_Flag) //如果选中某一位，那么每次当Seg_Flash_Flag值等于1时才显示，否则熄灭，实现了闪烁 如果没有选中某一位（小时位/分钟位/秒钟位），那么就是直接十位和个位一起显示
                {
                    Seg_Buf[2*n]=16;     // 熄灭十位
                    Seg_Buf[2*n+1]=16;   // 熄灭个位
                }
                else
                {
                    Seg_Buf[2*n]=pData[n]/10;   // 显示十位
                    Seg_Buf[2*n+1]=pData[n]%10; // 显示个位
                }
            }
            break;
        default:
            Seg_Disp_Mode=0; //如果出现异常，主动回到时钟显示模式
            break;            
    }

}

/*其他模块处理区域*/
/*led和蜂鸣器处理函数*/
void Led_Buzzer_Proc()
{
    if (Alarm_Enable_Flag==1)
    {
        P2_3=0; // 蜂鸣器持续响

        if (Timer1_500ms>=500)
        {
            Timer1_500ms=0;
            Led_Flash_State=!Led_Flash_State; // 每500ms翻转一次亮灭状态，并且只有在报警状态时才翻转
        }

        if (Led_Flash_State==1) // 如果是“亮”状态
        {
            if (Alarm[0]<12)
            {
                P1=0xf0; // 闹钟设置时间为12点之前，点亮前四个LED 
            }
            else
            {
                P1=0x0f; // 闹钟设置时间为12点之后，点亮后四个LED 
            }
        }
        else // 如果是“灭”状态
        {
            P1=0xff; // 所有LED熄灭
        }
    }
    else // 如果未处于报警状态
    {
        P2_3=1;   // 关闭蜂鸣器
        P1=0xff;  // 关闭所有LED
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

/*定时器0的服务中断函数*/
void Timer0server() interrupt 1
{
    TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
    Timer_1000ms++;
    Timer0_500ms++;
    Timer1_500ms++;
    Key_Slow_Down++;
    Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
    if(++Seg_Pos>=6) Seg_Pos=0;
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