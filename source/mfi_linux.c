#include "mfiapi.h"
#include "mfi_linux.h"
#include "mfi_message.h"
#include "mfi_data.h"
#include "mfi_uart.h"
#include "mfi_gpmc_cmd.h"
#include "mfi_bus_init.h"
#include "mfi_reg.h"
#include "mfi_test_define.h"
#include "mfi_module_info.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

int fd_gpmc,fd_uart;
//int irq_flag=0;

int GPMC_OPEN(void)
{
	int fd_gpmc;
	
	if((fd_gpmc=open("/dev/fpga",O_RDWR))<0)
		{
			printf("!!!!!!!!!!!!!!OPEN GPMC ERROR!!!!!!!!!!!!!!!\n");
			exit(1);
			return -1;
		}
	else
	{
		printf("..............GPMC OPEN................ \n\r"); 
		return fd_gpmc;
	}
}

int UART_OPEN(void)
{
	int fd_uart;
	
	if((fd_uart = open("/dev/ttyO2",O_RDWR|O_NOCTTY|O_NDELAY)) < 0 ){
		printf("!!!!!!!!!!!!!!OPEN UART ERROR!!!!!!!!!!!!!!!\n");
		exit(1);
		return -1;
	}
	else{
		printf("..............UART OPEN................ \n\r"); 
		return fd_uart;
	}
}
//信号处理函数
void signal_f(int signum)
{
		int err=0;
		int irq_flag=0;//不设置成全局变量，可重入
		static data_irq_count=0;
		MfiUInt16 temp;
		
//		printf("\nsignal_f: APP message: ioctl CMD = 0x%x\n",GPMC_IRQ_GET_CMD);

	#if ((!defined(EMULATOR))&&(!defined(HOST_TEST)))
		
		if(0!=(err=ioctl(fd_gpmc, GPMC_IRQ_GET_CMD, &irq_flag)))
			printf("signal_f: GPMC ioctl failed!!!\n");
			
		if(irq_flag==1)
		{
			#ifdef ALL_TEST
			printf("signal_f: Receive message interrupt!!!\n");
			#endif
			//BUS_manage.Write_Bus2Fifo_Msg(&(BUS_manage.MSG_RX_FIFO) , &BUS);
			MsgReadFromBus(&msgRxFifo);
		}
		else if(irq_flag==2)
		{
			#ifdef ALL_TEST
			printf("signal_f: Receive data interrupt!!!\n");
			temp=Read_Reg(fd_gpmc,&(DataBus_regs->Data_rx_status)); //9.16注意修改此处
			printf("signal_f: Before Receive : Data_rx_status = 0x%x\n",temp);
			#endif
			
			if(data_irq_count<3)
			{
				MfiUInt16 i;
				//清空FIFO
				temp=Read_Reg(fd_gpmc,&(DataBus_regs->Data_rx_ctrl));
  			Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_ctrl),temp | 0x0040);
  			for(i=0;i<10;i++);
 				temp=Read_Reg(fd_gpmc,&(DataBus_regs->Data_rx_ctrl));
 				Write_Reg(fd_gpmc,&(DataBus_regs->Data_rx_ctrl),temp & ~0x0040);
 				for(i=0;i<10;i++);
		
//				BUS_manage.ReceiveData_int_flag=0;
				data_irq_count++;
				return;
			}
			
			//ReceiveBus_Data(&BUS_manage , &BUS);dataRxFifo
			DataReadFromBus(&dataRxFifo);
			
			#ifdef ALL_TEST
			temp=Read_Reg(fd_gpmc,&(DataBus_regs->Data_rx_status));
			printf("signal_f: After Receive : Data_rx_status = 0x%x\n\n",temp);
			#endif
		}
		
	#else
		MsgReadFromBus(&msgRxFifo);
	#endif
}

//信号处理函数
void signal_f1(int signum)
{
		printf("sync Success!!!\n");
}

sigset_t mask;

void Sigio_init(void)
{
	int err;
	int Signal_flag;
	
	signal(SIGIO, signal_f);  //设置信号处理函数
//	signal(SIGUSR1, signal_f1);

	#if ((!defined(EMULATOR))&&(!defined(HOST_TEST)))
  fcntl(fd_gpmc, F_SETOWN, getpid()); //设置当前进程为被异步通知的进程
  Signal_flag = fcntl(fd_gpmc, F_GETFL); //获取文件状态标志
  fcntl(fd_gpmc, F_SETFL, Signal_flag|FASYNC); //设置文件状态标志，启用异步通知
  #endif
  
  sigemptyset(&mask);
  sigaddset(&mask,SIGIO);
  if(err = pthread_sigmask(SIG_BLOCK,&mask,NULL) != 0)
  	printf("!!!!!!!!!!!!!!pthread_sigmask error!!!!!!!!!!!!!!\n");
}

