#include "mfi_message.h"
#include "mfi_pool.h"
#include "mfi_rsrc_manager.h"
#include "mfi_module_info.h"
#include "mfi_system_command.h"
#include "mfi_test_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

mem_pools MsgRecPool;

MSG_Fifo msgRxFifo;
MSG_Fifo msgTxFifo;
CombMsg_Fifo msgCombFifo;

/*消息FIFO初始化函数*/
//消息fifo采用数组
MfiStatus MsgFifo_Init(MSG_Fifo* Msg_Fifo_p)
{
	static int flag=0;
	int i=0;
	
	if(flag<2){
		memset(Msg_Fifo_p,0,sizeof(MSG_Fifo));
		
		if(pthread_mutex_init(&(Msg_Fifo_p->lock),MFI_NULL)!=0)
			return MFI_ERROR_SYSTEM_ERROR;
		
		if(pthread_cond_init(&(Msg_Fifo_p->ready),MFI_NULL)!=0){
			pthread_mutex_destroy(&(Msg_Fifo_p->lock));
			return MFI_ERROR_SYSTEM_ERROR;
		}
  	
		Msg_Fifo_p->buf = (MSG_p)calloc(MSG_FIFO_DEF_LEN,sizeof(MSG));//sizeof(MSG)=16
		if(Msg_Fifo_p->buf==NULL){
			pthread_cond_destroy(&(Msg_Fifo_p->ready));
			pthread_mutex_destroy(&(Msg_Fifo_p->lock));
			return MFI_ERROR_ALLOC;
		}
		
		Msg_Fifo_p->empty  = MFI_TRUE;
		Msg_Fifo_p->full   = MFI_FALSE;
		Msg_Fifo_p->Msg_Fifo_Len = MSG_FIFO_DEF_LEN;
		Msg_Fifo_p->rIndex = 0;
		Msg_Fifo_p->wIndex = 0;
		flag++;
		
		return MFI_SUCCESS;
	}
		
	//根据资源管理器中的模块ip信息，为存在的模块分配暂存fifo
	Msg_Fifo_p->tmpbuf[HOST_IP]=(MSG_p)malloc(MSG_TMP_BUF_DEF_LEN*sizeof(MSG));
	if(Msg_Fifo_p->tmpbuf[HOST_IP]==NULL)
		return MFI_ERROR_ALLOC;
	for(;i<Module.number;i++){
		Msg_Fifo_p->tmpbuf[Module.Module_Info_p[i].mod_ip]=(MSG_p)malloc(MSG_TMP_BUF_DEF_LEN*sizeof(MSG));
		if(Msg_Fifo_p->tmpbuf[Module.Module_Info_p[i].mod_ip]==NULL)
			return MFI_ERROR_ALLOC;
	}

	flag=0;
	
		#ifdef ALL_TEST
		printf("MsgFifo_Init success!!\n");
		#endif
		
	return MFI_SUCCESS;
}

MfiStatus MsgFifo_Delete(MSG_Fifo* Msg_Fifo_p)
{
	int i=0;
	for(;i<SESSION_MAX_NUM;i++)
	{
		if(Msg_Fifo_p->tmpbuf[i]!=NULL){
			free(Msg_Fifo_p->tmpbuf[i]);
			Msg_Fifo_p->tmpbuf[i]=NULL;
		}
	}
	free(Msg_Fifo_p->buf);
	pthread_cond_destroy(&(Msg_Fifo_p->ready));
	pthread_mutex_destroy(&(Msg_Fifo_p->lock));
	
	return MFI_SUCCESS;
}

pthread_mutex_t DPFreamMsg_lock=PTHREAD_MUTEX_INITIALIZER;

