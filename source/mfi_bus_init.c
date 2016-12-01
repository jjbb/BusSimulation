#include "mfi_reg.h"
#include "mfi_linux.h"
#include <stdio.h>
#include <stdlib.h>

/*
 *ע�⣺
 *16.11.17:ʱ�䴰�����ñ��뱣֤�̶�ʱ�䴰��������С����һ�������Ϊ0�Ĺ̶�ʱ�䴰����Ĭ�Ϸ������������ˣ��˺���
 *����������ʱ������֮����Ҫ����fpga���ѵ�һ���̶�ʱ�䴰�����������
*/
void BUS_Drive_Init(void)
{
	//ͬ��ʱ������
	TimeSyncInit(fd_gpmc,1,1,60000,160);                      //340,50
	usleep(1);                                                 
                                                             
	MsgFilterSet(fd_gpmc,ACR0,0,0,1,0);                       //�������ַΪ1
	MsgFilterSet(fd_gpmc,AMR0,0xFFFF,0xFFFF,0,0xFFFF);        
	MsgFilterSet(fd_gpmc,ACR2,0,0,2,0);                       //485��ַΪ2
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

	Write_Reg(fd_gpmc,&(MsgBus_regs->Arb_msg_reset),1);        //�����ٲõķ���FIFO��λ����(������)
	Write_Reg(fd_gpmc,&(MsgBus_regs->Arb_msg_reset),0);        
	                                                           
//	Write_Reg(fd_gpmc,&(TimeSync_regs->Rst_n),1);            //Local time��ʼ��ʱ����������ʹ�ܣ����������һ��ʼ���յ���֡����
	printf("-------------------------------INIT MSG FINISH--------------------------------\n");
	
	Write_Reg(fd_gpmc,&(DataBus_regs->Arb_data_rst),1);        //��շ���fifo
	Write_Reg(fd_gpmc,&(DataBus_regs->Arb_data_rst),0);
	Write_Reg(fd_gpmc,&(DataBus_regs->Arb_data_ctrl),3);
	Write_Reg(fd_gpmc,&(DataBus_regs->Arb_data_ctrl),1);
	Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_rst),1);         //��ս���fifo
	usleep(10);
	Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_rst),0);
	usleep(10);
	DataFilterSet(fd_gpmc,ACR2,0,0,1,0);                      //�������ַΪ1
	DataFilterSet(fd_gpmc,AMR1,0xFFFF,0xFFFF,0,0xFFFF);
//	DataFilter_Set(fd_gpmc,ACR0,0,0,2,0);                    //485���ַΪ2
//	DataFilter_Set(fd_gpmc,AMR0,0xFFFF,0xFFFF,0,0xFFFF);
	DataFilterEn(fd_gpmc,filter0|filter2);

	Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_ctrl),0x0040);
	Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_ctrl),0x0011);   //0:Rx_EN  4:RX_IRQ_EN

//	Data_BUS_INIT_TEST(30);
	
	//���漸�����ڽ��FPGA���ֵ�һ��FIFOһ��ʼ�޷���գ����½�������������⡣ʵ��û��ʲô��ʼ������
	Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_intclr),0x0001); 
	sleep(1);
	Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_intclr),0x0001); 
	sleep(1);
	Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_intclr),0x0001); 
	Write_Reg(fd_gpmc,&(DataBus_regs->Arb_data_head_L),0x0040);  //��ȡ���ⲿ��ע��ʱ��bus_linux.c�У�data_irq_count<2,��Ҫ��Ϊ3
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
/***************����ʱ��Ϣ�̶�ʱ�䴰����ʹ�÷���*******************/
//	//�̶�ʱ�䴰����
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
//  Subwin_Set(fd_gpmc,0x0001,8,136);  //����1���Ӵ�����ʼʱ��4������36
//  MsgSubwin_En(fd_gpmc,0x0000);  //ʹ��1���Ӵ�
/*********************************************************************/