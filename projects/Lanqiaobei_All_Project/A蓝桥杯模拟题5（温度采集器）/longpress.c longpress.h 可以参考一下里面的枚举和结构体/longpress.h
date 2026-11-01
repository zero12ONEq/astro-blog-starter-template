#include <REGX52.H>
#ifndef __LONGPRESS_H__
#define __LONGPRESS_H__

// 长按配置参数
#define LONGPRESS_THRESHOLD  500   // 长按判定时间(ms)
#define REPEAT_INTERVAL      100   // 连续触发间隔(ms)

// 长按状态枚举
typedef enum 
{
    KEY_IDLE = 0,      // 空闲
    KEY_SHORT_PRESS,   // 短按
    KEY_LONG_PRESS,    // 长按(首次)
    KEY_REPEAT,        // 连续触发
    KEY_HOLDING        // 持续按住(不触发动作)
} KeyPressType;

// 长按检测结构体
typedef struct
{
    unsigned char key_code;        // 当前按键值
    unsigned short timer;          // 计时器
    unsigned short repeat_timer;   // 连续触发计时器
    unsigned char is_timing;       // 计时标志位
    unsigned char long_triggered;  // 长按已触发标志
} LongPressDetector;

// 函数声明
void LongPress_Init(LongPressDetector* detector);
KeyPressType LongPress_Detect(LongPressDetector* detector,unsigned char key_down,unsigned char key_up,unsigned char key_old);
void LongPress_TimerUpdate(LongPressDetector* detector);
//unsigned short LongPress_GetHoldTime(LongPressDetector* detector);  // 获取持续按下时间

#endif

