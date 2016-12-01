#include "mfi_test_define.h"
#include "mfi_module_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//void Test_Gpmc_Fun(void)
//{
//	  int i=0;
//		unsigned short int result;
//	
//	//读写测试地址
////		result=BUS.Read_Reg(BUS.filp,&(BUS.Common_regs->VID));
////		printf("VID:0x%4x ",result);
////		result=BUS.Read_Reg(BUS.filp,&(BUS.Common_regs->PID));
////		printf("PID:0x%4x\n",result);
////
////		result=BUS.Read_Reg(BUS.filp,&(BUS.Common_regs->RW_test));
////		printf("Read:0x%4x\n",result);
////		BUS.Write_Reg(BUS.filp,&(BUS.Common_regs->RW_test),0x55AA);
////		printf("Write:0x55AA ");
////		result=BUS.Read_Reg(BUS.filp,&(BUS.Common_regs->RW_test));
////		printf("Read:0x%4x\n",result);
////		BUS.Write_Reg(BUS.filp,&(BUS.Common_regs->RW_test),0xAA55);
////		printf("Write:0xAA55 ");
////		result=BUS.Read_Reg(BUS.filp,&(BUS.Common_regs->RW_test));
////		printf("Read:0x%4x\n",result);
////		BUS.Write_Reg(BUS.filp,&(BUS.Common_regs->RW_test),0x0000);
////		printf("Write:0x0000 ");
////		result=BUS.Read_Reg(BUS.filp,&(BUS.Common_regs->RW_test));
////		printf("Read:0x%4x\n",result);
////		BUS.Write_Reg(BUS.filp,&(BUS.Common_regs->RW_test),0xFFFF);
////		printf("Write:0xFFFF ");
////		result=BUS.Read_Reg(BUS.filp,&(BUS.Common_regs->RW_test));
////		printf("Read:0x%4x\n",result);
//		
////		result=BUS.Read_Reg(BUS.filp,&(BUS.TimeSync_regs->SyncP_senden));
////		printf("Read SyncP_senden:%d\n",result);		
////		result=BUS.Read_Reg(BUS.filp,&(BUS.TimeSync_regs->Twin_length));
////		printf("Read Twin_length:%d\n",result);		
////		result=BUS.Read_Reg(BUS.filp,&(BUS.TimeSync_regs->T_basic_set));
////		printf("Read T_basic_set:0x%4x\n",result);	
////		result=BUS.Read_Reg(BUS.filp,&(BUS.TimeSync_regs->Local_timer));
////		printf("Read Local_timer:%d\n",result);		
//		result=BUS.Read_Reg(BUS.filp,&(BUS.TimeSync_regs->Local_timer));
//		printf("Read Local_timer:%d\n",result);	
//		result=BUS.Read_Reg(BUS.filp,&(BUS.TimeSync_regs->Sync_status));
//		printf("Read Sync_status:%d\n",result);		
//
//
////		result=BUS.Read_Reg(BUS.filp,&(BUS.MsgBus_regs->Msg_rx_status));
////		printf("Read Msg_rx_status:0x%x\n",result);		
////		result=BUS.Read_Reg(BUS.filp,&(BUS.MsgBus_regs->Msg_rx_body0));
////		printf("Read Msg_rx_body0:0x%x\n",result);
////		result=BUS.Read_Reg(BUS.filp,&(BUS.MsgBus_regs->Msg_rx_body1));
////		printf("Read Msg_rx_body1:0x%x\n",result);
//		result=BUS.Read_Reg(BUS.filp,&(BUS.MsgBus_regs->Msg_rx_status));
//		printf("Read Msg_rx_status:0x%x\n",result);		
////		result=BUS.Read_Reg(BUS.filp,&(BUS.MsgBus_regs->Msg_rx_ctrl));
////		printf("Read Msg_rx_ctrl:0x%x\n",result);		
////		result=BUS.Read_Reg(BUS.filp,&(BUS.MsgBus_regs->Msg_ACR2_L));
////		printf("Read Msg_ACR2_L:0x%x\n",result);		
////		result=BUS.Read_Reg(BUS.filp,&(BUS.MsgBus_regs->Msg_ACR2_H));
////		printf("Read Msg_ACR2_H:0x%x\n",result);		
////		BUS.Write_Reg(BUS.filp,&(BUS.MsgBus_regs->Msg_rxint_clr),1);
//		
//		sleep(2);
////			BUS.TimeSync_Init(&BUS,0,0,288,100);//340,50
//		
//		//闪灯
//		for(i=0;i<3;i++)
//		{
//			BUS.Write_Reg(BUS.filp,&(BUS.Common_regs->LED_status),0);
//			sleep(1);
//			BUS.Write_Reg(BUS.filp,&(BUS.Common_regs->LED_status),1);
//			sleep(1);
//		}
//		
////		  BUS.MsgSubwin_En(&BUS,0x0000);  //使能1号子窗
////		BUS.Write_Reg(BUS.filp,&(BUS.TimeSync_regs->SyncP_senden),0);
////			BUS.TimeSync_Init(&BUS,1,1,288,100);//340,50
//
//	printf("\n");	
//}

