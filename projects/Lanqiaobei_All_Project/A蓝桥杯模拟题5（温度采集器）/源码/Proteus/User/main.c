/*头文件声明区域*/
#include <REGX52.H>
#include <key.h>
#include <seg.h>
#include <led.h>
/*变量声明区域*/
unsigned char Key_Val=0,Key_Down=0,Key_Up=0,Key_Old=0;
unsigned char Key_Slow_Down=0;//按键消抖变量 10ms
unsigned char Tinput_Data[3]={17,17,17};//温度输入数组，没有数据输入时，百位十位个位均为-
unsigned char Tinput_Index=0;//温度输入数组索引值，0-2
unsigned char Seg_Mode=0;//0-温度采集界面 1-数据显示界面 2-参数设置界面
unsigned char Seg_Pos=0;//数码管位选索引值
unsigned char Seg_Buf[]={16,16,16,16,16,16};//数码管显示内容配置数组
unsigned char Seg_Point[]={0,0,0,0,0,0};//温度采集界面显示小数点，默认都不显示
unsigned char Point_Wela=0;//在温度采集界面，标记当前小数点的位置
unsigned short Seg_Flash_Timer=0;//数码管500ms闪烁计时器
unsigned short Tinput_Datas=0;//用于处理输入的温度的
float Temperature=0.0;//最终的实际温度
bit Seg_Flash_Flag=1;//数码管闪烁标志位 1-亮状态 0-灭状态
bit Point_Flag=0;//一旦输入了一个小数点，它的值就置为1
bit Is_Timing=0;//按键s14，s15计时器开关 1-开 0-关
bit Long_Triggered=0;//长按模式开启标志位 1-开启 0-关闭
unsigned short Key_Timer_500ms=0;//按键500ms计时器
unsigned char Key_Timer_100ms=0;//按键100ms计时器
unsigned char TMAX=30;//温度上限值
unsigned char TMIN=20;//温度下限值
unsigned char TMAX_SET;//临时调整TMAX用的
unsigned char TMIN_SET;//临时调整TMIN用的
unsigned char Tset_Index=0;//参数设置界面切换索引值 0-选中TMAX 1-选中TMIN
bit Frist_Tem_Set=0;//是否进行了第一次的温度参数设置 是-1 否-0
unsigned char Error_Set_Count=0;//错误参数设置的计数器
unsigned char Led_Num=0;//LED10ms亮度等级计数器在0-5之间循环
unsigned char Led_Pwm=0;//亮度等级 5-亮度等级1（最亮） 3-亮度等级2（中间亮度）  1-亮度等级3（最低亮度）
unsigned char Led_Pos=0;//LED位选索引值
unsigned char Ucled[]={0,0,0,0,0,0,0,0};//led点亮或熄灭设置数组 1-点亮 0-熄灭
/*按键处理函数区域*/
void Key_Proc()
{   
    unsigned char m;//case 11 循环局部变量
    if(Key_Slow_Down>=10)
    {   
        Key_Slow_Down=0;
        Key_Val=Key_Read();
        Key_Down=Key_Val&(Key_Val^Key_Old);
        Key_Up=~Key_Val&(Key_Val^Key_Old);
        Key_Old=Key_Val;
        if(Key_Down>=1&&Key_Down<=10) //按键s1-s10被按下
        {   
            if(Seg_Mode==0&&Tinput_Index<3) //如果在温度采集界面，并且还未输入完毕
            {
                Tinput_Data[Tinput_Index]=Key_Down-1;
                Tinput_Index++;
            }
        }
        //按键s14长按处理
        if(Seg_Mode==2)
        {
            if(Key_Down==14) //s14按下
            {
                Is_Timing=1;//计时器功能开启
            }
            if(Key_Timer_500ms<500) //时间小于500ms
            {
                if(Key_Up==14) //按键s14抬起
                {
                    Is_Timing=0;//计时器功能关闭
                    Key_Timer_500ms=0;//500ms按键计时器复位
                    if(Tset_Index==0) //选中TMAX
                    {
                        if(++TMAX_SET>=71) TMAX_SET=10;
                    }
                    else //选中TMIN
                    {
                        if(++TMIN_SET>=71) TMIN_SET=10;
                    }
                }
            }
            else //时间大于500ms
            {   
                Long_Triggered=1;//长按模式开启
                if(Key_Old==14) //s14长按
                {
                    if(Key_Timer_100ms>=100)//每100ms执行一次自增操作
                    {
                        Key_Timer_100ms=0;
                        if(Tset_Index==0) //选中TMAX
                        {
                            if(++TMAX_SET>=71) TMAX_SET=10;
                        }
                        else //选中TMIN
                        {
                            if(++TMIN_SET>=71) TMIN_SET=10;
                        }
                    }
                }
                if(Key_Up==14)
                {
                    Is_Timing=0;//计时器功能关闭
                    Long_Triggered=0;//长按模式关闭
                    Key_Timer_500ms=0;//500ms按键计时器复位
                    Key_Timer_100ms=0;//100ms按键计时器复位
                }
            }
        }       
       //按键s15长按处理
        if(Seg_Mode==2) //在参数设置界面才能使用
        {
            if(Key_Down==15) //按键s15按下
            {
                Is_Timing=1;//计时器功能开启
            }
            if(Key_Timer_500ms<500) //按下持续时间小于500ms
            {
                if(Key_Up==15) //抬起s15
                {
                    Is_Timing=0;//计时器功能关闭
                    Key_Timer_500ms=0;//500ms按键计时器复位
                    if(Tset_Index==0) //选中TMAX
                    {
                        if(--TMAX_SET<=9) TMAX_SET=70;
                    }
                    else //选中TMIN
                    {
                        if(--TMIN_SET<=9) TMIN_SET=70;
                    }
                }
            }
            else //按下持续时间大于等于500ms
            {   
                Long_Triggered=1;//长按模式开启
                if(Key_Old==15) //按键s15长按
                {
                    if(Key_Timer_100ms>=100) //每100ms执行一次自减
                    {
                        Key_Timer_100ms=0;
                        if(Tset_Index==0) //选中TMAX
                        {
                            if(--TMAX_SET<=9) TMAX_SET=70;
                        }
                        else //选中TMIN
                        {
                            if(--TMIN_SET<=9) TMIN_SET=70;
                        }
                    }
                }
                if(Key_Up==15) //s15抬起
                {
                    Is_Timing=0;//计时器功能关闭
                    Long_Triggered=0;//长按模式关闭
                    Key_Timer_100ms=0;//100ms按键计时器复位
                    Key_Timer_500ms=0;//500ms按键计时器复位
                }
            }
        
        }
        switch(Key_Down)
        {   
            case 11:
                if(Seg_Mode==0&&Point_Flag==0&&Tinput_Data[0]!=17) //在温度采集界面,且还没有输入过小数点，且第一位已经输入了数据，再按下s11
                {
                    
                    Seg_Point[2+Tinput_Index]=1;//把现在选中数码管它之前的那一个数码管加上小数点，也就是实现输入完数据才能打小数点
                    /*
                    上面这个暗含了只有Seg_Point[3]和Seg_Point[4]可以打小数点，
                    因为最上面的if限定了第四个数码管必须输入数据才能打小数点
                    那么输入完第四个数码管，Tinput_Index就变成1了，而Tinput_Index
                    的上限值为2
                    */
                    Point_Wela=Tinput_Index;//标记当前哪个数码管打了小数点了
                    Point_Flag=1;//使得一次只能点一个小数点
                }
                break;
            case 12:
                if(Seg_Mode==1||Seg_Mode==2) //在数据显示界面或参数设置界面才能使用
                {   
                    if(Seg_Mode==1)//在数据显示界面按下s12
                    {   
                        TMAX_SET=TMAX;
                        TMIN_SET=TMIN;
                        Tset_Index=0;//把Tset_Index置0，保证进入参数设置界面是先选中TMAX
                        Seg_Mode=2;//跳转参数设置界面
                    }
                    else //如果在参数设置界面按下s12
                    {   
                        Frist_Tem_Set=1;//已经完成了至少一次的参数设置
                        if(TMAX_SET>=TMIN_SET) //yes 就保存数据，跳转数据显示界面
                        {   
                            TMAX=TMAX_SET;
                            TMIN=TMIN_SET;
                            Error_Set_Count=0;//无效参数设置计数器置为0
                            Tset_Index=0;
                            Seg_Mode=1;
                        }
                        else //no 就把前一次的值重新赋值给临时调整变量,跳转数据显示界面
                        {
                            TMAX_SET=TMAX;
                            TMIN_SET=TMIN;
                            Error_Set_Count++;//无效参数设置计数器++
                            Tset_Index=0;
                            Seg_Mode=1;
                        }
                    } 
                }
                break;
            case 13:
                if(Seg_Mode==2) //s13在参数设置界面才能使用
                {
                    Tset_Index++;
                    if(Tset_Index>=2) Tset_Index=0;                    
                }
                break;
            case 16:
                if(Seg_Mode!=0) //在其温度采集界面以外的界面按下s16
                {
                    Tinput_Index=0;
                    Point_Flag=0;//小数点标志位复位
                    Seg_Point[3]=Seg_Point[4]=0;//小数点复位
                    for(m=0;m<3;++m)
                    {
                        Tinput_Data[m]=17;//进入温度采集界面前，清空数据
                    }
                    Seg_Mode=0;
                }
                else //在温度采集界面按下s16
                {
                    if(Point_Flag==0||Tinput_Index<3)//如果没有点小数点或3个数码管没有输入完，为输入不规范
                    {
                        for(m=0;m<3;++m)
                        {
                            Tinput_Data[m]=17;//清空数据
                        }
                        Seg_Point[3]=Seg_Point[4]=0;//小数点复位
                        Point_Flag=0;//小数点标志位复位
                        Tinput_Index=0;//从头开始输入
                    }
                    else //如果输入规范。保存数据，进入数据显示界面
                    {
                        Tinput_Datas=Tinput_Data[0]*100+Tinput_Data[1]*10+Tinput_Data[2]+5;//+5达到四舍五入的目的
                        if(Point_Wela==1)//如果小数点点在第四个数码管
                        {
                            Temperature=Tinput_Datas/100.0;
                        }
                        else //如果小数点点在第五个数码管
                        {
                            Temperature=Tinput_Datas/10.0;
                        }
                        if(((unsigned char)Temperature)>85) //如果实际温度大于85
                        {
                            for(m=0;m<3;++m)
                            {
                                Tinput_Data[m]=17;//清空数据
                            }
                            Seg_Point[3]=Seg_Point[4]=0;//小数点复位
                            Point_Flag=0;//小数点标志位复位
                            Tinput_Index=0;//从头开始输入
                        }
                        else //如果实际温度小于85 即满足题目要求时
                        {
                            Seg_Point[3]=Seg_Point[4]=0;//小数点复位
                            Point_Flag=0;//小数点标志位复位
                            Tinput_Index=0;//输如选择标志位复位
                            Seg_Mode=1;//跳转数据显示界面
                        }
                    }
                
                }
                break;
        }
    }
}