/*
 *6.21发现问题:组帧检测B发送给检测A的申请重传消息，不能进行编号，也不能入暂存队列，理由如下：若编号入队，由于最终发送的
 *是之前已经编号的某个待重发消息，若该消息被正常接收，那么下一个发送到该目的地址的消息的编号被接收方接收时，其实跳过了一
 *个B发给A的申请重发消息；可以让接收方的等待编号自动+1，但是若该重发消息丢失，接收方又会发送申请重发给发送方，申请的是
 *之前B发给A的申请重发消息，按照目前的重发机制，该对目的端无意义的申请重发消息会被发送出去。但是对于接收端判断错误之后
 *发送到源端的申请重发消息可以进行编号入队。
 *综上：对分帧函数进行修改，添加选择是否编号入队的标记
*/
//分帧函数
//当组帧模块调用该函数发送重传消息时，消息发送线程取出该消息判断到是重传消息时，
//从暂存fifo取出待重传消息，并设置重传位
MfiStatus DecmposeFreamMsg(MfiUInt32 dst_ip, MSG_Fifo* Msg_Fifo_p, MfiUInt32 priorty, MfiUInt32 msgclass, MfiUInt32 msgtype, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 flag)
{
	MID_BITS msgid={0};
	int i=1,j=0;
	MfiUInt32 real_len=0,fream_num=0;
	
	msgid.m_src_addr=HOST_IP;
	msgid.m_dst_addr=dst_ip;
	msgid.m_type=msgtype;
	msgid.priority2=priorty>>2;
	msgid.priority1=priorty>>1;
	msgid.priority0=priorty;
	msgid.m_class3=msgclass>>3;
	msgid.m_class2=msgclass>>2;
	msgid.m_class1=msgclass>>1;	
	msgid.m_class0=msgclass;
	
	//长度<=10,单帧
	if(retCnt<=MESSAGE_MAX_LEN-2){
		msgid.m_single=0;
		pthread_mutex_lock(&(Msg_Fifo_p->lock));
		while(Msg_Fifo_p->full==MFI_TRUE)
			pthread_cond_wait(&(Msg_Fifo_p->ready),&(Msg_Fifo_p->lock));
		//fifo操作
		Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_id.bit=msgid;
		Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[0]=retCnt;
		if(flag==0){
			Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[1]=Msg_Fifo_p->number[dst_ip]++;
			Msg_Fifo_p->number[dst_ip]%=(MSG_TMP_BUF_DEF_LEN-1);//最大编号253
		}
		else if(flag==1)
			Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[1]=MSG_TMP_BUF_DEF_LEN-1;//固定时间窗消息，不编号
		else if(flag==2)
			Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[1]=MSG_TMP_BUF_DEF_LEN; //不编号
		
		//转存发送内容
		for(j=2;j<MESSAGE_MAX_LEN;j++)
		{
			if(j<retCnt+2)
			{
				Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[j]=*buf;
				++buf;
			}
			else
				Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[j]=0;
		}
		
		if(Msg_Fifo_p->wIndex < (Msg_Fifo_p->Msg_Fifo_Len-1))
			Msg_Fifo_p->wIndex++;
		else
			Msg_Fifo_p->wIndex=0;
		
		if(Msg_Fifo_p->wIndex == Msg_Fifo_p->rIndex)
			Msg_Fifo_p->full=MFI_TRUE;
		
		Msg_Fifo_p->empty=MFI_FALSE;
		pthread_mutex_unlock(&(Msg_Fifo_p->lock));
		
		return MFI_SUCCESS;
	}
	
	real_len=MESSAGE_MAX_LEN-3;//发送连续帧时，一帧的实际长度
	fream_num=(retCnt+real_len-1)/real_len;
	
	i=1;
	msgid.m_single=1;
	pthread_mutex_lock(&(Msg_Fifo_p->lock));
	while(retCnt>0){
		while(Msg_Fifo_p->full==MFI_TRUE)
			pthread_cond_wait(&(Msg_Fifo_p->ready),&(Msg_Fifo_p->lock));
		//fifo操作
		Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_id.bit=msgid;
		Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[0]=(retCnt>real_len?real_len:retCnt);
			
		if(flag==0){
			Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[1]=Msg_Fifo_p->number[dst_ip]++;
			Msg_Fifo_p->number[dst_ip]%=(MSG_TMP_BUF_DEF_LEN-1);
		}
		else if(flag==1)
			Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[1]=MSG_TMP_BUF_DEF_LEN-1;//固定时间窗消息，不编号
		else if(flag==2)
			Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[1]=MSG_TMP_BUF_DEF_LEN; //不编号
			
		Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[2]=i<<4 | fream_num;
		
		for(j=3;j<MESSAGE_MAX_LEN;j++)
		{
			if(j<retCnt+3)
			{
				Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[j]=*buf;
				++buf;
			}
			else
				Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[j]=0;
		}
		
		retCnt-=Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[0];
		if(Msg_Fifo_p->wIndex < (Msg_Fifo_p->Msg_Fifo_Len-1))
			Msg_Fifo_p->wIndex++;
		else
			Msg_Fifo_p->wIndex=0;
		
		if(Msg_Fifo_p->wIndex == Msg_Fifo_p->rIndex)
			Msg_Fifo_p->full=MFI_TRUE;
		
		Msg_Fifo_p->empty=MFI_FALSE;
		
		++i;
	}
	pthread_mutex_unlock(&(Msg_Fifo_p->lock));
	
	return MFI_SUCCESS;
}

