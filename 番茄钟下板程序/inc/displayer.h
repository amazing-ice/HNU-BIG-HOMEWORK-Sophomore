/**********************************display V2.0a 说明 ************************************************************************
displayer用于控制“STC-B学习板”上8个7段数码管（Seg7）和8个指示灯（Led）工作.提供显示模块加载和5个应用函数：

  (1) displayerInit()：显示模块加载函数；
	
  (2) SetDisplayerArea(char Begin_of_scan,char Ending_of_Scan)：设置LED启用区域。8个数码管从左至右编号分别为0——7，函数参数
      Begin_of_scan设定启用数码管起始编号，Ending_of_Scan为结束编号。设定范围内的数码管才工作和显示。
			注：正常情况下，两个参数取值范围为0——7，且Ending_of_Scan>Begin_of_scan。但利用动态扫描和人眼视觉效果，可设置超出
			    该范围的参数，以实现特殊显示效果：如软件调整显示亮度，或非灯亮度显示，等；
					
  (3) Seg7Print(char d0,char d1,char d2,char d3,char d4,char d5,char d6,char d7)：将8个参数值分别译码显示到对应的数码管上。
      显示译码表(code char decode_table[])在main.c中,用户可以修改和增减.
			
  (4) LedPrint(char led_val)：控制8个指示灯开关。参数light_val的8个bit位对应8个指示灯的开关，“1”——指示灯“亮”
 （5）Seg7SetMode(char m0,char m1,char m2,char m3,char m4,char m5,char m6,char m7)：8个数码管闪烁显示控制。8个参数分别控制8个数码管
           =0：对应显示常亮
					 =1～9：对应不同方式闪烁
					 =10～254：常暗
					 =255：方式不变
					 m0：控制左起第1个数码管，m1：控制左起第1个数码管......
 （6）LedSetMode(char m0,char m1,char m2,char m3,char m4,char m5,char m6,char m7)	：8个LED灯闪烁显示控制。8个参数分别控制8个LED灯		 
          参数意义：同（5）
           m0：控制左起第1个LED灯，m1：控制左起第1个LED灯......
编写：徐成（电话18008400450）   2021年2月26日设计，2024年10月15日更新，增加控制闪烁显示功能函数：Seg7SetMode、LedSetMode
*/

#ifndef _displayer_H_
#define _displayer_H_
extern void DisplayerInit();										                                               //加载显示模块工作
extern void SetDisplayerArea(char Begin_of_scan,char Ending_of_Scan);						               //设置有效显示区域
extern void Seg7Print(char d0,char d1,char d2,char d3,char d4,char d5,char d6,char d7);        //在数码管上输出	
extern void LedPrint(char led_val);                                                            //在LED灯上输出
extern void Seg7SetMode(char m0,char m1,char m2,char m3,char m4,char m5,char m6,char m7);      //数码管闪烁控制。 =0：对应显示常亮。=1～9：对应不同方式闪烁。=10～254：常暗。=255：方式不变
extern void LedSetMode(char m0,char m1,char m2,char m3,char m4,char m5,char m6,char m7);       //LED灯闪烁控制。  =0：对应显示常亮。=1～9：对应不同方式闪烁。=10～254：常暗。=255：方式不变
#endif  