#define test_len_msg 53
	
MfiStatus Test_SendMsg(MfiSession mfi)
{
	int i;
	char* data=NULL;
	MfiStatus status;
	
	data=malloc(test_len_msg);
	
	for(i=0;i<test_len_msg;i++)
		data[i]=i;
	
	status = MfiWriteMsg(mfi, 0, 1, data, test_len_msg, MFI_TMO_INFINITE,0);
	free(data);
	data=NULL;
	return status;
}

MfiStatus Test_ReceiveMsg(MfiSession Mfi, MfiUInt32 timeout)
{
	int j=0;
	MfiUInt8* pointer=NULL,*tmpp=NULL;
	MfiStatus status;
	MfiUInt32 msgtype;
	MfiUInt32 retCnt;
		
	status=MfiReadMsg(Mfi, &msgtype, &pointer, &retCnt, timeout);
	tmpp=pointer;
	if(status!=MFI_SUCCESS)
		return status;
/*	if(i!=test_len_msg){
		printf("message receive length error!\n");
		exit(1);
		}*/
	printf("MSG TYPE: %d:\n",msgtype);
	while(retCnt)
	{
		if((j==15)||(retCnt==1))
		{
			printf("0x%x\n",*pointer);
/*			if(*pointer!=test_len_msg-retCnt){
				printf("message receive data error!\n");
				exit(1);
			}*/
			j=0;
			retCnt--;
			pointer+=1;
		}
		else
		{
			printf("0x%x ",*pointer);
/*			if(*pointer!=test_len_msg-retCnt){
				printf("message receive data error!\n");
				exit(1);
			}*/
			j++;
			retCnt--;
			pointer+=1;			
		}
	}
	MfiCombMsgFree(tmpp);
	pointer=NULL;
	tmpp=NULL;
	
	return status;
}

MfiStatus Test_ReceiveMsgAsync(MfiSession Mfi){
	MfiPBuf buf=NULL;
	MfiUInt32 msgtype;
	MfiUInt32 retCnt;
	MfiStatus status;
	
	status=MfiReadMsgAsync(Mfi, &msgtype, &buf, &retCnt, NULL);
	if(status==MFI_SUCCESS_SYNC)
		printf("Test_ReceiveMsgAsync: MFI_SUCCESS_SYNC!\n");
	else if(status!=MFI_SUCCESS) 
		printf("ERROR: Test_ReceiveMsgAsync: status=0x%08x , Mfi=%d\n",status,Mfi);
		
	return status;
}

MfiStatus Test_SendMsgAsync(MfiSession Mfi){
	int i;
	char* data=NULL;
	MfiStatus status;
	
	data=malloc(test_len_msg);
	
	for(i=0;i<test_len_msg;i++)
		data[i]=i;
		
	status = MfiWriteMsgAsync(Mfi, 0, 1, data, test_len_msg, NULL);
	if(status==MFI_SUCCESS_SYNC)
		printf("Test_SendMsgAsync: MFI_SUCCESS_SYNC!\n");
	else if(status==MFI_SUCCESS) 
		printf("Test_SendMsgAsync_MFI_SUCCESS!\n");
	else
		printf("ERROR: Test_SendMsgAsync: status=0x%08x , Mfi=%d\n",status,Mfi);
//	free(data);
	data=NULL;
	return status;
}