void Pthread_init(void)
{
	int err;
	pthread_t tid;
	
	if(err = pthread_create(&tid,NULL,thr_sighandle,0) != 0){
  	printf("!!!!!!!!!!!!!!pthread_create error!!!!!!!!!!!!!!\n");
  	exit(1);
  }
  else
  	printf("Creat Signal Handle Pthread!\n");
  	
  if(err = pthread_create(&tid,NULL,thr_communication,0) != 0){
  	printf("!!!!!!!!!!!!!!pthread_create error!!!!!!!!!!!!!!\n");
  	exit(1);
  }
  else
  	printf("Creat Communication Pthread!\n");	
}

void* thr_sighandle(void* arg)
{
	int err,signo;
	
	for(;;)
	{
		err=sigwait(&mask,&signo);
		if(err != 0){
			printf("!!!!!!!!!!!!!!sigwait error!!!!!!!!!!!!!!\n");
		}
		
		#ifdef ALL_TEST
		printf("!!!!!!!!!!!!!!get sigio!!!!!!!!!!!!!!\n");
		#endif
		
		switch(signo){
			case SIGIO:
				signal_f(signo);
				break;
			default:
				break;
		}
	}
}

void* thr_communication(void* arg)
{
	while(1){
		MsgSendToBus(&msgTxFifo);                                 //send msg
		DataSendToBus(&dataTxFifo);                               //send data
		CombineFreamMsg(NULL, &msgRxFifo, &msgCombFifo);          //combine msg from rxfifo
		CombineFreamData(NULL, &dataRxFifo, &dataCombFifo);       //combine data from rxfifo
		
		#if !defined(HOST_TEST)
		Uart_Rx_Options(fd_uart,uart_rx_buff,1,0);                //uart communication,Uart_Command_Analyze
		#endif
	}
}

MfiStatus SysPool_Init(void){
	MfiStatus status;
	
	if(MemPools_Init(&MsgRecPool,0,10,DEF_MEM_PAGE)!=MFI_SUCCESS){
		printf("!!!!!!!!!!!!!!MemPools_Init error!!!!!!!!!!!!!!\n");
  	exit(1);
	}
	
	if((MemPools_Init(&DataSendPool,0,10,DEF_MEM_PAGE)!=MFI_SUCCESS)||(MemPools_Init(&DataRecPool,0,10,DEF_MEM_PAGE)!=MFI_SUCCESS)){
		printf("!!!!!!!!!!!!!!MemPools_Init error!!!!!!!!!!!!!!\n");
  	exit(1);
	}
}

void SystemInit(void)
{
	#if ((!defined(EMULATOR))&&(!defined(HOST_TEST)))
	fd_gpmc=GPMC_OPEN();
	#endif
	
	#if !defined(HOST_TEST)
	fd_uart = UART_OPEN(); //打开串口0读写  

  Uart_Speed_Set(fd_uart, 9600);
  Uart_Parity_Set(fd_uart,8,1,'n');
  #endif
	
	Sigio_init();
	
	if((MsgFifo_Init(&msgRxFifo)!=MFI_SUCCESS)||(MsgFifo_Init(&msgTxFifo)!=MFI_SUCCESS)){
		printf("!!!!!!!!!!!!!!MsgFifo_Init error!!!!!!!!!!!!!!\n");
		goto error1;
	}
	if((DataFifo_Init(&dataRxFifo)!=MFI_SUCCESS)||(DataFifo_Init(&dataTxFifo)!=MFI_SUCCESS)){
		printf("!!!!!!!!!!!!!!DataFifo_Init error!!!!!!!!!!!!!!\n");
		goto error2;
	}
	
	SysPool_Init();
	
	Pthread_init();
  
  #if ((!defined(EMULATOR))&&(!defined(HOST_TEST)))
	BUS_Drive_Init();   //默认基本时间周期、固定时间周期和滤波器配置总线
	#endif

	#if !defined(HOST_TEST)
	while(Module.Module_Info_p==NULL){
		Uart_Tx_Options(fd_uart,uart_tx_buff,REQUEST_ID_COMMAND,NULL); //请求注册信息
		sleep(1);
	}
	#else 
	ModuleInit(1);
	#endif
	
	//二次调用，初始化消息发送fifo的暂存fifo
	if(MsgFifo_Init(&msgTxFifo)!=MFI_SUCCESS){
		printf("!!!!!!!!!!!!!!MsgFifo_Init2 error!!!!!!!!!!!!!!\n");
		goto error2;
	}
	
	if(MfiOpenDefaultRM(NULL)!=MFI_SUCCESS){
		printf("!!!!!!!!!!!!!!MfiOpenDefaultRM error!!!!!!!!!!!!!!\n");
		goto error2;
	}
	
	//通信链路自检
	
	return MFI_SUCCESS;
	
	error2:
		DataFifo_Delete(&dataRxFifo);
		DataFifo_Delete(&dataTxFifo);
	error1:
		MsgFifo_Delete(&msgRxFifo);
		MsgFifo_Delete(&msgTxFifo);
		exit(1);
}