/*数码管处理函数区域*/
void Seg_Proc()
{   
    unsigned char i;//case0 循环局部变量
    unsigned char a;//case2 循环局部变量
    if(Seg_Flash_Timer>=500)
    {
        Seg_Flash_Timer=0;
        Seg_Flash_Flag=!Seg_Flash_Flag;
    }
    switch(Seg_Mode)
    {
        case 0:
            Seg_Buf[0]=10;//‘A’
            Seg_Buf[1]=16;
            Seg_Buf[2]=16;
            for(i=0;i<3;++i)
            {
                if(Tinput_Index==i&&!Seg_Flash_Flag) //如果选中某一位但是为灭状态，就熄灭
                {
                        Seg_Buf[3+i]=16;
                }
                else //如果选中某一位但为亮状态，或没选中，就正常显示
                {
                        Seg_Buf[3+i]=Tinput_Data[i];    
                }
            }
            break;
        case 1:
            Seg_Buf[0]=12;
            Seg_Buf[1]=16;
            Seg_Buf[2]=16;
            Seg_Buf[3]=16;
            Seg_Buf[4]=(Temperature>=10)?(unsigned char)Temperature/10%10:16;//如果实际温度小于10，第五个数码管熄灭
            Seg_Buf[5]=(unsigned char)Temperature%10;
            break;
        case 2:
            Seg_Buf[0]=18;//‘P’
            Seg_Buf[1]=16;
            for(a=0;a<2;++a)
            {
                if(Tset_Index==a&&!Seg_Flash_Flag) //选中TMAX或TMIN且为灭状态
                {
                    Seg_Buf[2+2*a]=16;
                    Seg_Buf[3+2*a]=16;
                }
                else //选中TMAX或TMIN且为亮状态，或者没选中，则正常显示
                {
                    Seg_Buf[2+2*a]=(a==0)?TMAX_SET/10%10:TMIN_SET/10%10;
                    Seg_Buf[3+2*a]=(a==0)?TMAX_SET%10:TMIN_SET%10;                    
                }
            }
            break;
    }

}

