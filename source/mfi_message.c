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

/*��ϢFIFO��ʼ������*/
//��Ϣfifo��������
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
		
	//������Դ�������е�ģ��ip��Ϣ��Ϊ���ڵ�ģ������ݴ�fifo
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
 *6.21��������:��֡���B���͸����A�������ش���Ϣ�����ܽ��б�ţ�Ҳ�������ݴ���У��������£��������ӣ��������շ��͵�
 *��֮ǰ�Ѿ���ŵ�ĳ�����ط���Ϣ��������Ϣ���������գ���ô��һ�����͵���Ŀ�ĵ�ַ����Ϣ�ı�ű����շ�����ʱ����ʵ������һ
 *��B����A�������ط���Ϣ�������ý��շ��ĵȴ�����Զ�+1�����������ط���Ϣ��ʧ�����շ��ֻᷢ�������ط������ͷ����������
 *֮ǰB����A�������ط���Ϣ������Ŀǰ���ط����ƣ��ö�Ŀ�Ķ�������������ط���Ϣ�ᱻ���ͳ�ȥ�����Ƕ��ڽ��ն��жϴ���֮��
 *���͵�Դ�˵������ط���Ϣ���Խ��б����ӡ�
 *���ϣ��Է�֡���������޸ģ�����ѡ���Ƿ�����ӵı��
