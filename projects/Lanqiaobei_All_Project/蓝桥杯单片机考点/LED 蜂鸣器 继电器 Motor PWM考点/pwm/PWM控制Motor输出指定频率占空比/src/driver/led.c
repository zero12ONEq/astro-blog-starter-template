/***********************************************************************
 * 文件名: led.c
 * 功能描述: LED显示和外设控制模块，用于控制LED、蜂鸣器、电机和继电器
 * 主要函数: 
 *   - Led_Disp: LED显示控制
 *   - Led_Off: 关闭所有LED
 *   - Beep: 蜂鸣器控制
 *   - Motor: 电机控制
 *   - Relay: 继电器控制
 ***********************************************************************/

 #include "led.h"

 // 保存当前LED状态，用于状态变化检测
 idata unsigned char temp_1=0x00;
 idata unsigned char temp_1_old=0xff;
 
 /**
  * 功能: LED显示控制
  * 描述: 根据传入的LED状态数组控制8个LED的亮灭
  *       只有当LED状态发生变化时才更新输出，减少I/O操作
  * 参数: ucLed - LED状态数组，每个元素对应一个LED
  *              0表示亮，1表示灭
  * 示例: {1,0,1,0,1,0,1,0} 对应二进制 10101010
  */
 void Led_Disp(unsigned char *ucLed)
 {
	 unsigned char temp;
	 
	 // 将8个LED状态合并为一个字节
	 temp_1=0x00;
	 temp_1=(ucLed[0]<<0)|(ucLed[1]<<1)|(ucLed[2]<<2)|(ucLed[3]<<3)|
		   (ucLed[4]<<4)|(ucLed[5]<<5)|(ucLed[6]<<6)|(ucLed[7]<<7);
	 
	 // 只有当LED状态发生变化时才更新
	 if(temp_1!=temp_1_old)
	 {
		 P0 = ~temp_1;  // LED低电平亮，所以需要取反
		 
		 /*
			 通过P2口地址线选择LED锁存器
			 P2 = (P2&0x1f) | 0x80;  // 将P2的高3位设置为100，选择LED锁存器
			 P2 &= 0x1f;             // 将P2的高3位清零，释放锁存器
		 */
		 temp = P2 & 0x1f;       // 保留P2的低5位
		 temp = temp | 0x80;     // 将高3位设置为100(选择LED锁存器)
		 P2 = temp;
		 temp = P2 & 0x1f;       // 将高3位清零(释放锁存器)
		 P2 = temp;
		 
		 temp_1_old = temp_1;    // 保存当前状态
	 }
 }
 
 /**
  * 功能: 关闭所有LED
  * 描述: 将所有LED熄灭
  */
 void Led_Off()
 {
	 unsigned char temp;
	 
	 P0 = 0xff;  // LED低电平亮，所以全1表示全灭
	 
	 /*
		 通过P2口地址线选择LED锁存器
		 P2 = (P2&0x1f) | 0x80;  // 将P2的高3位设置为100，选择LED锁存器
		 P2 &= 0x1f;             // 将P2的高3位清零，释放锁存器
	 */
	 temp = P2 & 0x1f;       // 保留P2的低5位
	 temp = temp | 0x80;     // 将高3位设置为100(选择LED锁存器)
	 P2 = temp;
	 temp = P2 & 0x1f;       // 将高3位清零(释放锁存器)
	 P2 = temp;
	 
	 temp_1_old=0x00;        // 重置状态记录
 }
 
 // 保存当前外设状态，用于状态变化检测
 idata unsigned char temp_2=0x00;
 idata unsigned char temp_2_old=0xff;
 
 /**
  * 功能: 蜂鸣器控制
  * 描述: 控制蜂鸣器的开启和关闭
  * 参数: enable - 0表示关闭，1表示开启
  */
 void Beep(bit enable)
 {
	 unsigned char temp;
	 
	 // 设置蜂鸣器状态位(P0.6)
	 if(enable) temp_2 |= 0x40;   // 将第6位置1，开启蜂鸣器
	 else temp_2 &= ~(0x40);      // 将第6位置0，关闭蜂鸣器
	 
	 // 只有当状态发生变化时才更新
	 if(temp_2!=temp_2_old)
	 {
		 P0 = temp_2;
		 
		 /*
			 通过P2口地址线选择外设锁存器
			 P2 = (P2&0x1f) | 0xa0;  // 将P2的高3位设置为101，选择外设锁存器
			 P2 &= 0x1f;             // 将P2的高3位清零，释放锁存器
		 */
		 temp = P2 & 0x1f;       // 保留P2的低5位
		 temp = temp | 0xa0;     // 将高3位设置为101(选择外设锁存器)
		 P2 = temp;
		 temp = P2 & 0x1f;       // 将高3位清零(释放锁存器)
		 P2 = temp;
		 
		 temp_2_old = temp_2;    // 保存当前状态
	 }
 }
 
 /**
  * 功能: 电机控制
  * 描述: 控制电机的开启和关闭
  * 参数: enable - 0表示关闭，1表示开启
  */	
 void Motor(bit enable)
 {
	 unsigned char temp;
	 
	 // 设置电机状态位(P0.5)
	 if(enable) temp_2 |= 0x20;   // 将第5位置1，开启电机
	 else temp_2 &= ~(0x20);      // 将第5位置0，关闭电机
	 
	 // 只有当状态发生变化时才更新
	 if(temp_2!=temp_2_old)
	 {
		 P0 = temp_2;
		 
		 /*
			 通过P2口地址线选择外设锁存器
			 P2 = (P2&0x1f) | 0xa0;  // 将P2的高3位设置为101，选择外设锁存器
			 P2 &= 0x1f;             // 将P2的高3位清零，释放锁存器
		 */
		 temp = P2 & 0x1f;       // 保留P2的低5位
		 temp = temp | 0xa0;     // 将高3位设置为101(选择外设锁存器)
		 P2 = temp;
		 temp = P2 & 0x1f;       // 将高3位清零(释放锁存器)
		 P2 = temp;
		 
		 temp_2_old = temp_2;    // 保存当前状态
	 }
 }
 
 /**
  * 功能: 继电器控制
  * 描述: 控制继电器的开启和关闭
  * 参数: enable - 0表示关闭，1表示开启
  */	
 void Relay(bit enable)
 {
	 unsigned char temp;
	 
	 // 设置继电器状态位(P0.4)
	 if(enable) temp_2 |= 0x10;   // 将第4位置1，开启继电器
	 else temp_2 &= ~(0x10);      // 将第4位置0，关闭继电器
	 
	 // 只有当状态发生变化时才更新
	 if(temp_2!=temp_2_old)
	 {
		 P0 = temp_2;
		 
		 /*
			 通过P2口地址线选择外设锁存器
			 P2 = (P2&0x1f) | 0xa0;  // 将P2的高3位设置为101，选择外设锁存器
			 P2 &= 0x1f;             // 将P2的高3位清零，释放锁存器
		 */
		 temp = P2 & 0x1f;       // 保留P2的低5位
		 temp = temp | 0xa0;     // 将高3位设置为101(选择外设锁存器)
		 P2 = temp;
		 temp = P2 & 0x1f;       // 将高3位清零(释放锁存器)
		 P2 = temp;
		 
		 temp_2_old = temp_2;    // 保存当前状态
	 }
 }