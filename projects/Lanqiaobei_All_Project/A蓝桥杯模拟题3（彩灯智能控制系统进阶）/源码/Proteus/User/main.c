/*头文件声明区域*/
#include <REGX52.H>
#include <key.h>
#include <seg.h>
#include <intrins.h>
/*变量和常量声明区域*/
unsigned char Seg_Pos=0;//数码管位选标志位
unsigned char Seg_Buf[]={16,16,16,16,16,16};//数码管显示设置数组 默认为全部熄灭
unsigned char Seg_Mode=0;//数码管的模式 0-运行状态界面 1-流转时间设置界面 2-数据显示界面
bit Seg_Flash_Flag=0;//数码管闪烁标志位 1-亮 0-灭
unsigned char  Seg_Chose_Flag=0;//数码管闪烁选择标志位 0-运行模式闪 1-流转时间闪
unsigned short Seg_Flash_Timer=0;//数码管闪烁400ms计时器
unsigned char Led_Mode=0;//led运行模式0-3 四个模式
unsigned short Led_Move_Time[]={400,400,400,400};//led流转间隔时间数组 400-1200ms 默认四种模式都为400ms
unsigned short Led_Move_Time_Set[4];//led流转时间设置数组
unsigned char Led_Set_Mode=0;//0-3 设置界面的模式，之所以新建一个变量是为了避免比如设置界面设置的是模式2，然后跳出去显示界面也变成模式2
unsigned short Led_Move_Timer=0;//led流转时间计时器
unsigned char Led_Mode34_Data[]={0x7e,0xbd,0xdb,0xe7};//从左到右为18,27,36,45灯亮
unsigned char Led_Mode34_Index=0;//数组Led_Mode34_Data的索引值
unsigned char Ucled=0xfe;
unsigned char Seg_Mode2_Point[]={1,1,0,0,0,0};//数据显示界面数组，1,2两个数码管显示小数点
unsigned char Key_Val=0,Key_Down=0,Key_Up=0,Key_Old=0;//按键检测相关变量
unsigned char Key_Slow_Down=10;//按键消抖专用变量 10ms
unsigned char Led_Disp_Data;//数据显示界面最后两个数码管显示的数据
/* 运行/停止语义化 */
bit System_Flag=0;//1-运行(R) 0-停止（S）
#define RUN  1
#define STOP 0

bit Is_Timing=0;//计时器开关 1-开启 0-关闭
unsigned short Key_Timer_600ms=0;//600ms按键计时器

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
        if (System_Flag==STOP) //系统处于停止状态 
        {
                if(Key_Down==4) //按下s4
                {   if(Seg_Mode==0) //处于运行状态界面
                    {
                        Is_Timing=1;//计时器开启
                    }
                }
                if(Key_Timer_600ms<600) //如果s4按下时间小于600ms
                {
                    if(Key_Up==4) //s4抬起
                    {	
                        Is_Timing=0; //计时器功能关闭
                        Key_Timer_600ms=0;//500ms按键计时器复位
                    }
                }
                else //如果s4按下时间大于等于600ms
                {
                    if(Key_Old==4) //s4长按
                    {
                        Seg_Mode=2;
                    }
                    if(Key_Up==4) //s4抬起
                    {
                        Is_Timing=0; //计时器功能关闭
                        Key_Timer_600ms=0;//500ms按键计时器复位
                        Seg_Mode=0;//回到运行状态界面
                    }
                }	
        }
        switch(Key_Down)
        {   
            unsigned char i;//case6 for循环局部变量
            unsigned char m;//case6 for循环局部变量
            case 4:
                if(Seg_Mode==1)
                {
                    
                    if(Seg_Chose_Flag==0)
                    {
                        Led_Set_Mode--;
                        if(Led_Set_Mode==255) Led_Set_Mode=3;
                    }
                    else
                    {
                        Led_Move_Time_Set[Led_Set_Mode]=Led_Move_Time_Set[Led_Set_Mode]-100;
                        if(Led_Move_Time_Set[Led_Set_Mode]<=300) Led_Move_Time_Set[Led_Set_Mode]=1200; //减到300变1200
                    }
                }
                break;
            case 5:
                if(Seg_Mode==1)
                {
                    if(Seg_Chose_Flag==0)
                    {
                        Led_Set_Mode++;
                        if(Led_Set_Mode>=4) Led_Set_Mode=0;
                    }
                    else
                    {
                        Led_Move_Time_Set[Led_Set_Mode]=Led_Move_Time_Set[Led_Set_Mode]+100;
                        if(Led_Move_Time_Set[Led_Set_Mode]>=1300) Led_Move_Time_Set[Led_Set_Mode]=400;//加到1300变400
                    } 
                }                    
                break;
            case 6:
                if(System_Flag==STOP)
                {
                    if(Seg_Mode==0) // 从运行界面进入设置
                    {
                        for(i=0;i<4;i++)
                        {
                            Led_Move_Time_Set[i]=Led_Move_Time[i];
                        }
                        Led_Set_Mode=Led_Mode; // 默认显示当前模式
                        Seg_Mode=1;
                        Seg_Chose_Flag=0;
                    }
                    else if(Seg_Mode==1) //else if的优先级比if低。先判断上面的if
                    {
                        if(++Seg_Chose_Flag>=2) Seg_Chose_Flag=0 ; //第二次包括第二次之后按下按键6，切换选择运行模式闪还是流转时间间隔闪
                        if(Seg_Chose_Flag==0)  //当第三次按下按键6，保存设置值并返回运行状态界面
                        {
                            for(m=0;m<4;m++)
                            {
                                Led_Move_Time[m]=Led_Move_Time_Set[m];
                            }
                            Seg_Mode=0;
                        }
                    }
                }
                break;
            case 7:
                System_Flag=!System_Flag;
                break;
        
        }
    }

}