MfiUInt32 test_async_recvnum=0;
MfiStatus test_handler(MfiSession Mfi, MfiEventType eventType, MfiEvent event, MfiAddr userHandle){
	MfiPBuf buf=NULL,tmpbuf=NULL;
	MfiUInt32 msgtype,w_or_r;
	MfiUInt32 retCnt;
	int j=0;
	
	
	if(eventType!=MFI_EVENT_IO_COMPLETION){
		printf("EVENT DEAL ERROR: eventType error!");
		return 0;
	}

	MfiGetAttribute(event, MFI_ATTR_ASYNC_TYPE, &w_or_r, 0);
	if(w_or_r==MFI_ASYNC_READ_MSG){
		MfiGetAttribute(event, MFI_ATTR_BUFFER, &buf, 0);
		MfiGetAttribute(event, MFI_ATTR_DATA_TYPE, &msgtype, 0);
		MfiGetAttribute(event, MFI_ATTR_RET_COUNT, &retCnt, 0);
		tmpbuf=buf;
	
		printf("ASYNC RECV HANDLER: MFI: %d, MSG TYPE: %d:\n",Mfi,msgtype);
	
		while(retCnt)
		{
			if((j==15)||(retCnt==1))
			{
				printf("0x%x\n",*buf);
/*			if(*pointer!=test_len_msg-retCnt){
				printf("message receive data error!\n");
				exit(1);
			}*/
				j=0;
				retCnt--;
				buf+=1;
			}
			else
			{
				printf("0x%x ",*buf);
/*			if(*pointer!=test_len_msg-retCnt){
				printf("message receive data error!\n");
				exit(1);
			}*/
				j++;
				retCnt--;
				buf+=1;			
			}
		}
	
		MfiCombMsgFree(tmpbuf);
		//free(tmpbuf);
		tmpbuf=NULL;
		buf=NULL;
		test_async_recvnum++;
	}
	else if(w_or_r==MFI_ASYNC_WRITE_MSG){
		MfiGetAttribute(event, MFI_ATTR_BUFFER, &buf, 0);
		free(buf);
		buf=NULL;
		printf("ASYNC SEND HANDLER: MFI: %d\n",Mfi);
	}
	
	return MFI_SUCCESS;
}

void ModuleInit(int num){
	MfiPUInt8 tmp=NULL;
	int i=0;
	
	Module.number=num;
	tmp=malloc(num*9+2); //+2个byte用于存放板数num
	((MfiUInt16*)tmp)[0]=num;
	Module.Module_Info_p=(_Module_Info_p)(tmp+2);
		
	for(i=0;i<num;i++)
	{
		Module.Module_Info_p[i].mod_ip=i+2;
		Module.Module_Info_p[i].mod_id[0]=i+2;
		Module.Module_Info_p[i].manf_id[0]=i+2;
		Module.Module_Info_p[i].mod_resv1=0;
		Module.Module_Info_p[i].mod_resv2=0;
	}

	return;
}

//测试重发机制，此时虚拟总线管理器每7帧丢弃一帧
void test_resend(MfiSession mfi){
	MfiStatus status;
	if(Test_SendMsg(mfi)==MFI_SUCCESS) printf("Test_SendMsg SUCCESS : 1  !!!\n"); else  printf("Test_SendMsg ERROR : 1  !!!\n"); 
	if(Test_SendMsg(mfi)==MFI_SUCCESS) printf("Test_SendMsg SUCCESS : 2  !!!\n"); else  printf("Test_SendMsg ERROR : 2  !!!\n"); 
	if(Test_SendMsg(mfi)==MFI_SUCCESS) printf("Test_SendMsg SUCCESS : 3  !!!\n"); else  printf("Test_SendMsg ERROR : 3  !!!\n"); 
	if(Test_SendMsg(mfi)==MFI_SUCCESS) printf("Test_SendMsg SUCCESS : 4  !!!\n"); else  printf("Test_SendMsg ERROR : 4  !!!\n"); 
	Test_ReceiveMsgAsync(mfi);
	Test_ReceiveMsgAsync(mfi);
	if((status=Test_ReceiveMsg(mfi,2000))!=MFI_SUCCESS)  printf("ERROR: Test_ReceiveMsg: status=0x%08x , Mfi=%d\n",status,mfi);
	if((status=Test_ReceiveMsg(mfi,2000))!=MFI_SUCCESS)  printf("ERROR: Test_ReceiveMsg: status=0x%08x , Mfi=%d\n",status,mfi);
}