MfiUInt32 sendnum=0,resend=0,reqrsend=0,recvnum=0,reqrecv=0;


MfiStatus MsgSendToBus(MSG_Fifo* Msg_Fifo_p)
{
	MfiUInt32 dst_ip=0,num=0;
	MfiUInt32 msgclass=0;
	MSG_p msg;
	MSG mtmp;
	
	#ifdef ALL_TEST
	printf("FUNCTION : MsgSendToBus!\n");
	#endif
	
	pthread_mutex_lock(&(Msg_Fifo_p->lock));
	if(Msg_Fifo_p->empty==MFI_TRUE){
		pthread_mutex_unlock(&(Msg_Fifo_p->lock));
		#ifdef ALL_TEST
		printf("MsgSendToBus: fifo empty!\n");
		#endif
		return MFI_SUCCESS;
	}
	
//	printf("MsgSendToBus: 1!\n");
	dst_ip=Msg_Fifo_p->buf[Msg_Fifo_p->rIndex].m_id.bit.m_dst_addr;
	num=Msg_Fifo_p->buf[Msg_Fifo_p->rIndex].m_txt1.m_char[1];
	if(num < MSG_TMP_BUF_DEF_LEN-1){
		//正常编号的消息
//		printf("MsgSendToBus: 2!\n");
		Msg_Fifo_p->tmpbuf[dst_ip][num]=Msg_Fifo_p->buf[Msg_Fifo_p->rIndex];
		msg=&(Msg_Fifo_p->tmpbuf[dst_ip][num]);
//		printf("MsgSendToBus: 2-1!\n");
	}
	else{
		//不编号消息，包括：组帧模块发过来的重发通知；固定时间窗消息；广播消息
		mtmp=Msg_Fifo_p->buf[Msg_Fifo_p->rIndex];
		msg=&mtmp;
	}
	
	if(Msg_Fifo_p->rIndex < (Msg_Fifo_p->Msg_Fifo_Len-1))
		Msg_Fifo_p->rIndex++;
	else
		Msg_Fifo_p->rIndex=0;
	
	if(Msg_Fifo_p->rIndex == Msg_Fifo_p->wIndex)
		Msg_Fifo_p->empty=MFI_TRUE;
	Msg_Fifo_p->full=MFI_FALSE;
	
	pthread_cond_signal(&(Msg_Fifo_p->ready));
	pthread_mutex_unlock(&(Msg_Fifo_p->lock));
//	printf("MsgSendToBus: 3!\n");
	
	msgclass=msg->m_id.bit.m_class0|msg->m_id.bit.m_class1<<1|msg->m_id.bit.m_class2<<2|msg->m_id.bit.m_class3<<3;
	//判断是否为组帧模块发过来的重发通知 ！！！
	if(num==MSG_TMP_BUF_DEF_LEN && msgclass==URGENT_ORDER_CLASS && msg->m_id.bit.m_type==RETRANS_APPLY_IN)
	{
//		printf("MsgSendToBus: 4!\n");
		//该消息的第一个消息位置存放了待重发编号，更新待发送消息的编号
		num=mtmp.m_txt1.m_short[1];
		while(num!=mtmp.m_txt2.m_short[0]){
			msg=&(Msg_Fifo_p->tmpbuf[dst_ip][num]);
			msg->m_id.bit.m_retry=1;  //标记重发
			num=(num+1)%(MSG_TMP_BUF_DEF_LEN-1);
			
			//调用总线驱动接口，将消息发送到FPGA
			#if defined(EMULATOR)||defined(HOST_TEST)
			Emulator_send((char*)msg,sizeof(MSG));
			#endif
			
			#ifdef RESEND_MSG
			printf("MsgSendToBus: RESEND: dst_ip: %d    num: %d\n",dst_ip,num-1);
			printf("RESEND: resend=%d\n",++resend);
			printf("RESEND: Fream head: 0x%8x\n",msg->m_id.all);
			printf("txt:0x%8x  ",msg->m_txt1.m_int);
			printf("0x%8x  ",msg->m_txt2.m_int);
			printf("0x%8x\n",msg->m_txt3.m_int);
			#endif
		}
		
		return MFI_SUCCESS;
	}
	else if(num==MSG_TMP_BUF_DEF_LEN-1){
		//调用总线驱动接口，将消息发送到FPGA中固定时间窗相关寄存器
		
		return MFI_SUCCESS;
	}
	
		#ifdef PRINT_MSG_FREAM_SEND
		if(num < MSG_TMP_BUF_DEF_LEN-1) sendnum++; else reqrsend++;
		printf("MsgSendToBus: sendnum=%d\n",sendnum);
		printf("FREAMSEND: Fream head: 0x%8x\n",msg->m_id.all);
		printf("txt:0x%8x  ",msg->m_txt1.m_int);
		printf("0x%8x  ",msg->m_txt2.m_int);
		printf("0x%8x\n",msg->m_txt3.m_int);
		#endif
		
	//调用总线驱动接口，将消息发送到FPGA
	#if defined(EMULATOR)||defined(HOST_TEST)
	Emulator_send((char*)msg,sizeof(MSG));
	#endif

	return MFI_SUCCESS;
}

