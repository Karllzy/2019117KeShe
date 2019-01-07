#include "reg52.h"
#include "absacc.h"

typedef unsigned int u16;
typedef unsigned char u8;

u8 	receiveCounter=0, sendCounter=0;							//用于接受和发送的寄存器
u8	receiveBuff[7] = {0x55, 0xff, 0xC5, 0x00, 0x00, 0xAA, 0xff};	//发送格式为:55, 地址, CH5, 数据高, 数据低, 校验码(高四位无效)
u8	sendBuff[5] = {0x55, 0xff, 0xEE, 0xAA, 0xff};				//失败报错格式为:55, 地址， EEH， AA, 校验码
u8	motors1=0x00, motors2=0x00;

void UsartInit();
u8 readSwitch();
u8 getCheckCode(u8 receive_or_send);

void  main()
{
	UsartInit();
	//InitTimer0()	暂时没有必要使用
	while(1)
	{
		P1 = motors1;
		P2 = motors2 | 0xf0;									//使得高4位全部置为1，避免影响其他外设
	}						
}
/*******************************************************************************
* 函数名         :UsartInit()
* 函数功能		 :初始化串口中断
* 输入           : 无
* 输出         	 : 无
*******************************************************************************/
void UsartInit()
{
	EA = 1;			    //打开中断总开关
	ET1 = 1;			//允许定时器1中断
	TMOD=0X21;			//设置定时器1为工作方式2，（定时器0为工作方式1）
	PCON=0X00;			//波特率不加倍（SMOD置为0）

	TH1=0XFD;			//设置波特率为9600bps
	TL1=0XFD;			
	TR1=1;				//开始定时
}


/*******************************************************************************
* 函数名         : Usart() interrupt 4
* 函数功能		 : 串口通信中断函数
* 输入           : 无
* 输出         	 : 无
*******************************************************************************/
void Usart() interrupt 4
{
	if(RI)
	{
		switch(receiveCounter)
		{
			case 0:											//如果是第1位, 先进行协议确认,协议错误，直接放弃
				if(SBUF==0x55)
				{
					receiveBuff[receiveCounter] = SBUF;
					receiveCounter++ ;
				}
				else
				{
					receiveCounter = 0;
				}
				break;

			case 1:											//如果是第2位，进行地址确认，如果地址确认失败，直接放弃
				if (SBUF == readSwitch())
				{
					receiveBuff[receiveCounter] = SBUF;
					receiveCounter++ ;
				}
				else
				{
					receiveCounter = 0 ;
				}
				break;

			case 2:											//如果是第3位，看看指令对不对，如果指令错了，不仅放弃还要发送错误报告
				if (SBUF == 0xC5)
				{
					receiveBuff[receiveCounter] = SBUF;
					receiveCounter++ ;	
				}
				else
				{
					SBUF = sendBuff[sendCounter] ;
					receiveCounter = 0 ;//放弃此次接收数据
					sendCounter++ ;
				}
				break;
			case 3:											//如果是第4位直接接收
				receiveBuff[receiveCounter] = SBUF;
				receiveCounter++ ;
				break;
			case 4:											//如果是第5位直接接收
				receiveBuff[receiveCounter] = SBUF;
				receiveCounter++ ;
				break;
			case 5:											//如果是第6位，进行协议确认，若协议错误，不仅放弃还要发送错误报告
				if (SBUF == 0xAA)
				{
					receiveBuff[receiveCounter] = SBUF;
					receiveCounter++ ;
				}
				else
				{
					SBUF = sendBuff[sendCounter] ;
					receiveCounter = 0 ;//放弃此次接收数据
					sendCounter++ ;
				}
				break;
			case 6:											//如果是第7位, 进行校验码检查	
				if(SBUF == getCheckCode(1))					//进行接受检验码检查
				{
					motors1 = receiveBuff[4];			//低八位放入motors1
					motors2 = receiveBuff[3];               //高八位放入motors2
				}
		}
		RI = 0; 											//清空RI等待下次被中断
	}
	else
	{
		switch(sendCounter)
		{
			case 1:
				SBUF = readSwitch();						//若是第二位，发送地址
				sendBuff[sendCounter] = readSwitch();
				sendCounter++ ;
				break;
			case 4:											//若是第五位，发送校验码
				SBUF = getCheckCode(0);
			 	sendCounter++ ;
				break;
			default:
				SBUF = sendBuff[sendCounter];
				sendCounter++ ;
		}
		TI = 0;
	}
}

/*******************************************************************************
* 函数名         : InitTimer0
* 函数功能		 : 初始化定时器0
* 输入           : 无
* 输出         	 : 无
*******************************************************************************/
void InitTimer0()
{
	EA = 1;			    //打开中断总开关
	ET0 = 1;			//允许定时器0的中断
	TMOD=0X21;			//设置定时器0为工作方式1

	TH0=0XCD;			//设置定时的时长为1ms
	TL0=0XD4;			
	TR0=1;				//开始定时	
}


/*******************************************************************************
* 函数名         :readSwitch
* 函数功能		 : 读取拨码开关的设定地址
* 输入           : 无
* 输出         	 : 认为设定的地址
*******************************************************************************/
u8 readSwitch()
{	
	u8 address;
	address = XBYTE[0x7FFF] ;
	return address;
}
/*******************************************************************************
* 函数名         : getCheckCode()
* 函数功能		 : 获取校验码
* 输入           : 是接受校验码还是发送校验码，接收校验为1，发送校验为0
* 输出         	 : 校验码
*******************************************************************************/
u8 getCheckCode(u8 receive_or_send)
{
	u8 result = 0x00, i;	
	if( receive_or_send == 1)
	{
		for (i=0; i<6; i++)
		{	
			result += receiveBuff[i] ;
		}
	}
	else
	{
		for (i=0; i<4; i++)
		{
			result += sendBuff[i] ;
		};
	}
	return result; 

}