*/
//��֡����
//����֡ģ����øú��������ش���Ϣʱ����Ϣ�����߳�ȡ������Ϣ�жϵ����ش���Ϣʱ��
//���ݴ�fifoȡ�����ش���Ϣ���������ش�λ
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
	
	//����<=10,��֡
	if(retCnt<=MESSAGE_MAX_LEN-2){
		msgid.m_single=0;
		pthread_mutex_lock(&(Msg_Fifo_p->lock));
		while(Msg_Fifo_p->full==MFI_TRUE)
			pthread_cond_wait(&(Msg_Fifo_p->ready),&(Msg_Fifo_p->lock));
		//fifo����
		Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_id.bit=msgid;
		Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[0]=retCnt;
		if(flag==0){
			Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[1]=Msg_Fifo_p->number[dst_ip]++;
			Msg_Fifo_p->number[dst_ip]%=(MSG_TMP_BUF_DEF_LEN-1);//�����253
		}
		else if(flag==1)
			Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[1]=MSG_TMP_BUF_DEF_LEN-1;//�̶�ʱ�䴰��Ϣ�������
		else if(flag==2)
			Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[1]=MSG_TMP_BUF_DEF_LEN; //�����
		
		//ת�淢������
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
	
	real_len=MESSAGE_MAX_LEN-3;//��������֡ʱ��һ֡��ʵ�ʳ���
	fream_num=(retCnt+real_len-1)/real_len;
	
	i=1;
	msgid.m_single=1;
	pthread_mutex_lock(&(Msg_Fifo_p->lock));
	while(retCnt>0){
		while(Msg_Fifo_p->full==MFI_TRUE)
			pthread_cond_wait(&(Msg_Fifo_p->ready),&(Msg_Fifo_p->lock));
		//fifo����
		Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_id.bit=msgid;
		Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[0]=(retCnt>real_len?real_len:retCnt);
			
		if(flag==0){
			Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[1]=Msg_Fifo_p->number[dst_ip]++;
			Msg_Fifo_p->number[dst_ip]%=(MSG_TMP_BUF_DEF_LEN-1);
		}
		else if(flag==1)
			Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[1]=MSG_TMP_BUF_DEF_LEN-1;//�̶�ʱ�䴰��Ϣ�������
		else if(flag==2)
			Msg_Fifo_p->buf[Msg_Fifo_p->wIndex].m_txt1.m_char[1]=MSG_TMP_BUF_DEF_LEN; //�����
			
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
		//������ŵ���Ϣ
//		printf("MsgSendToBus: 2!\n");
		Msg_Fifo_p->tmpbuf[dst_ip][num]=Msg_Fifo_p->buf[Msg_Fifo_p->rIndex];
		msg=&(Msg_Fifo_p->tmpbuf[dst_ip][num]);
//		printf("MsgSendToBus: 2-1!\n");
	}
	else{
		//�������Ϣ����������֡ģ�鷢�������ط�֪ͨ���̶�ʱ�䴰��Ϣ���㲥��Ϣ
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
	//�ж��Ƿ�Ϊ��֡ģ�鷢�������ط�֪ͨ ������
	if(num==MSG_TMP_BUF_DEF_LEN && msgclass==URGENT_ORDER_CLASS && msg->m_id.bit.m_type==RETRANS_APPLY_IN)
	{
//		printf("MsgSendToBus: 4!\n");
		//����Ϣ�ĵ�һ����Ϣλ�ô���˴��ط���ţ����´�������Ϣ�ı��
		num=mtmp.m_txt1.m_short[1];
		while(num!=mtmp.m_txt2.m_short[0]){
			msg=&(Msg_Fifo_p->tmpbuf[dst_ip][num]);
			msg->m_id.bit.m_retry=1;  //����ط�
			num=(num+1)%(MSG_TMP_BUF_DEF_LEN-1);
			
			//�������������ӿڣ�����Ϣ���͵�FPGA
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
		//�������������ӿڣ�����Ϣ���͵�FPGA�й̶�ʱ�䴰��ؼĴ���
		
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
		
	//�������������ӿڣ�����Ϣ���͵�FPGA
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
		//����FIFO������ֱ�ӽ�������������Ϣ
		pthread_mutex_unlock(&(Msg_Fifo_p->lock));
		//�����ǣ�������Ϣ
		
		return MFI_ERROR_BUF_FULL;
	}
	
	//�������������ӿڣ���FPGA������Ϣ������buf
	//������ҪCRC��
	#if defined(EMULATOR)||defined(HOST_TEST)
	if(Emulator_recv(&msg_p, &len)==-1){
		pthread_mutex_unlock(&(Msg_Fifo_p->lock));
//		printf("~~~~~~~~~~~~~MsgReadFromBus~~~~~~~~~~~~~~~~~~~1\n");
		return 0;
	}
	memcpy(&msg,msg_p,len);//����Ϊ������Ϣ�ĳ���
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

/*��Ϣ��֡FIFO��ʼ������*/
MfiStatus CombMsg_Fifo_Init(CombMsg_Fifo_p pointer)
{
	memset(pointer,0,sizeof(CombMsg_Fifo));
	
	pointer->queue_len=0;
	pointer->head.next=NULL;
	
	return MFI_SUCCESS;
}

//��֡����
//���ܣ��ӽ�����Ϣ����ȡ����Ϣ����������֡�������䵽�����Ự���߶Դ�������Ӧ����
//      ������ʹ�õ��й���Ϣ��֡ͷ����Ч���ȡ�֡λ����Ϣ���ο���Ϣ֡ͷ�����Լ����ݶ���
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
	//�ӵײ�ͨ��fifoȡ��һ֡��Ϣ
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
	
	//�жϱ���Ƿ���ȷ
	src_ip=mtmp.m_id.bit.m_src_addr;
	num=mtmp.m_txt1.m_char[1];
	if(num < MSG_TMP_BUF_DEF_LEN-1 && num!=Msg_Fifo_p->number[src_ip]){
		if(mtmp.m_id.bit.m_retry!=1){
			//��Ϣ��ʧ�����������ط���Ϣ���ط���Ϣ����Ϊ[Msg_Fifo_p->number[src_ip],num)֮��ı��
			//����Դ���������һ��ר�ŵĴ���ģ�飬����һ���߳��������У�����������������ģ�ͣ�
			//��֡ģ����������ж������ָ��ɴ�����ģ���������֡ģ�飬�����ط��������Ϣ
			buf[0].m_short[0]=Msg_Fifo_p->number[src_ip];
			buf[0].m_short[1]=num;
			
			#ifdef RESEND_MSG
			static int miss_num=0;
			printf("CombineFreamMsg: MISS MSG%d: [%d, %d)\n",miss_num,Msg_Fifo_p->number[src_ip],num);
			//exit(1);
			#endif
			pthread_mutex_lock(&DPFreamMsg_lock);
			DecmposeFreamMsg(src_ip, &msgTxFifo, URGENT_ORDER_DEF_PORITY, URGENT_ORDER_CLASS, RETRANS_APPLY_OUT, (MfiPBuf)buf, 4,2);
			pthread_mutex_unlock(&DPFreamMsg_lock);//�ͷ���1
			#ifdef RESEND_MSG
			printf("CombineFreamMsg: APPLY RESEND%d: [%d, %d)\n",miss_num,Msg_Fifo_p->number[src_ip],num);
			miss_num++;
			#endif
			
			Msg_Fifo_p->number[src_ip]=num+1;	 //renew the num
			Msg_Fifo_p->number[src_ip]%=(MSG_TMP_BUF_DEF_LEN-1);//�����253
		}
	}
	else if(num < MSG_TMP_BUF_DEF_LEN-1){
		Msg_Fifo_p->number[src_ip]++;
		Msg_Fifo_p->number[src_ip]%=(MSG_TMP_BUF_DEF_LEN-1);//�����253
	}

//	printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~2\n");
	
	//��֡
	if(mtmp.m_id.bit.m_single==0){
		msgclass=mtmp.m_id.bit.m_class0|mtmp.m_id.bit.m_class1<<1|mtmp.m_id.bit.m_class2<<2|mtmp.m_id.bit.m_class3<<3;
		//�ж��Ƿ�Ϊ���Ͷ˷����������ط���Ϣ������
		if(msgclass==URGENT_ORDER_CLASS && mtmp.m_id.bit.m_type==RETRANS_APPLY_OUT)
		{
			//����A���������ط��ڲ���Ϣ
			pthread_mutex_lock(&DPFreamMsg_lock);
			DecmposeFreamMsg(src_ip, &msgTxFifo, URGENT_ORDER_DEF_PORITY, URGENT_ORDER_CLASS, RETRANS_APPLY_IN, &(mtmp.m_txt1.m_char[2]), 4,2);
			pthread_mutex_unlock(&DPFreamMsg_lock);//�ͷ���1
			
			#ifdef RESEND_MSG
			printf("CombineFreamMsg: RECV APPLY: src_ip: %d\n",src_ip);
			printf("[%d, %d)\n",mtmp.m_txt1.m_short[1],mtmp.m_txt2.m_short[0]);
			#endif
			
			return MFI_SUCCESS;
		}
		
		//src_ipת�Ựid des_mfi
		Ip_to_Mfi(src_ip,&des_mfi);
		
		//�жϻỰ�Ƿ��Ѿ��رգ��Ƿ�������ʹ���У�
		if(RsrcManager==NULL)
			return MFI_SUCCESS;
		else if(SesStatusCheck(&(RsrcManager->session_list[des_mfi]))==NOINUSE)
			return MFI_SUCCESS;
			
		//�����ڴ棬ת����Ϣ
		pool_create(&MsgRecPool, sizeof(CombMsg_Head)+mtmp.m_txt1.m_char[0], &pool);
		combMsg=(CombMsg_Head_p)pool_alloc(&MsgRecPool, pool);
		memcpy(combMsg+1,mtmp.m_txt1.m_char+2,mtmp.m_txt1.m_char[0]);
		combMsg->memsize=pool->size;
		combMsg->m_id.all=mtmp.m_id.all;
		combMsg->len=mtmp.m_txt1.m_char[0];
		
		//����Ϣ����Ự����Ϣfifo������buf  ���߸�����Ϣ����������ߵĻỰ
		//������Ϣ������࣬���Ի�ȡ���������buf��Ӧ����Ϣ�������Ϣfifo���������͹���������Դ�Ự��Ϣfifo
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
	
	//����֡
	mtmp.m_id.bit.m_retry=0;
	if(num < MSG_TMP_BUF_DEF_LEN-1)
	{ //�����Ϣ�Ĵ���
		for(i=0;i<CombMsg_Fifo_p->queue_len;++i){
			combMsg=temp->next;
			if(combMsg->startnum>=MSG_TMP_BUF_DEF_LEN-1 || combMsg->m_id.all!=mtmp.m_id.all || ((combMsg->startnum+combMsg->freamnum)%(MSG_TMP_BUF_DEF_LEN-1))<=num){
//				printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~13\n");
//				printf("~~~~~~~~~~~~~~~~CombineFreamMsg: %d,%d,0x%08x\n",combMsg->startnum,combMsg->freamnum,combMsg->m_id.all);
				combMsg->time--;
				//��ʱ�������������ڴ�
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
				tgtmp=temp;     //target��last
				temp=combMsg;
			}
		}
	}
	else
	{ //�Ǳ����Ϣ�Ĵ���
		for(i=0;i<CombMsg_Fifo_p->queue_len;++i){
			combMsg=temp->next;
			if(combMsg->startnum<MSG_TMP_BUF_DEF_LEN-1 || combMsg->m_id.all!=mtmp.m_id.all){
				combMsg->time--;
				//��ʱ�������������ڴ�
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
			{	//���ط���Ϣ���м�©֡����ֱ�Ӷ���
					temp->next=combMsg->next;
					pool_create(&MsgRecPool, combMsg->memsize, &pool);
					pool_free(pool, combMsg);
					++cnt;
			}
			else{
				flag=1;
				target=combMsg;
				tgtmp=temp;     //target��last
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
			
			//src_ipת�Ựid des_mfi
			Ip_to_Mfi(src_ip,&des_mfi);
//			printf("~~~~~~~~~~~~~~~~CombineFreamMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~10\n");
			//�жϻỰ�Ƿ��Ѿ��رգ��Ƿ�������ʹ���У�
			if((RsrcManager!=NULL)&&(SesStatusCheck(&(RsrcManager->session_list[des_mfi]))==INUSE))
			{
				msgclass=mtmp.m_id.bit.m_class0|mtmp.m_id.bit.m_class1<<1|mtmp.m_id.bit.m_class2<<2|mtmp.m_id.bit.m_class3<<3;
				//����Ϣ����Ự����Ϣfifo������buf  ���߸�����Ϣ����������ߵĻỰ
				//������Ϣ������࣬���Ի�ȡ���������buf��Ӧ����Ϣ�������Ϣfifo���������͹���������Դ�Ự��Ϣfifo
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
		//�����ڴ桢�������
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

//����֮֡�����Ϣ����������������
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