MfiStatus MsgReadFromBus(MSG_Fifo* Msg_Fifo_p)
{
	MSG msg;
	#if defined(EMULATOR)||defined(HOST_TEST)
	char* msg_p=NULL;
	int len=0;
//	int tmp;
	#endif
	
	pthread_mutex_lock(&(Msg_Fifo_p->lock));
	
	if(Msg_Fifo_p->full==MFI_TRUE){
		//接收FIFO已满，直接解锁，并丢弃消息
		pthread_mutex_unlock(&(Msg_Fifo_p->lock));
		//清除标记，丢弃消息
		
		return MFI_ERROR_BUF_FULL;
	}
	
	//调用总线驱动接口，从FPGA接收消息，存入buf
	//可能需要CRC等
	#if defined(EMULATOR)||defined(HOST_TEST)
	if(Emulator_recv(&msg_p, &len)==-1){
		pthread_mutex_unlock(&(Msg_Fifo_p->lock));
//		printf("~~~~~~~~~~~~~MsgReadFromBus~~~~~~~~~~~~~~~~~~~1\n");
		return 0;
	}
	memcpy(&msg,msg_p,len);//长度为整个消息的长度
	free(msg_p);
	msg_p=NULL;
//	tmp=msg.m_id.bit.m_dst_addr;
//	msg.m_id.bit.m_dst_addr=msg.m_id.bit.m_src_addr;
//	msg.m_id.bit.m_src_addr=tmp;
	#endif

	Msg_Fifo_p->buf[Msg_Fifo_p->wIndex]=msg;	
	if(Msg_Fifo_p->wIndex < (Msg_Fifo_p->Msg_Fifo_Len-1))
		Msg_Fifo_p->wIndex++;
	else
		Msg_Fifo_p->wIndex=0;
		
	if(Msg_Fifo_p->wIndex == Msg_Fifo_p->rIndex)
		Msg_Fifo_p->full=MFI_TRUE;
		
	Msg_Fifo_p->empty=MFI_FALSE;
	
	pthread_mutex_unlock(&(Msg_Fifo_p->lock));
	
	#ifdef PRINT_MSG_FREAM_RECV
	if(msg.m_txt1.m_char[1]<MSG_TMP_BUF_DEF_LEN-1)  recvnum++; else reqrecv++;
	printf("MsgReadFromBus:\n");
	printf("FREAMRECV: Fream head: 0x%8x\n",msg.m_id.all);
	printf("txt:0x%8x  ",msg.m_txt1.m_int);
	printf("0x%8x  ",msg.m_txt2.m_int);
	printf("0x%8x\n",msg.m_txt3.m_int);
	#endif
		
	return MFI_SUCCESS;
}

/*消息组帧FIFO初始化函数*/
MfiStatus CombMsg_Fifo_Init(CombMsg_Fifo_p pointer)
{
	memset(pointer,0,sizeof(CombMsg_Fifo));
	
	pointer->queue_len=0;
	pointer->head.next=NULL;
	
	return MFI_SUCCESS;
}