void test_async_io(MfiSession mfi){
	static int Msg_sync_flag=0;
	static int Msg_async_flag_recv=0;
	static int Msg_async_flag_send=0;
	static int sendnum=0,recvnum=0;
	static int flag=0;
	pthread_t pid;
	MfiStatus status;

	if(sendnum>200)
		return;

	if(flag==0){
		pthread_create(&pid,NULL,asyncWrite,(void*)mfi);
		flag=1;
		}
	
	//Test_ReceiveMsg(mod_mfi[0],5);
	Msg_sync_flag++;
	if(Msg_sync_flag==600000){
		if((status=Test_SendMsg(mfi))!=MFI_SUCCESS){
			printf("ERROR: Test_SendMsg: status=%d\n",status);
		}
		else
			sendnum++;
		//	printf("Test_SendMsg success!\n");
		Msg_sync_flag=0;
	}
		
	if(Msg_async_flag_recv==10000){
		Test_ReceiveMsgAsync(mfi);
		Msg_async_flag_recv=0;
	}
	Msg_async_flag_recv++;
		
	if(Msg_async_flag_send==400000){
		Test_SendMsgAsync(mfi);
		sendnum++;
		Msg_async_flag_send=0;
	}
	Msg_async_flag_send++;
	
}

void*asyncWrite(void* attr){
	static int sendnum=0;
	static int Msg_async_flag_send=0;

	while(1){
		if(sendnum>200)
			sleep(10);
	
		if(Msg_async_flag_send==400000){
			Test_SendMsgAsync((MfiUInt32)attr);
			sendnum++;
			Msg_async_flag_send=0;
		}
		Msg_async_flag_send++;
	}
}

//MSG test_msg_1 = {.m_id.all=0x1,.m_cf.all=0xa1,.m_txt1.m_int=0x02010a01,.m_txt2.m_int=0x06050403,.m_txt3.m_int=0x0a090807};
//MSG test_msg_2 = {.m_id.all=0x1,.m_cf.all=0x31,.m_txt1.m_int=0x0c0b0300,.m_txt2.m_int=0x0000000d,.m_txt3.m_int=0};
//MSG test_singer = {.m_id.all=0x0,.m_cf.all=0x51,.m_txt1.m_int=0x12111005,.m_txt2.m_int=0x00001413,.m_txt3.m_int=0}; 