/*led处理函数区域*/
void Led_Proc()
{   
    if(Frist_Tem_Set) //至少进行了一次参数设置，才比较实际温度和设置温度
    {
        if(Temperature>TMAX)
        {
            Ucled[0]=1;//L1点亮
            Led_Pwm=5;//亮度等级为1
        }
        else
        {
            Ucled[0]=0;//L1熄灭
        }
        if(Temperature>=TMIN&&Temperature<=TMAX)
        {   
            Ucled[1]=1;//L2点亮
            Led_Pwm=3;//亮度等级为2            
        }
        else
        {
            Ucled[1]=0;//L2熄灭
        }
        if(Temperature<TMIN)
        {
            Ucled[2]=1;//L3点亮
            Led_Pwm=1;//亮度等级为3
        }
        else
        {
            Ucled[2]=0;//L3熄灭
        }
        if(Error_Set_Count!=0) //如果有无效参数设置操作
        {
            Ucled[3]=1;;//L4点亮
        }
        else //出现正确的参数设置后
        {
            Ucled[3]=0;//L4熄灭
        }
    }
}

/*定时器0的初始化函数区域*/
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

/*定时器0的服务中断函数区域*/
void Timer0server() interrupt 1
{   
    TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
    Key_Slow_Down++;
    Seg_Flash_Timer++;
    if(Is_Timing) //计时器开启
    {
        Key_Timer_500ms++;
    }
    if(Long_Triggered) //长按模式开启
    {
        Key_Timer_100ms++;
    }
    if(Seg_Mode==0)
    {
        Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
    }
    else
    {
        Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
    }
    if(++Seg_Pos>=6) Seg_Pos=0;//Seg_Pos在0-5之间循环
    
    if(++Led_Num>=6) Led_Num=0;//每10ms在0-5之间循环一轮
    if(++Led_Pos>=8) Led_Pos=0;//在0-7之间循环
    if(Led_Num<Led_Pwm)//比如Led_Pwm=2时 6ms周期内前2ms正常显示
    {
        Led_Disp(Led_Pos,Ucled[Led_Pos]);
    }
    else//后4ms熄灭 那么在6ms的周期内，人眼是看不出闪烁的，就只会看到不同的亮度 
    {
        Led_Disp(Led_Pos,0);
    }
    
}
void main()
{   
    Timer0Init();
    while(1)
    {
        Key_Proc();
        Seg_Proc();
        Led_Proc();
    }


}