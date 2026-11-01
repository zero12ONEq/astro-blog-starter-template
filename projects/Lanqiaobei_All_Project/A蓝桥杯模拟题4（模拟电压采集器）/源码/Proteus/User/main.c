/*头文件声明区域*/
#include <REGX52.H>
#include <key.h>
#include <seg.h>
#include <led.h>
/*常量和变量声明区域*/
unsigned char Seg_Mode=0;//0-电压采集界面 1-数据显示界面 2-参数设置界面 3-计数统计界面
unsigned char Seg_Pos=0;//数码管位选标志位
unsigned char Seg_Buf[]={16,16,16,16,16,16};//数码管显示内容设置数组，默认全部熄灭
unsigned char Seg_Point1[]={0,0,0,1,0,0};//Vain_Disp_Data<10时的小数点，以及Vain_Set_Data的小数点
unsigned char Seg_Point2[]={0,0,0,0,1,0};//Vain_Disp_Data>10时的小数点
unsigned char Input_Vain_Data[4]={20,20,20,20};//电压输入值数组 0-3分别表示输入值（单位mv）的千位，百位，十位，个位 不给元素赋值默认为-
float New_Vain_Disp_Data;//新输入的实际电压
float Vain_Disp_Data=0.00;//电压数据显示界面变量，默认是0.00,可当成前一次输入的实际电压
float Vain_Set_Data=3.00;//参数设置界面变量（设置电压） 为浮点型 默认为3.00
unsigned int Count=0;//计数统计界面计数器
bit Count_Flag=0;//计数值标志位 当实际电压大于设置电压时为1 反之为0
unsigned char Input_Index=0;//电压输入数组的索引值 
unsigned char Key_Val=0,Key_Down=0,Key_Up=0,Key_Old=0;//按键读取专用变量
unsigned char Key_Slow_Down=0;//按键减速专用变量 10ms
unsigned short Seg_Flash_Timer=0;//数码管闪烁专用计时器 500ms/次
unsigned char Led_Pos=0;//led扫描标志位
unsigned char Led_Buf[8]={0,0,0,0,0,0,0,0};//led亮或灭配置数组 默认8个led都是灭的
unsigned short Key_Error_Count=0;//无效按键被按下次数的计数器
unsigned short Sys_Ticks=0;//当实际电压小于设置电压时的计时器
bit Seg_Flash_Flag=0;//数码管闪烁标志位
bit First_Collection_Flag = 0; // 第一次采集标志位，用于防止上电后Vain_Disp_Data小于Vain_Set_Data所导致的L1点亮的情况
/*按键处理函数区域*/
void Key_Proc()
{   
    unsigned char m;//case11 循环局部变量
    unsigned char j;//case14 循环局部变量
    unsigned char a;//case11 循环局部变量
    if(Key_Slow_Down>=10)
    {
        Key_Slow_Down=0;
        Key_Val=Key_Read();
        Key_Down=Key_Val&(Key_Val^Key_Old);
        Key_Up=~Key_Val&(Key_Val^Key_Old);
        Key_Old=Key_Val;
        
        if(Key_Down>=1&&Key_Down<=10)
        {   
            if(Seg_Mode==0&&Input_Index<4) //只有在电压采集界面时才能输入电压，并且进行输入限制，输入完4个数据就不能输入了
            {   
                Key_Error_Count=0;//有效按键操作 Key_Error_Count置为0
                Input_Vain_Data[Input_Index]=Key_Down-1;//按下按键1，输入0，按下按键3，输入2...
                Input_Index++;//3->4->5->6个数码管依次进行输入
            }
            else
            {
                Key_Error_Count++; //当按键1-10在非电压采集界面被按下，无效按键计数器+1
            }
        }
        switch(Key_Down)
        {
            case 11:
                Key_Error_Count=0;//有效按键操作 Key_Error_Count置为0
                if(Seg_Mode==0)  //如果处于电压采集界面
                {
                    if(Input_Index>=4) //输入数据规范，即3,4,5,6数码管都输入进去了数据，才保存数据
                    {   
                        //保存数据 +5以达到进位的目的 /1000.0使得最终New_Vain_Disp_Data显示的是小数
                        New_Vain_Disp_Data=(Input_Vain_Data[0]*1000+Input_Vain_Data[1]*100+Input_Vain_Data[2]*10+Input_Vain_Data[3]+5)/1000.0;
                        First_Collection_Flag=1;//标记第一次采集已完成
                        // 检查触发条件：前一次电压 > Vp 且新电压 < Vp
                        if (Vain_Disp_Data > Vain_Set_Data && New_Vain_Disp_Data < Vain_Set_Data) 
                        {
                                Count++; // 计数值加1
                        }
                        
                        // 更新前一次电压值
                        Vain_Disp_Data = New_Vain_Disp_Data;
                        Seg_Mode=1;//返回电压数据显示界面
                    }
                    else //输入不规范，清空数据并重新从第3个数码管开始输入
                    {
                        Input_Index=0;
                        for(m=0;m<4;m++)
                        {
                            Input_Vain_Data[m]=20;//把之前S1-S10输入的数据都写-，达到清空的目的。
                        }
                    }
                
                }
                else //如果原来不处于电压采集界面，按下s11，跳转电压采集界面
                {
                    for(a=0;a<4;++a)
                    {
                        Input_Vain_Data[a]=20;//进入电压采集界面前清空数据
                    }
                    Input_Index=0;//电压输入数组索引值置为0，达到重新输入数据的目的
                    Seg_Mode=0;
                }
                break;
            case 12:
                if(Seg_Mode>0)
                {   
                    Key_Error_Count=0;//有效按键操作 Key_Error_Count置为0
                    if(++Seg_Mode>=4) Seg_Mode=1; //让数码管模式在1-3之间循环
                }
                else 
                {
                    Key_Error_Count++;//在电压采集界面按下s12，无效按键计数器+1
                }
                break;
            case 14:
                if(Seg_Mode==0) //在电压采集界面按下S14
                {   
                    Key_Error_Count=0;//有效按键操作 Key_Error_Count置为0
                    for(j=0;j<4;++j)
                    {
                        Input_Vain_Data[j]=20; //清空
                    }
                    Input_Index=0;//把Input_Index置为0，从第一位开始重新输入
                }
                if(Seg_Mode==3) //在计数统计界面
                {   
                    Key_Error_Count=0;//有效按键操作 Key_Error_Count置为0
                    Count=0;//清空
                }
                if(Seg_Mode==1||Seg_Mode==2)
                {
                    Key_Error_Count++;//在电压数据显示界面或电压参数设置界面按下s14，无效按键计数器+1
                }
                break;
            case 15:
            case 16:
                if(Seg_Mode==2)
                {   
                    Key_Error_Count=0;//有效按键操作 Key_Error_Count置为0
                    Vain_Set_Data=(Key_Down==15)?Vain_Set_Data+0.50:Vain_Set_Data-0.50;//如果s15按下，加，如果s16按下，减
                    if(Key_Down==15)
                    {
                        if(Vain_Set_Data>=6.00) Vain_Set_Data=1.00; //达到6v上限，变下限1v
                    }
                    else
                    {
                        if(Vain_Set_Data<=1.00) Vain_Set_Data=6.00; //达到1v下限，变上限6v
                    }
                }
                else
                {
                    Key_Error_Count++;//在电压参数设置界面以外的界面按下s15或s16，无效按键计数器+1
                }
                break;
        }
    }
}