//#define send_data_len 256
//void Test_SendData(void)
//{
//	int i=0;
//        static int j=0;
//	uint8_t* data=NULL;
//	
//	
//	if(!Is_Data_Send_Ready())
//		return;
//		
////	printf("*************Test_SendData function!**********\n");
//	
//	data=(uint8_t*)malloc(send_data_len);
//	data[0]=j++;
//        if(j==256) j=0;
//	for(i=1;i<send_data_len;i++)
//		data[i]=i;
//		
//	
//	BUS_manage.DT_class=0;
//	BUS_manage.DT_priority=0;
//	BUS_manage.DT_length=send_data_len;
//	BUS_manage.DT_Src_IP_Adr=1;         //src:1 dst:2 type:1 883   src:1 dst:1 type:1 843   
//	BUS_manage.DT_Dst_IP_Adr=2;         //src:2 dst:2 type:1 885  src:2 dst:1 type:1 845
//	BUS_manage.DT_type=1;
//	BUS_manage.DT_arry=data;
//	
//	BUS_manage.Write_User2Fifo_Data(&BUS_manage,&(BUS_manage.DATA_TX_FIFO) , NULL);
//}
//
//void Test_ReceiveData(void)
//{
//	static int flag=0;
//	int i=0,j=0,num=0,length=0,singer_len=0;
//	uint8_t** pointer=NULL;
//	
//	if(!Is_Data_Receive_Ready())
//		return;
//		
//	printf("*************Test_ReceiveData function!**********\n");
//	
//	BUS_manage.DR_flag=0;
//	length=BUS_manage.DR_length;
//	num=BUS_manage.DR_num;
//	pointer=BUS_manage.DR_arry;
//
///*	if(!((length==send_data_len)||(length==4))){
//		printf("data receive length error!\n");
//		exit(1);
//	}*/
//	
//	if(num>1)
//	{
//		while(num)
//		{
//			if(num>1)
//				i=Data_max_length-8;
//			else
//				i=length;
//				
//				singer_len=i;
//			while(i)
//			{
//				if((j==15)||(i==1))
//				{
//					printf("0x%x\n",**pointer);
//					j=0;
//					i--;
//					*pointer+=1;
//				}
//				else
//				{
//					printf("0x%x ",**pointer);
//					j++;
//					i--;
//					*pointer+=1;			
//				}
//			}
//			num--;
//			*pointer-=singer_len;
////			free(*pointer);
////			*pointer=NULL;
//			pointer++;
//			length=length-Data_max_length+8;
//	
//		}
//		num=BUS_manage.DR_num;
//		pointer-=num;
////		free(pointer);
////		pointer=NULL;
//	}
//	else if(num==1)
//	{
//		i=length;
//		singer_len=i;
//			while(i)
//			{
//				if((j==15)||(i==1))
//				{
//					printf("0x%x\n",**pointer);
//					j=0;
//					i--;
//					*pointer+=1;
//				}
//				else
//				{
//					printf("0x%x ",**pointer);
//					j++;
//					i--;
//					*pointer+=1;			
//				}
//			}
//			
//			*pointer-=singer_len;
////			free(*pointer);
////			*pointer=NULL;
////			free(pointer);
////			pointer=NULL;
//	}
//	BUS_manage.DR_finish=1;
//}
//
////最简单的测试收发的函数，不同过中断，强制接收
//void Data_BUS_INIT_TEST(int value)
//{
//	int i=0,j=1;
//	uint16_t temp=0;
//	
//	while(1)
//	{	
//		printf("************************第%d次****************************\n",j);
//		temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Arb_data_status));
//		printf("Arb_data_status:   0x%x\n",temp);
//		temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rx_status));
//		printf("Data_rx_status:   0x%x\n",temp);
//		temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rx_intclr));
//		printf("Data_rx_intclr:   0x%x\n\n",temp);
//		
//	BUS.Write_Reg(BUS.filp,&(BUS.DataBus_regs->Arb_data_head_L),0X584A);//584A
//	BUS.Write_Reg(BUS.filp,&(BUS.DataBus_regs->Arb_data_head_H),0X3F73);
//	
//	for(i=0;i<value;i++)
//	{
//		BUS.Write_Reg(BUS.filp,&(BUS.DataBus_regs->Arb_data_body_L),0X1234+i);
//		temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Arb_data_body_L));
//		printf("Arb_data_body_L:   0x%x\n",temp);
//		BUS.Write_Reg(BUS.filp,&(BUS.DataBus_regs->Arb_data_body_H),0x5678+i);
//		temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Arb_data_body_H));
//		printf("Arb_data_body_H:   0x%x\n",temp);
//	}
//	
//		temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Arb_data_status));
//		printf("Arb_data_status:   0x%x\n",temp);
//		temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rx_status));
//		printf("Data_rx_status:   0x%x\n",temp);
//		temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rx_intclr));
//		printf("Data_rx_intclr:   0x%x\n\n",temp);
//		
//	BUS.Write_Reg(BUS.filp,&(BUS.DataBus_regs->Arb_data_sendrdy),0xA55A);
//	printf("-----1------\n");
//	sleep(1);
//	printf("-----2------\n");
//	
//		temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Arb_data_status));
//		printf("Arb_data_status:   0x%x\n",temp);
//		temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rx_status));
//		printf("Data_rx_status:   0x%x\n",temp);
//		temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rx_intclr));
//		printf("Data_rx_intclr:   0x%x\n\n",temp);
//		
//	temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rxhead_L));
//	printf("data rx head L:   0x%x\n",temp);
//	temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rxhead_H));
//	printf("data rx head H:   0x%x\n\n",temp);
//	for(i=0;i<value;i++)
//	{
//		temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rxbody_H));
//		printf("data rx data H:   0x%x\n",temp);
//		temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rxbody_L));
//		printf("data rx data L:   0x%x\n",temp);		
//	}
//	
//	temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rx_ctrl));
//  BUS.Write_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rx_ctrl),temp | 0x0040);
//  temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rx_ctrl));
//  BUS.Write_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rx_ctrl),temp & ~0x0040);
//  
//		temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Arb_data_status));
//		printf("Arb_data_status:   0x%x\n",temp);
//		temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rx_status));
//		printf("Data_rx_status:   0x%x\n",temp);
//		temp=BUS.Read_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rx_intclr));
//		printf("Data_rx_intclr:   0x%x\n\n",temp);
//		printf("-----3------\n");
//		BUS.Write_Reg(BUS.filp,&(BUS.DataBus_regs->Data_rx_intclr),0x0001);
//		printf("-----4------\n");
//		sleep(1);
//		printf("-----5------\n");
//		if(j==100)
//			j=1;
//		else
//			j++;
//	}
//}

extern void sendDataToIbusSimulation(char*, int);
extern int receiveDataFromIbusSimulation(char**, int*);

void Emulator_send(char* buf, int len){
	sendDataToIbusSimulation(buf,len);
	return;
}

int Emulator_recv(char** buf, int* len){
	return receiveDataFromIbusSimulation(buf,len);
}