/*数码管显示设置函数区域*/
void Seg_Proc()
{   
    switch(Seg_Mode)
    {
        case 0:
            Seg_Buf[0]=(System_Flag==RUN)?10:5;//如果为运行状态第一个数码管显示R，如果为停止状态，显示S
            Seg_Buf[1]=Led_Mode+1;
            Seg_Buf[2]=(Led_Move_Time[Led_Mode]>=1000)?Led_Move_Time[Led_Mode]/1000%10:16;//如果流转时间大于等于1000ms，第三个数码管显示千位，否则熄灭
            Seg_Buf[3]=Led_Move_Time[Led_Mode]/100%10;//百位
            Seg_Buf[4]=Led_Move_Time[Led_Mode]/10%10;//十位
            Seg_Buf[5]=Led_Move_Time[Led_Mode]%10;//个位
            break;
        case 1:
            if(Seg_Flash_Timer>=400) //数码管400ms亮，400ms灭
            {
                Seg_Flash_Timer=0;
                Seg_Flash_Flag=!Seg_Flash_Flag;
            }
            if(Seg_Chose_Flag==0)   //如果运行模式闪
            {
                Seg_Buf[0]=16;//流转时间设置界面的第一个数码管熄灭
                Seg_Buf[1]=(Seg_Flash_Flag==1)?(Led_Set_Mode+1):16;                
            }
            else //否则正常显示运行模式
            {
                Seg_Buf[0]=16;//流转时间设置界面的第一个数码管熄灭
                Seg_Buf[1]=Led_Set_Mode+1;//设置界面正在设置的模式                 
            }
            if(Seg_Chose_Flag==1)  //如果是流转时间闪
            {   
                if(Seg_Flash_Flag==1)
                {
                    Seg_Buf[2]=(Led_Move_Time_Set[Led_Set_Mode]>=1000)?Led_Move_Time_Set[Led_Set_Mode]/1000%10:16;
                }
                else
                {
                    Seg_Buf[2]=16;
                }
                Seg_Buf[3]=(Seg_Flash_Flag==1)?(Led_Move_Time_Set[Led_Set_Mode]/100%10):16;//百位
                Seg_Buf[4]=(Seg_Flash_Flag==1)?(Led_Move_Time_Set[Led_Set_Mode]/10%10):16;//十位
                Seg_Buf[5]=(Seg_Flash_Flag==1)?(Led_Move_Time_Set[Led_Set_Mode]%10):16;//个位                 
            }
            else //否则正常显示流转时间间隔
            {
                Seg_Buf[2]=(Led_Move_Time_Set[Led_Set_Mode]>=1000)?Led_Move_Time_Set[Led_Set_Mode]/1000%10:16;
                Seg_Buf[3]=Led_Move_Time_Set[Led_Set_Mode]/100%10;//百位
                Seg_Buf[4]=Led_Move_Time_Set[Led_Set_Mode]/10%10;//十位
                Seg_Buf[5]=Led_Move_Time_Set[Led_Set_Mode]%10;//个位 
            }
            break;
        case 2:
            Seg_Buf[0]=13;//'d'
            Seg_Buf[1]=10;//'A'
            Seg_Buf[2]=16;//第三个数码管不显示
            Seg_Buf[3]=Led_Mode+1;
            if(Led_Mode<2) //如果为模式0,1
            {
                Seg_Buf[4]=0;//第五个数码管显示0
                Seg_Buf[5]=Led_Disp_Data;
            }
            else  //如果为模式2,3
            {
                Seg_Buf[4]=Led_Disp_Data;
                Seg_Buf[5]=9-Led_Disp_Data;
            }
            break;
    }

}