/*数码管显示设置函数区域*/
void Seg_Proc()
{   
    unsigned char i; //case0 for循环局部变量

    switch(Seg_Mode)
    {
        case 0:
            if(Seg_Flash_Timer>=500)
            {
                Seg_Flash_Timer=0;
                Seg_Flash_Flag=!Seg_Flash_Flag;//每500ms翻转一次
            }
            Seg_Buf[0]=16;
            Seg_Buf[1]=16;
            for(i=0;i<4;++i)
            {
                if(Input_Index==i&&!Seg_Flash_Flag) //如果选中某一位并且为“不亮”状态，就熄灭
                {
                    Seg_Buf[2+i]=16;
                }
                else //如果没选中或者选中但是为“亮”状态，就点亮
                {
                    Seg_Buf[2+i]=Input_Vain_Data[i];
                }
            }
            break;
        case 1:
            Seg_Buf[0]=17;//第一个数码管显示U
            Seg_Buf[1]=16;//第2,3个数码管熄灭
            Seg_Buf[2]=16;
            //把浮点数强制转化成整数，把小数点后面的数全部抛弃 使得第4个数码管显示小数点前的数字
            //但是如果输入电压为9999，那么Vain_Disp_Data为10.00v,那么取十位输入第四个数码管。
            Seg_Buf[3]=(Vain_Disp_Data>=10)?(unsigned int)Vain_Disp_Data/10%10:(unsigned int)Vain_Disp_Data;
            Seg_Buf[4]=(unsigned int)(Vain_Disp_Data*100)/10%10;//第五个数码管取小数点后的第一位数 2.78->278.00->278->7
            Seg_Buf[5]=(unsigned int)(Vain_Disp_Data*100)%10;//第六个数码管取小数点后的第二位数 2.78->278.00->278->8
            break;
        case 2:
            Seg_Buf[0]=18;//第一个数码管显示P
            Seg_Buf[1]=16;//第2,3个数码管熄灭
            Seg_Buf[2]=16;
            Seg_Buf[3]=(unsigned int)Vain_Set_Data;//把浮点数强制转化成整数，把小数点后面的数全部抛弃 使得第4个数码管显示小数点前的数字
            Seg_Buf[4]=(unsigned int)(Vain_Set_Data*100)/10%10;//第五个数码管取小数点后的第一位数 5.50->550.00->550->5
            Seg_Buf[5]=(unsigned int)(Vain_Set_Data*100)%10;//第六个数码管取小数点后的第二位数 5.50->550.00->550->0
            break;
        case 3:
            Seg_Buf[0]=19;//第一个数码管显示n
            Seg_Buf[1]=(Count/10000%10==0)?16:Count/10000%10;//万位，若没数据就熄灭，有数据则正常显示，下面同理
            Seg_Buf[2]=(Count/1000%10==0)?16:Count/1000%10;//千位
            Seg_Buf[3]=(Count/100%10==0)?16:Count/100%10;//百位
            Seg_Buf[4]=(Count/10%10==0)?16:Count/10%10;//十位
            Seg_Buf[5]=Count%10;//个位
            break;
    }

}

