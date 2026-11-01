#include <longpress.h>

// 初始化长按检测器
void LongPress_Init(LongPressDetector* detector) 
{
    detector->key_code = 0;
    detector->timer = 0;
    detector->repeat_timer = 0;
    detector->is_timing = 0;
    detector->long_triggered = 0;
}

// 长按检测主函数
KeyPressType LongPress_Detect(LongPressDetector* detector,unsigned char key_down,unsigned char key_up,unsigned char key_old) 
{
    KeyPressType result = KEY_IDLE;//没有按键按下，返回空闲
    
    // 检测按键按下
    if(key_down) 
    {
        detector->key_code = key_down;
        detector->is_timing = 1;
        detector->timer = 0;
        detector->repeat_timer = 0;
        detector->long_triggered = 0;
    }
    
    // 检测按键抬起
    if(key_up && detector->is_timing) //如果在计时状态下抬起按键
    {
        detector->is_timing = 0;//停止计时
        
        // 判断是短按
        if(detector->timer < LONGPRESS_THRESHOLD) //如果计时器当前时间小于长按触发值
        {
            result = KEY_SHORT_PRESS;//返回短按
        }
        
        detector->timer = 0;//计时器复位
        detector->key_code = 0;//当前按键值复位
        return result;
    }
    
    // 检测长按状态
    if(detector->is_timing && key_old == detector->key_code) //如果计时开启并且某个按键一直按住
    {
        if(detector->timer >= LONGPRESS_THRESHOLD) //如果计时器时间达到长按触发值
        {
            if(!detector->long_triggered) //如果long_triggered=0，即还未触发过长按
            {
                detector->long_triggered = 1; //long_triggered置为1
                result = KEY_LONG_PRESS;  // 返回首次长按
            } 
            else //如果long_triggered=1，即之前已经触发过长按了
            {
                // 连续触发
                if(detector->repeat_timer >= REPEAT_INTERVAL) //repeat_timer每达到REPEAT_INTERVAL值之后置为0，本题保证自增/自减的速率控制在100ms
                {
                    detector->repeat_timer = 0;
                    result = KEY_REPEAT;//返回连续触发
                } 
                else //repeat_timer没有达到REPEAT_INTERVAL值，即持续按住但未到触发间隔
                {
                    result = KEY_HOLDING;  //返回 持续按住但未到触发间隔
                }
            }
        }
    }
    
    return result;
}

// 定时器更新(在定时器中断中调用,1ms调用一次)
void LongPress_TimerUpdate(LongPressDetector* detector) 
{
    if(detector->is_timing)//判断是否计时已经开启
    {
        if(detector->timer < 0xFFFF) 
        {
            detector->timer++;
        }
        if(detector->long_triggered && detector->repeat_timer < 0xFFFF) 
        {
            detector->repeat_timer++;
        }
    }
}

//// 获取当前按键持续时间
//unsigned short LongPress_GetHoldTime(LongPressDetector* detector) 
//{
//    return detector->timer;
//}