//组帧函数
//功能：从接收消息队列取出消息，检查错误，组帧，并分配到各个会话或者对错误做相应处理
//      函数中使用的有关消息的帧头、有效长度、帧位等信息，参看消息帧头定义以及内容定义
MfiStatus CombineFreamMsg(MfiSession Mfi, MSG_Fifo* Msg_Fifo_p, CombMsg_Fifo* CombMsg_Fifo_p)
{
	MfiStatus status;
	MfiUInt32 src_ip=0,num=0,cnt=0;
	MfiUInt32 msgclass=0;
	MSG mtmp;
	pool_head_p pool=NULL;
	CombMsg_Head_p combMsg=NULL,temp=&(CombMsg_Fifo_p->head);
	CombMsg_Head_p target=NULL,tgtmp=NULL;
	int i=0,flag=0;
	MfiByte* src_addr=NULL,freampos=0,freamnum=0;
	MfiSession des_mfi;
	MTXT_REG buf[1];

//	printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~0\n");
	//从底层通信fifo取出一帧消息
	pthread_mutex_lock(&(Msg_Fifo_p->lock));
	if(Msg_Fifo_p->empty==MFI_TRUE){
		pthread_mutex_unlock(&(Msg_Fifo_p->lock));
		return MFI_SUCCESS;
	}
	
	mtmp=Msg_Fifo_p->buf[Msg_Fifo_p->rIndex];

	if(Msg_Fifo_p->rIndex < (Msg_Fifo_p->Msg_Fifo_Len-1))
		Msg_Fifo_p->rIndex++;
	else
		Msg_Fifo_p->rIndex=0;
	
	if(Msg_Fifo_p->rIndex == Msg_Fifo_p->wIndex)
		Msg_Fifo_p->empty=MFI_TRUE;
	Msg_Fifo_p->full=MFI_FALSE;
	
	pthread_mutex_unlock(&(Msg_Fifo_p->lock));

//	printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~1\n");
	
	//判断编号是否正确
	src_ip=mtmp.m_id.bit.m_src_addr;
	num=mtmp.m_txt1.m_char[1];
	if(num < MSG_TMP_BUF_DEF_LEN-1 && num!=Msg_Fifo_p->number[src_ip]){
		if(mtmp.m_id.bit.m_retry!=1){
			//消息丢失，发送申请重发消息，重发消息内容为[Msg_Fifo_p->number[src_ip],num)之间的编号
			//可针对错误处理建立一个专门的处理模块，放在一个线程里面运行，采用生产者消费者模型，
			//组帧模块仅仅往其中丢入错误指令，由错误处理模块来争夺分帧模块，发送重发申请等消息
			buf[0].m_short[0]=Msg_Fifo_p->number[src_ip];
			buf[0].m_short[1]=num;
			
			#ifdef RESEND_MSG
			static int miss_num=0;
			printf("CombineFreamMsg: MISS MSG%d: [%d, %d)\n",miss_num,Msg_Fifo_p->number[src_ip],num);
			//exit(1);
			#endif
			pthread_mutex_lock(&DPFreamMsg_lock);
			DecmposeFreamMsg(src_ip, &msgTxFifo, URGENT_ORDER_DEF_PORITY, URGENT_ORDER_CLASS, RETRANS_APPLY_OUT, (MfiPBuf)buf, 4,2);
			pthread_mutex_unlock(&DPFreamMsg_lock);//释放锁1
			#ifdef RESEND_MSG
			printf("CombineFreamMsg: APPLY RESEND%d: [%d, %d)\n",miss_num,Msg_Fifo_p->number[src_ip],num);
			miss_num++;
			#endif
			
			Msg_Fifo_p->number[src_ip]=num+1;	 //renew the num
			Msg_Fifo_p->number[src_ip]%=(MSG_TMP_BUF_DEF_LEN-1);//最大编号253
		}
	}
	else if(num < MSG_TMP_BUF_DEF_LEN-1){
		Msg_Fifo_p->number[src_ip]++;
		Msg_Fifo_p->number[src_ip]%=(MSG_TMP_BUF_DEF_LEN-1);//最大编号253
	}

//	printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~2\n");
	
	//单帧
	if(mtmp.m_id.bit.m_single==0){
		msgclass=mtmp.m_id.bit.m_class0|mtmp.m_id.bit.m_class1<<1|mtmp.m_id.bit.m_class2<<2|mtmp.m_id.bit.m_class3<<3;
		//判断是否为发送端发来的申请重发消息！！！
		if(msgclass==URGENT_ORDER_CLASS && mtmp.m_id.bit.m_type==RETRANS_APPLY_OUT)
		{
			//向检测A发送申请重发内部消息
			pthread_mutex_lock(&DPFreamMsg_lock);
			DecmposeFreamMsg(src_ip, &msgTxFifo, URGENT_ORDER_DEF_PORITY, URGENT_ORDER_CLASS, RETRANS_APPLY_IN, &(mtmp.m_txt1.m_char[2]), 4,2);
			pthread_mutex_unlock(&DPFreamMsg_lock);//释放锁1
			
			#ifdef RESEND_MSG
			printf("CombineFreamMsg: RECV APPLY: src_ip: %d\n",src_ip);
			printf("[%d, %d)\n",mtmp.m_txt1.m_short[1],mtmp.m_txt2.m_short[0]);
			#endif
			
			return MFI_SUCCESS;
		}
		
		//src_ip转会话id des_mfi
		Ip_to_Mfi(src_ip,&des_mfi);
		
		//判断会话是否已经关闭（是否在正常使用中）
		if(RsrcManager==NULL)
			return MFI_SUCCESS;
		else if(SesStatusCheck(&(RsrcManager->session_list[des_mfi]))==NOINUSE)
			return MFI_SUCCESS;
			
		//申请内存，转存消息
		pool_create(&MsgRecPool, sizeof(CombMsg_Head)+mtmp.m_txt1.m_char[0], &pool);
		combMsg=(CombMsg_Head_p)pool_alloc(&MsgRecPool, pool);
		memcpy(combMsg+1,mtmp.m_txt1.m_char+2,mtmp.m_txt1.m_char[0]);
		combMsg->memsize=pool->size;
		combMsg->m_id.all=mtmp.m_id.all;
		combMsg->len=mtmp.m_txt1.m_char[0];
		
		//！消息存入会话的消息fifo或属性buf  或者根据消息大类存入总线的会话
		//根据消息大类分类，属性获取类存入属性buf，应用消息类存入消息fifo，其他类型归属总线资源会话消息fifo
		if(msgclass==APP_MSG_CLASS){
			status=SesMsgFifoWrite(&(RsrcManager->session_list[des_mfi].msg_rfifo), combMsg);
		}
		else if(msgclass==MSG_GET_CLASS){
			#ifdef ATTR_MECH
			printf("CombineFreamMsg: GET ATTR\n");
			#endif
			pthread_mutex_lock(&(RsrcManager->session_list[des_mfi].attr_get_buf.lock));   
			RsrcManager->session_list[des_mfi].attr_get_buf.buf=combMsg;
			pthread_cond_signal(&(RsrcManager->session_list[des_mfi].attr_get_buf.ready));
			pthread_mutex_unlock(&(RsrcManager->session_list[Mfi].attr_get_buf.lock));
			status=MFI_SUCCESS;
		}
		else{
			status=SesMsgFifoWrite(&(RsrcManager->session_list[RsrcManager->bus_rsrc->session].msg_rfifo), combMsg);
		}
		
		SesStatusFree(&(RsrcManager->session_list[des_mfi]));
		
		if(status!=MFI_SUCCESS){
			pool_create(&MsgRecPool, combMsg->memsize, &pool);
			pool_free(pool, combMsg);
		}
			
		return status;;
	}

//	printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~3\n");
	
	//连续帧
	mtmp.m_id.bit.m_retry=0;
	if(num < MSG_TMP_BUF_DEF_LEN-1)
	{ //编号消息的处理
		for(i=0;i<CombMsg_Fifo_p->queue_len;++i){
			combMsg=temp->next;
			if(combMsg->startnum>=MSG_TMP_BUF_DEF_LEN-1 || combMsg->m_id.all!=mtmp.m_id.all || ((combMsg->startnum+combMsg->freamnum)%(MSG_TMP_BUF_DEF_LEN-1))<=num){
//				printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~13\n");
//				printf("~~~~~~~~~~~~~~~~CombineFreamMsg: %d,%d,0x%08x\n",combMsg->startnum,combMsg->freamnum,combMsg->m_id.all);
				combMsg->time--;
				//超时，丢弃，回收内存
				if(combMsg->time==0){
					#ifdef ALL_TEST 
					printf("CombineFreamMsg: OUT TIME! DISCARD THE MSG!! ID: 0x%8x\n",combMsg->m_id.all);
					#endif
					temp->next=combMsg->next;
					pool_create(&MsgRecPool, combMsg->memsize, &pool);
					pool_free(pool, combMsg);
					++cnt;
				}
				else
					temp=combMsg;
			}
			else{
//				printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~14\n");
				flag=1;
				target=combMsg;
				tgtmp=temp;     //target的last
				temp=combMsg;
			}
		}
	}
	else
	{ //非编号消息的处理
		for(i=0;i<CombMsg_Fifo_p->queue_len;++i){
			combMsg=temp->next;
			if(combMsg->startnum<MSG_TMP_BUF_DEF_LEN-1 || combMsg->m_id.all!=mtmp.m_id.all){
				combMsg->time--;
				//超时，丢弃，回收内存
				if(combMsg->time==0){
					#ifdef ALL_TEST 
					printf("CombineFreamMsg: OUT TIME! DISCARD THE MSG!! ID: 0x%8x\n",combMsg->m_id.all);
					#endif
					temp->next=combMsg->next;
					pool_create(&MsgRecPool, combMsg->memsize, &pool);
					pool_free(pool, combMsg);
					++cnt;
				}
				else
					temp=combMsg;
			}
			else if(combMsg->cnt + (mtmp.m_txt1.m_char[2]>>4) != (combMsg->freamnum + 1))
			{	//非重发消息的中间漏帧，可直接丢弃
					temp->next=combMsg->next;
					pool_create(&MsgRecPool, combMsg->memsize, &pool);
					pool_free(pool, combMsg);
					++cnt;
			}
			else{
				flag=1;
				target=combMsg;
				tgtmp=temp;     //target的last
				temp=combMsg;
			}
		}
	}
	
	CombMsg_Fifo_p->queue_len-=cnt;
	if(flag==1){
//		printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~4\n");
		freampos=(mtmp.m_txt1.m_char[2]>>4)-1;
		src_addr=(MfiByte*)(target+1)+freampos*(MESSAGE_MAX_LEN-3);
		memcpy(src_addr,mtmp.m_txt1.m_char+3,mtmp.m_txt1.m_char[0]);
		target->cnt--;
//		printf("~~~~~~~~~~~~~~~~cnt=====%d~~~~~~~~~~~~~~~~~~~~~~~~~\n",target->cnt);
		target->len+=mtmp.m_txt1.m_char[0];
		if(target->cnt==1){
			tgtmp->next=target->next;
			CombMsg_Fifo_p->queue_len--;
			
			//src_ip转会话id des_mfi
			Ip_to_Mfi(src_ip,&des_mfi);
//			printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~10\n");
			//判断会话是否已经关闭（是否在正常使用中）
			if((RsrcManager!=NULL)&&(SesStatusCheck(&(RsrcManager->session_list[des_mfi]))==INUSE))
			{
				msgclass=mtmp.m_id.bit.m_class0|mtmp.m_id.bit.m_class1<<1|mtmp.m_id.bit.m_class2<<2|mtmp.m_id.bit.m_class3<<3;
				//！消息存入会话的消息fifo或属性buf  或者根据消息大类存入总线的会话
				//根据消息大类分类，属性获取类存入属性buf，应用消息类存入消息fifo，其他类型归属总线资源会话消息fifo
//				printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~11\n");

				if(msgclass==APP_MSG_CLASS){
					status=SesMsgFifoWrite(&(RsrcManager->session_list[des_mfi].msg_rfifo), target);
					printf("CombineFreamMsg: GET APP MSG!!!!!!!!!\n");
				}
				else if(msgclass==MSG_GET_CLASS){
					#ifdef ATTR_MECH
					printf("CombineFreamMsg: GET ATTR\n");
					#endif
					pthread_mutex_lock(&(RsrcManager->session_list[des_mfi].attr_get_buf.lock));   
					RsrcManager->session_list[des_mfi].attr_get_buf.buf=target;
					pthread_cond_signal(&(RsrcManager->session_list[des_mfi].attr_get_buf.ready));
					pthread_mutex_unlock(&(RsrcManager->session_list[Mfi].attr_get_buf.lock));
					status = MFI_SUCCESS;
				}
				else{
					status=SesMsgFifoWrite(&(RsrcManager->session_list[RsrcManager->bus_rsrc->session].msg_rfifo), target);
				}

				SesStatusFree(&(RsrcManager->session_list[des_mfi]));
				
				if(status!=MFI_SUCCESS){
					pool_create(&MsgRecPool, target->memsize, &pool);
					pool_free(pool, target);
				}
				
				return status;
			}

//			printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~12\n");
			pool_create(&MsgRecPool, target->memsize, &pool);
			pool_free(pool, target);
			return MFI_SUCCESS;
		}
	}
	else{
//		printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~5\n");
		//分配内存、存入队列
		freamnum=mtmp.m_txt1.m_char[2] & 0x0F;
		freampos=(mtmp.m_txt1.m_char[2] >> 4)-1;
		
		if(num>=(MSG_TMP_BUF_DEF_LEN-1) && freampos!=0){
//			printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~6\n");
			return MFI_SUCCESS;
		}

//		printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~7\n");
		pool_create(&MsgRecPool, sizeof(CombMsg_Head)+freamnum*(MESSAGE_MAX_LEN-3), &pool);
		combMsg=(CombMsg_Head_p)pool_alloc(&MsgRecPool, pool);
//		printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~8\n");
		src_addr=(MfiByte*)(combMsg+1)+freampos*(MESSAGE_MAX_LEN-3);
		memcpy(src_addr,mtmp.m_txt1.m_char+3,mtmp.m_txt1.m_char[0]);
		combMsg->memsize=pool->size;
		combMsg->time=COMBMSG_KEEP_TIME;
		combMsg->m_id.all=mtmp.m_id.all;
		combMsg->freamnum=freamnum;
		combMsg->startnum=num-freampos;
//		printf("~~~~~~~~~~~char=0x%2x,startnum=%d,freamnum=%d,freampos=%d~~~~~~~~~~\n",mtmp.m_txt1.m_char[2],combMsg->startnum,freamnum,freampos);
		combMsg->cnt=freamnum;
		combMsg->len=mtmp.m_txt1.m_char[0];
		
		combMsg->next=CombMsg_Fifo_p->head.next;
		CombMsg_Fifo_p->head.next=combMsg;
		CombMsg_Fifo_p->queue_len++;
		
//		printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~9\n");
	}

	return MFI_SUCCESS;
}