/*led和蜂鸣器处理函数区域*/
void Led_Proc()
{   
    if(First_Collection_Flag&&Vain_Disp_Data<Vain_Set_Data) //只有当采集了至少一次实际电压并且，实际电压小于设置电压时，才进行计数
    {
        if(Sys_Ticks>=5000)
        {
            Led_Buf[0]=1;//当实际电压小于设置电压的时间超过5s，L1点亮
        }
        else
        {
            Led_Buf[0]=0;//否则熄灭
        }
    }
    else //当实际电压大于设置电压时
    {   
        Sys_Ticks=0; //计时器置为0
        Led_Buf[0]=0;//L1不亮
    }
    Led_Buf[1]=(Count%2==0)?0:1;//当Count为偶数时L2熄灭，为奇数时L2点亮
    Led_Buf[2]=Key_Error_Count/3;//连续3次以上的无效按键操作，L3点亮，触发有效按键操作时熄灭
    
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
    Key_Slow_Down++;
    Seg_Flash_Timer++;
    Led_Disp(Led_Pos,Led_Buf[Led_Pos]);
    if(++Led_Pos>=8) Led_Pos=0;//让Led_Pos在0-7之间循环
    if(Vain_Disp_Data<Vain_Set_Data)
    {
        Sys_Ticks++; //当实际电压小于参数设置电压时，计时器++
    }
    if(Seg_Mode==1||Seg_Mode==2) //电压数据显示界面或参数设置界面要显示小数点
    {
        if(Seg_Mode==1)
        {
            if(Vain_Disp_Data>=10)//如果实际电压大于10v，第五个数码管显示小数点
            {
                Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point2[Seg_Pos]);
            }
            else //否则第四个数码管显示小数点
            {
                Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point1[Seg_Pos]);
            }
        }
        else //如果为参数设置界面，第四个数码管显示小数点
        {
            Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point1[Seg_Pos]);
        }
    }
    else //其他数码管界面不显示小数点
    {
        Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
    }
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
        Led_Proc();
    }
}
