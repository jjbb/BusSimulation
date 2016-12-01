#include "mfi_reg.h"
#include "mfi_linux.h"
#include <stdio.h>
#include <stdlib.h>

/*
 *注意：
 *16.11.17:时间窗的配置必须保证固定时间窗的数量最小都有一个，编号为0的固定时间窗总是默认分配给主机。因此，此函数
 *中在配置完时间周期之后，需要配置fpga，把第一个固定时间窗分配给主机。
*/
void BUS_Drive_Init(void)
{
	//同步时间设置
	TimeSyncInit(fd_gpmc,1,1,60000,160);                      //340,50
	usleep(1);                                                 
                                                             
	MsgFilterSet(fd_gpmc,ACR0,0,0,1,0);                       //主机板地址为1
	MsgFilterSet(fd_gpmc,AMR0,0xFFFF,0xFFFF,0,0xFFFF);        
	MsgFilterSet(fd_gpmc,ACR2,0,0,2,0);                       //485地址为2
	MsgFilterSet(fd_gpmc,AMR1,0xFFFF,0xFFFF,0,0xFFFF);
//	MsgFilter_Set(fd_gpmc,ACR2,0x00000500);
//	MsgFilter_Set(fd_gpmc,AMR1,0xFFFFF83F);
	Write_Reg(fd_gpmc,&(MsgBus_regs->Msg_rx_ctrl),0);
	MsgRxFifoRst(fd_gpmc,1);
	MsgRxFifoRst(fd_gpmc,0);
	Write_Reg(fd_gpmc,&(MsgBus_regs->Msg_rx_rst),0);
	MsgReceiveEn(fd_gpmc,1);
	MsgFilterEn(fd_gpmc,filter0|filter2);
	MsgRxIntEn(fd_gpmc,1);
//	Write_Reg(fd_gpmc,&(MsgBus_regs->Msg_rxint_clr),1);
//	Write_Reg(fd_gpmc,&(TimeSync_regs->Rst_n),1);
//	Write_Reg(fd_gpmc,&(MsgBus_regs->Msg_rx_ctrl),0x0002);
//	Write_Reg(fd_gpmc,&(MsgBus_regs->Msg_rx_rst),0);
//	Write_Reg(fd_gpmc,&(MsgBus_regs->Msg_rx_ctrl),0x0019);

	Write_Reg(fd_gpmc,&(MsgBus_regs->Arb_msg_reset),1);        //自由仲裁的发送FIFO复位操作(必须有)
	Write_Reg(fd_gpmc,&(MsgBus_regs->Arb_msg_reset),0);        
	                                                           
//	Write_Reg(fd_gpmc,&(TimeSync_regs->Rst_n),1);            //Local time开始计时，放在这里使能，否则可能在一开始接收到半帧数据
	printf("-------------------------------INIT MSG FINISH--------------------------------\n");
	
	Write_Reg(fd_gpmc,&(DataBus_regs->Arb_data_rst),1);        //清空发送fifo
	Write_Reg(fd_gpmc,&(DataBus_regs->Arb_data_rst),0);
	Write_Reg(fd_gpmc,&(DataBus_regs->Arb_data_ctrl),3);
	Write_Reg(fd_gpmc,&(DataBus_regs->Arb_data_ctrl),1);
	Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_rst),1);         //清空接收fifo
	usleep(10);
	Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_rst),0);
	usleep(10);
	DataFilterSet(fd_gpmc,ACR2,0,0,1,0);                      //主机板地址为1
	DataFilterSet(fd_gpmc,AMR1,0xFFFF,0xFFFF,0,0xFFFF);
//	DataFilter_Set(fd_gpmc,ACR0,0,0,2,0);                    //485板地址为2
//	DataFilter_Set(fd_gpmc,AMR0,0xFFFF,0xFFFF,0,0xFFFF);
	DataFilterEn(fd_gpmc,filter0|filter2);

	Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_ctrl),0x0040);
	Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_ctrl),0x0011);   //0:Rx_EN  4:RX_IRQ_EN

//	Data_BUS_INIT_TEST(30);
	
	//下面几步用于解决FPGA部分第一个FIFO一开始无法清空，导致接收有问题的问题。实际没有什么初始化作用
	Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_intclr),0x0001); 
	sleep(1);
	Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_intclr),0x0001); 
	sleep(1);
	Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_intclr),0x0001); 
	Write_Reg(fd_gpmc,&(DataBus_regs->Arb_data_head_L),0x0040);  //当取消这部分注释时，bus_linux.c中，data_irq_count<2,需要改为3
	Write_Reg(fd_gpmc,&(DataBus_regs->Arb_data_head_H),0);
	Write_Reg(fd_gpmc,&(DataBus_regs->Arb_data_body_L),8);
	Write_Reg(fd_gpmc,&(DataBus_regs->Arb_data_body_H),1);
	Write_Reg(fd_gpmc,&(DataBus_regs->Arb_data_body_L),0);
	Write_Reg(fd_gpmc,&(DataBus_regs->Arb_data_body_H),1);
	Write_Reg(fd_gpmc,&(DataBus_regs->Arb_data_sendrdy),0xA55A);
	sleep(3);
	Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_intclr),0x0001); 
	printf("-------------------------------INIT DATA FINISH--------------------------------\n");
}

/******************************************************************/
/***************测试时消息固定时间窗设置使用方法*******************/
//	//固定时间窗发送
//	Write_Reg(fd_gpmc,&(MsgBus_regs->Time_msg_body0),0x1234);
//	Write_Reg(fd_gpmc,&(MsgBus_regs->Time_msg_body1),0x7890);
//	Write_Reg(fd_gpmc,&(MsgBus_regs->Time_msg_body2),0x3456);
//	Write_Reg(fd_gpmc,&(MsgBus_regs->Time_msg_body3),0x9012);
//	Write_Reg(fd_gpmc,&(MsgBus_regs->Time_msg_body4),0x5678);
//	Write_Reg(fd_gpmc,&(MsgBus_regs->Time_msg_body5),0x1234);
//	Write_Reg(fd_gpmc,&(MsgBus_regs->Time_msg_head_L),0x5151); //5151
//	Write_Reg(fd_gpmc,&(MsgBus_regs->Time_msg_head_H),0x0001);//0001
//	Write_Reg(fd_gpmc,&(MsgBus_regs->Time_msg_cfg_L),0x0001);//0001
//	Write_Reg(fd_gpmc,&(MsgBus_regs->Time_msg_cfg_H),0x0000);//0001
//	
//  Subwin_Set(fd_gpmc,0x0001,8,136);  //设置1号子窗，起始时刻4，长度36
//  MsgSubwin_En(fd_gpmc,0x0000);  //使能1号子窗
/*********************************************************************/
