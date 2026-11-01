/*头文件区域*/
#include <STC89C5xRC.H>
#include <intrins.h>
#include <key.h>
#include <seg.h>

/*变量声明区域*/
unsigned char Uled=0xFE; //让uled一开始等于1111 1110
unsigned char Key_Val=0,Key_Down=0,Key_Up=0,Key_Old=0; 
unsigned char Key_Slow_Down = 0;//按键减速专用变量
unsigned char Seg_Pos=0;//数码管位选标志位
unsigned char Seg_Buf[]={16,16};//数码管段选设置专用数组
bit System_Flag=1;//定义一个bit类型的变量，上电后默认启动
unsigned short Led_Slow_Down=0;//led减速专用变量
unsigned char Led_34_Data[4]={0x7e,0xbd,0xdb,0xe7};//模式3,4的数组 第一个数为18灯亮 第二个数为27灯亮
unsigned char Led_Data_Intex=0;
unsigned char Led_5_Data[4]={0xee,0xdd,0xbb,0x77};//模式5的数组，依次为15 26 37 48灯亮
unsigned char Led_Mode=1;//彩灯控制系统的模式 上电后默认为模式1

/*按键处理函数*/
void Key_Proc()
{
    if(Key_Slow_Down>=10)
    {
       Key_Slow_Down=0;
       Key_Val=Key_Read();
       Key_Down=Key_Val&(Key_Val^Key_Old); 
       Key_Up=~Key_Val&(Key_Val^Key_Old);
       Key_Old = Key_Val;
       
       switch(Key_Down)
       {
           case 1:
              System_Flag=1;//当Key_Down=1，也就是按键1被按下时，把System_Flag标志位置为1
              break;
           case 2:
              System_Flag=0;//当Key_Down=2，也就是按键2被按下时，把System_Flag标志位置为0
              break;
           case 3:
               Led_Mode++;
               if(Led_Mode==6) Led_Mode=1;
               //每当模式切换后，都重置彩灯的状态
               Led_Data_Intex=0;
               Uled=0xFE;
               break;
           case 4:
               Led_Mode--;
               if(Led_Mode==0) Led_Mode=5;
               //每当模式切换后，都重置彩灯的状态
               Led_Data_Intex=0;
               Uled=0xFE;
               break;    
       }
    }

}
/*数码管显示设置函数*/

void Seg_Proc()
{   
    Seg_Buf[0]=Led_Mode;
    if(System_Flag==1) 
    {
        Seg_Buf[1]=10;
    }
    else
    {
        Seg_Buf[1]=11;
    }
}
/*其他外设处理函数*/
void Led_Proc()
{
    if (Led_Slow_Down >= 500)
    {     
       Led_Slow_Down=0;
       if(System_Flag==1) //当标志位的值为1时，执行流水灯
       {    
           switch(Led_Mode)
           {
               case 1:
                   P1=Uled;
                   Uled=_crol_(Uled,1);//循环左移
                   break;
               
               case 2:
                   Uled=_cror_(Uled,1);//循环右移
                   P1=Uled;
                   break;  
               case 3: 
                   P1=Led_34_Data[Led_Data_Intex];
                   Led_Data_Intex++;
                   if(Led_Data_Intex==4) Led_Data_Intex=0;
                   break;
               case 4:
                   P1=Led_34_Data[3-Led_Data_Intex];
                   Led_Data_Intex++;
                   if(Led_Data_Intex==4) Led_Data_Intex=0;
                   break;               
               case 5:
                   P1=Led_5_Data[Led_Data_Intex];
                   Led_Data_Intex++;
                   if(Led_Data_Intex==4) Led_Data_Intex=0;                    
                   break;
           }
       } 
    }
}
/*定时器初始化函数*/
void Timer0Init(void)		//每1ms调用一次，那么也就是每1ms开启一次ET0中断 1毫秒@12.000MHz
{
	TMOD &= 0xF0;		//设置定时器模式
    TMOD |= 0x01; // 确保是模式1
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
    
    ET0=1;//打开定时器0的中断 （局部中断）
    EA=1;//打开EA （全局中断）
}

/*定时器0中断服务函数*/
void time0server() interrupt 1
{
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
    Seg_Proc();
    Key_Slow_Down++;
    Led_Slow_Down++;
    Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]);
    if(++Seg_Pos==2) Seg_Pos=0;
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