MfiStatus MfiCombMsgFree(MfiByte* buf)
{
	MfiUInt16 memsize=0;
	pool_head_p pool=NULL;
	CombMsg_Head_p combMsg=NULL;
	
	combMsg=(CombMsg_Head_p)buf-1;
	memsize=combMsg->memsize;
	//printf("FUNCTION : MfiCombMsgFree------1\n");
	memset(combMsg,0,memsize);
	//printf("FUNCTION : MfiCombMsgFree------2\n");
	pool_create(&MsgRecPool, memsize, &pool);
	//printf("FUNCTION : MfiCombMsgFree------3\n");
	if(pool_free(pool, combMsg)==-1){
		free(combMsg);
	}
	//printf("FUNCTION : MfiCombMsgFree------4\n");
	return MFI_SUCCESS;
}

//将组帧之后的消息解析出各部分内容
MfiStatus CombMsgChange(CombMsg_Head_p combMsg, MfiPUInt32 msgclass, MfiPUInt32 msgtype, MfiPUInt32 msgsrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt)
{
	if(msgclass!=NULL)
		*msgclass=combMsg->m_id.bit.m_class0|combMsg->m_id.bit.m_class1<<1|combMsg->m_id.bit.m_class2<<2|combMsg->m_id.bit.m_class3<<3;
		
	if(msgtype!=NULL)
		*msgtype=combMsg->m_id.bit.m_type;
		
	if(msgsrcaddr!=NULL)
		*msgsrcaddr=combMsg->m_id.bit.m_src_addr;
	
	*bufp=(MfiPBuf)(combMsg+1);
	*retCnt=combMsg->len;
	
	return MFI_SUCCESS;
}

MfiStatus Mfi_to_Ip(MfiSession Mfi,MfiPUInt32 ip){
	MfiPSessionInfo psession=NULL;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	if(psession->rsrc_type==MODULE_RSRC_TYPE){
		*ip=((MfiPModuleRsrcNodeInfo)(psession->rsrc))->ip;
		return MFI_SUCCESS;
	}
	else if(psession->rsrc_type==BUS_RSRC_TYPE){
		*ip=HOST_IP;
		return MFI_SUCCESS;
	}
}

MfiStatus Ip_to_Mfi(MfiUInt32 ip,MfiPSession Mfi){
	MfiPModuleRsrcNodeInfo node;
	
	for(node=RsrcManager->module_rsrc;node!=NULL;node=node->next){
		if(node->ip!=ip)
			continue;
		*Mfi=node->session;
		return MFI_SUCCESS;
	}
	
	if(ip==HOST_IP)
		*Mfi=RsrcManager->bus_rsrc->session;
		
	return MFI_SUCCESS;
}