/*led和蜂鸣器处理函数区域*/
void Led_Proc()
{   
    unsigned char x=0;
    unsigned char cur;   //当前真正显示到端口的值
    if(System_Flag==RUN)
    {
        if(Led_Move_Timer>=Led_Move_Time[Led_Mode])
        {
            Led_Move_Timer=0;
            switch(Led_Mode)
            {
                case 0: //模式1：L1->L8
                    if(Ucled==0x7e) Ucled=0xfe;//从模式4回到模式1，把Ucled的值置为初值0xfe
                    P1=Ucled;
                    cur = P1;                        // 记下当前哪个led亮
                    Ucled=_crol_(Ucled,1);
                    if(Ucled==0xfe) Led_Mode=1;//当模式1运行到最后一步时P1=Ucled=0x7f，然后Ucled=_crol_(Ucled,1)，Ucled变为0xfe，切换到模式2
                    break;
                case 1://模式2：L8->L1 
                    Ucled=_cror_(Ucled,1);
                    P1=Ucled;
                    cur = P1;                        
                    if(Ucled==0xfe) Led_Mode=2;//当模式2运行到最后一步后，切换到模式3
                    break;
                case 2://模式3：18->27->36->45
                    Ucled=Led_Mode34_Data[Led_Mode34_Index];
                    P1=Ucled;
                    cur = P1;
                    if(++Led_Mode34_Index>=4) 
                    {
                        Led_Mode34_Index=0;
                        Led_Mode=3; ////当模式3运行到最后一步后，切换到模式4
                    }
                    break;
                case 3://模式4：45->36->27->18
                    Ucled=Led_Mode34_Data[3-Led_Mode34_Index];
                    P1=Ucled;
                    cur = P1;
                    if(++Led_Mode34_Index>=4) 
                    {
                        Led_Mode34_Index=0;
                        Led_Mode=0;//当模式4运行到最后一步后，切换到模式1
                    }
                    break;            
            } 
        }
        else
        {
            // 未到步进时也要拿当前口线值参与显示
            cur = P1;
        }
        while(x<8&&(~cur&(0x01<<x))==0) //这个while循环的作用就是 通过位移x次让~Ucled与0x01<<x对齐如果(~Ucled&(0x01<<x))等于0说明没对齐 比如~Ucled=0000 0010 x就需要位移一次
        {
            x++;
        }
        Led_Disp_Data=x+1;
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

/*定时器0的中断服务函数区域*/
void Timer0server() interrupt 1
{
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
    if(Seg_Mode==2)
    {
        Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Mode2_Point[Seg_Pos]);
    }
    else
    {
        Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0); 
    }
    if(++Seg_Pos>=6) Seg_Pos=0;
    Key_Slow_Down++;
    Seg_Flash_Timer++;
    if(System_Flag==RUN) Led_Move_Timer++;//当系统处于运行状态时，Led_Move_Timer开始计时
    if(Is_Timing)
    {
        Key_Timer_600ms++;
    }
}

/*main函数区域*/
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
