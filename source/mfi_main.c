#include "mfi_linux.h"
#include "mfiapi.h"
#include "mfi_test_define.h"
#include "mfi_rsrc_manager.h"
#include "mfi_module_info.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(EMULATOR)||defined(HOST_TEST)
extern void initBusModules();
extern int initSuccessFlag;//初始化成功标志位，正为初始化成功，负为失败，0为未操作
#endif

#ifdef EMULATOR
#define FUN_MAIN mymain
#else
#define FUN_MAIN main
#endif

extern MfiUInt32 sendnum,resend,reqrsend,recvnum,reqrecv;


int FUN_MAIN(void)
{
	int flag=0,receive_flag=0,i=0,number=1;
	MfiSession rm_mfi,mod_mfi[31];
	MfiStatus status;
	MfiPModuleRsrcNodeInfo moudle;
	
	//系统初始化
	SystemInit();
	
	#if defined(EMULATOR)||defined(HOST_TEST)
	initBusModules();  	//进行虚拟前端模块的总体初始化
	initSuccessFlag = 1;
	printf("emulator bus init success!!!\n");
	#endif
	
	//打开资源管理器
	if((status=MfiOpenDefaultRM(&rm_mfi))!=MFI_SUCCESS){
		printf("ERROR: MfiOpenDefaultRM: status=%d\n",status);
		return -1;
	}
	else
		printf("MfiOpenDefaultRM success!\n");

	number=Module.number;
	
	//打开虚拟资源  RsrcManager->bus_rsrc->rsrc_name
	//printf("RSRC NAME: %s\n", RsrcManager->module_rsrc->rsrc_name);
	moudle=RsrcManager->module_rsrc;
	while(i<number){
		if((status=MfiOpen(rm_mfi, moudle->rsrc_name, &mod_mfi[i]))!=MFI_SUCCESS){
			printf("ERROR: MfiOpen: status=%d\n",status);
			return -1;
		}
		else
			printf("MfiOpen success : mfi=%d\n", mod_mfi[i]);


		//注册事件处理回调函数	
		if((status=MfiInstallHandler(mod_mfi[i], MFI_EVENT_IO_COMPLETION, test_handler, NULL))!=MFI_SUCCESS){
			printf("ERROR: InstallHandler: status=%d\n",status);
			return -1;
		}
		else
			printf("InstallHandler success!\n");
	
		//使能异步IO事件
		if((status=MfiEnableEvent(mod_mfi[i], MFI_EVENT_IO_COMPLETION, MFI_HNDLR))!=MFI_SUCCESS){
			printf("ERROR: MfiEnableEvent: status=%d\n",status);
			return -1;
		}
		else
			printf("MfiEnableEvent success!\n");

		i++;
		moudle=moudle->next;
		//Test_ReceiveMsgAsync(mod_mfi);
	}
	
	while(1)
	{
		
		if(receive_flag==10000000)
		{
			printf("************RUN***************\n");
			receive_flag=0;
		}
		receive_flag++;
	
		#ifdef SEND_MSG_SYNC
		static int Msg_sync_flag=0;
		//Test_ReceiveMsg(mod_mfi[0],5);
		Msg_sync_flag++;
		if(Msg_sync_flag==600000){
			if((status=Test_SendMsg(mod_mfi[0]))!=MFI_SUCCESS){
				printf("ERROR: Test_SendMsg: status=%d\n",status);
			}
			else
				;
			//	printf("Test_SendMsg success!\n");
			Msg_sync_flag=0;
		}
		#endif
		
		#ifdef RECV_MSG_ASYNC
		static int Msg_async_flag_recv=0;
		if(Msg_async_flag_recv==500000){
			Test_ReceiveMsgAsync(mod_mfi[0]);
			Msg_async_flag_recv=0;
		}
		Msg_async_flag_recv++;
		#endif
		
		#ifdef SEND_MSG_ASYNC
		static int Msg_async_flag_send=0;
		if(Msg_async_flag_send==400000){
			Test_SendMsgAsync(mod_mfi[0]);
			Msg_async_flag_send=0;
		}
		Msg_async_flag_send++;
		#endif

		#ifdef TEST_RESEND
		i=0;
		while(i<number){
			test_resend(mod_mfi[i]);
			i++;
		}
		//sleep(5);
		printf("sendnum = %d, reqresend = %d, resend = %d\n",sendnum,reqrsend,resend);
		printf("recvnum = %d, reqrecv = %d\n",recvnum,reqrecv);
		//sleep(5);
		#endif

		#ifdef TEST_ASYNC
		 test_async_io(mod_mfi[0]);
		#endif
/*		#ifdef test_gpmc
		Test_Gpmc_Fun();
  	#endif
  	 
  	#ifdef test_SendMsg
  	Test_ReceiveMsg();

  	if(flag==0)
  	{
  		Test_SendMsg();
  		flag=1;
  	}
  	#endif
  	
  	#ifdef test_Art_Msg_Send
  	static int Msg_flag=0;
  	Test_ReceiveMsg();
  	
  	Msg_flag++;
  	
  	if(Msg_flag==60000)   //3000000
  	{
//  		sleep(5);
  		Test_SendMsg();
  		Msg_flag=0;
  	}
  	
  	if(Msg_flag==2)
  	{
 // 		BUS_manage.Write_Bus2Fifo_Msg(&(BUS_manage.MSG_RX_FIFO) , &BUS);
  	}
  	#endif
  	
  	#ifdef test_Art_Data_Send
  	static int Data_flag=0;
  	Test_ReceiveData();
  	Data_flag++;
  	
  	if(Data_flag==3000000)
  	{
//  		sleep(5);
  		Test_SendData();
  		receive_flag=1;
  		Data_flag=0;
  	}

  	#endif
  	
//  	Test_ReceiveMsg();
 // 	Test_ReceiveData();
 */
	}
	
//	close(fd_gpmc);
	
	return 0;
}
