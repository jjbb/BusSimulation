#include "mfi_io.h"
#include "mfi_attribute.h"
#include "mfi_session.h"
#include "mfi_rsrc_manager.h"
#include "mfi_message.h"
#include "mfi_data.h"
#include "mfi_system_command.h"
#include "mfi_test_define.h"
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//ͬ������Ϣ����
//���ܣ��ú������ڿ��ŵ���ͨ��Դ����������Դ������Ϣ�Ķ�ȡ
//      (����������Դ�������ȡ��Ϣ��Դ��ַ����Ϣ�Ĵ��ࣨ����Ϊ������������ͣ���)
MfiStatus RsrcSessionReadMsg(MfiSession Mfi, MfiPUInt32 msgtype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiUInt32 timeout)
{
	MfiStatus status;
	MfiPSessionInfo psession=NULL;
	struct timespec now;
	int i=0;
	MfiPMessage msgp=NULL;

	printf("~~~~~~~~~~~~~~~~RsrcSessionReadMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~0\n");
	
	psession=&(RsrcManager->session_list[Mfi]);
	if(timeout==MFI_TMO_USEATTRVAL)
	{
		if((i=FindAttr(MFI_ATTR_TMO_VALUE, RsrcManager->session_list[Mfi].rsrc_attr, RsrcManager->session_list[Mfi].attr_amount))==-1){
			return MFI_ERROR_NSUP_ATTR;
		}
		timeout=RsrcManager->session_list[Mfi].rsrc_attr[i].attr_val.attr_ui;
		printf("timeout:  %d\n",timeout);
	}

	printf("~~~~~~~~~~~~~~~~RsrcSessionReadMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~1\n");
	
	if(timeout>MFI_TMO_IMMEDIATE && timeout<MFI_TMO_INFINITE){
		clock_gettime(CLOCK_REALTIME,&now);
		now.tv_sec=now.tv_sec+(timeout*1000000+now.tv_nsec)/1000000000;
		now.tv_nsec=(timeout*1000000+now.tv_nsec)%1000000000;
	}
		
	pthread_mutex_lock(&(psession->msg_rfifo.lock));
	while(psession->msg_rfifo.empty==MFI_TRUE){
		if(timeout==MFI_TMO_IMMEDIATE){
			pthread_mutex_unlock(&(psession->msg_rfifo.lock));
			return MFI_ERROR_NO_DATA;
		}
		else if(timeout==MFI_TMO_INFINITE){
			pthread_cond_wait(&(psession->msg_rfifo.ready),&(psession->msg_rfifo.lock));
		}
		else{
			printf("~~~~~~~~~~~~~~~~RsrcSessionReadMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~2\n");
			if((status=pthread_cond_timedwait(&(psession->msg_rfifo.ready),&(psession->msg_rfifo.lock),&now))==ETIMEDOUT)
				goto error;
			printf("pthread_cond_timedwait : status=0x%x  !!!!!!!!!\n",status);
			printf("now.tv_sec=%d, now.tv_nsec=%d !!!!!!!!!\n",now.tv_sec,now.tv_nsec);
		}
	}
	
	//��fifo��ȡ��Ϣ
	msgp=psession->msg_rfifo.buf[psession->msg_rfifo.rindex];
	if(psession->msg_rfifo.rindex < psession->msg_rfifo.buf_len-1)
		psession->msg_rfifo.rindex++;
	else
		psession->msg_rfifo.rindex=0;
		
	if(psession->msg_rfifo.rindex==psession->msg_rfifo.windex)
		psession->msg_rfifo.empty=MFI_TRUE;
	psession->msg_rfifo.full=MFI_FALSE;
	
	pthread_mutex_unlock(&(psession->msg_rfifo.lock));
	printf("~~~~~~~~~~~~~~~~RsrcSessionReadMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~3\n");
	CombMsgChange(msgp, NULL, msgtype, NULL, bufp, retCnt);
	
	return MFI_SUCCESS;
	
	error:
		printf("~~~~~~~~~~~~~~~~RsrcSessionReadMsg~~~~~~~~~~~~~~~~~~~~~~~~~~~4\n");
		pthread_mutex_unlock(&(psession->msg_rfifo.lock));
		
		return MFI_ERROR_TMO;
}

//ͬ�������ݺ���
//���ܣ��ú������ڿ��ŵ���ͨ��Դ����������Դ�������ݵĶ�ȡ
MfiStatus RsrcSessionReadData(MfiSession Mfi, MfiPUInt32 datatype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiUInt32 timeout)
{
	MfiPSessionInfo psession=NULL;
	struct timespec now;
	MfiPdata datap=NULL;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	if(timeout==MFI_TMO_USEATTRVAL)
	{
		if((i=FindAttr(MFI_ATTR_TMO_VALUE, RsrcManager->session_list[Mfi].rsrc_attr, RsrcManager->session_list[Mfi].attr_amount))==-1){
			return MFI_ERROR_NSUP_ATTR;
		}
		timeout=RsrcManager->session_list[Mfi].rsrc_attr[i].attr_val.attr_ui;
	}
	
	if(timeout>MFI_TMO_IMMEDIATE && timeout<MFI_TMO_INFINITE){
		clock_gettime(CLOCK_REALTIME,&now);
		now.tv_sec=now.tv_sec+(timeout*1000000+now.tv_nsec)/1000000000;
		now.tv_nsec=(timeout*1000000+now.tv_nsec)%1000000000;
	}
	
	pthread_mutex_lock(&(psession->data_rfifo.lock));
	while(psession->data_rfifo.empty==MFI_TRUE){
		if(timeout==MFI_TMO_IMMEDIATE){
			pthread_mutex_unlock(&(psession->data_rfifo.lock));
			return MFI_ERROR_NO_DATA;
		}
		else if(timeout==MFI_TMO_INFINITE){
			pthread_cond_wait(&(psession->data_rfifo.ready),&(psession->data_rfifo.lock));
		}
		else{
			if(pthread_cond_timedwait(&(psession->data_rfifo.ready),&(psession->data_rfifo.lock),&now)==ETIMEDOUT)
				goto error;
		}
	}
	
	//��fifo��ȡ��Ϣ
	datap=psession->data_rfifo.buf[psession->data_rfifo.rindex];
	if(psession->data_rfifo.rindex < psession->data_rfifo.buf_len-1)
		psession->data_rfifo.rindex++;
	else
		psession->data_rfifo.rindex=0;
		
	if(psession->data_rfifo.rindex==psession->data_rfifo.windex)
		psession->data_rfifo.empty=MFI_TRUE;
	psession->data_rfifo.full=MFI_FALSE;
	
	pthread_mutex_unlock(&(psession->data_rfifo.lock));
	CombDataChange(datap, NULL, datatype, NULL, bufp, retCnt);	
	
	return MFI_SUCCESS;
	
	error:
		pthread_mutex_unlock(&(psession->data_rfifo.lock));
		return MFI_ERROR_TMO;
}

//ͬ��д��Ϣ����
//���ܣ��ú������ڿ��ŵ���ͨ��Դ����������Դ������Ϣ�ķ���
MfiStatus RsrcSessionWriteMsg(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgtype, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 timeout, MfiUInt32 sendtype)
{
	MfiPSessionInfo psession=NULL;
	struct timespec now;
	MfiUInt32 ip=0;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	if(timeout==MFI_TMO_USEATTRVAL)
	{
		if((i=FindAttr(MFI_ATTR_TMO_VALUE, RsrcManager->session_list[Mfi].rsrc_attr, RsrcManager->session_list[Mfi].attr_amount))==-1){
			return MFI_ERROR_NSUP_ATTR;
		}
		timeout=RsrcManager->session_list[Mfi].rsrc_attr[i].attr_val.attr_ui;
	}
	
	if(timeout>MFI_TMO_IMMEDIATE && timeout<MFI_TMO_INFINITE){
		clock_gettime(CLOCK_REALTIME,&now);
		now.tv_sec=now.tv_sec+(timeout*1000000+now.tv_nsec)/1000000000;
		now.tv_nsec=(timeout*1000000+now.tv_nsec)%1000000000;
	}
		
	if(timeout==MFI_TMO_IMMEDIATE){
		if(pthread_mutex_trylock(&DPFreamMsg_lock)!=0)   //��ȡ�ײ�ͨ��֡FIFO����1
			return MFI_ERROR_TMO;
	}
	else if(timeout==MFI_TMO_INFINITE)
		pthread_mutex_lock(&DPFreamMsg_lock);            //��ȡ�ײ�ͨ��֡FIFO����1
	else{
		if(pthread_mutex_timedlock(&DPFreamMsg_lock,&now)==ETIMEDOUT)  //��ȡ�ײ�ͨ��֡FIFO����1��������ռ��֡ģ��
			return MFI_ERROR_TMO;
	}
	
	if(priorty==0) priorty=APP_MSG_DEF_PORITY;
	//���÷�֡��������ȡ��2�����Ựת��Ϊip������֡�ռ䣬��֡��д��fifo���ͷ���2
	Mfi_to_Ip(Mfi,&ip);
	//sendflag==0:��ͨ����;1:�̶�ʱ�䴰����
	DecmposeFreamMsg(ip, &msgTxFifo, priorty, APP_MSG_CLASS, msgtype, buf, retCnt,sendtype);
	
	pthread_mutex_unlock(&DPFreamMsg_lock);//�ͷ���1
	return MFI_SUCCESS;
}

//ͬ��д���ݺ���
//���ܣ��ú������ڿ��ŵ���ͨ��Դ����������Դ�������ݵķ���
MfiStatus RsrcSessionWriteData(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 datatype, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 timeout, MfiUInt32 sendtype)
{
	MfiPSessionInfo psession=NULL;
	struct timespec now;
	MfiUInt32 ip=0;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	if(timeout==MFI_TMO_USEATTRVAL)
	{
		if((i=FindAttr(MFI_ATTR_TMO_VALUE, RsrcManager->session_list[Mfi].rsrc_attr, RsrcManager->session_list[Mfi].attr_amount))==-1){
			return MFI_ERROR_NSUP_ATTR;
		}
		timeout=RsrcManager->session_list[Mfi].rsrc_attr[i].attr_val.attr_ui;
	}
	
	if(timeout>MFI_TMO_IMMEDIATE && timeout<MFI_TMO_INFINITE){
		clock_gettime(CLOCK_REALTIME,&now);
		now.tv_sec=now.tv_sec+(timeout*1000000+now.tv_nsec)/1000000000;
		now.tv_nsec=(timeout*1000000+now.tv_nsec)%1000000000;
	}
		
	if(timeout==MFI_TMO_IMMEDIATE){
		if(pthread_mutex_trylock(&(DPFreamData_lock))!=0)   //��ȡ�ײ�ͨ��֡FIFO����1
			return MFI_ERROR_TMO;
	}
	else if(timeout==MFI_TMO_INFINITE)
		pthread_mutex_lock(&(DPFreamData_lock));            //��ȡ�ײ�ͨ��֡FIFO����1
	else{
		if(pthread_mutex_timedlock(&DPFreamData_lock,&now)==ETIMEDOUT)  //��ȡ�ײ�ͨ��֡FIFO����1��������ռ��֡ģ��
			return MFI_ERROR_TMO;
	}
		
	if(priorty==0) priorty=APP_DATA_DEF_PORITY;
	//���÷�֡��������ȡ��2�����Ựת��Ϊip������֡�ռ䣬��֡��д��fifo���ͷ���2
	Mfi_to_Ip(Mfi,&ip);
	//sendflag==0:��ͨ����;1:�̶�ʱ�䴰����
	DecmposeFreamData(ip, &dataTxFifo, priorty, APP_DATA_CLASS, datatype, buf, retCnt, sendtype);
	
	pthread_mutex_unlock(&DPFreamData_lock);//�ͷ���1
	return MFI_SUCCESS;
}

//�첽������г�ʼ������
MfiStatus AsyncQueueInit(MfiPAsyncQueue queue)
{
	if(pthread_mutex_init(&(queue->lock),MFI_NULL)!=0){
		return MFI_ERROR_SYSTEM_ERROR;
	}
	
	if(pthread_cond_init(&(queue->ready),MFI_NULL)!=0){
		pthread_mutex_destroy(&(queue->lock));
		return MFI_ERROR_SYSTEM_ERROR;
	}
	
	queue->msg_task_queue.queue_len=0;
	queue->msg_task_queue.queue_head.next=&(queue->msg_task_queue.queue_head);
	queue->msg_task_queue.queue_head.last=&(queue->msg_task_queue.queue_head);
	queue->msg_task_queue.queue_tail=&(queue->msg_task_queue.queue_head);
	
	queue->data_task_queue.queue_len=0;
	queue->data_task_queue.queue_head.next=&(queue->data_task_queue.queue_head);
	queue->data_task_queue.queue_head.last=&(queue->data_task_queue.queue_head);
	queue->data_task_queue.queue_tail=&(queue->data_task_queue.queue_head);
	
	return MFI_SUCCESS;
}

//�첽�������������
MfiStatus AsyncQueueDelete(MfiPAsyncQueue queue)
{
	MfiPAsyncTaskInfo temp=NULL;
	int i=0;
	
	pthread_mutex_lock(&(queue->lock));
	for(;i<queue->msg_task_queue.queue_len;i++){
		temp=queue->msg_task_queue.queue_head.next;
		queue->msg_task_queue.queue_head.next=temp->next;
		//if(temp->job_buf) free(temp->job_buf);        //��job_buf�ǿգ���Ϊ�첽д����Ҫ�ͷ�buf�Ŀռ� 5.20�ģ��첽д��buf���û��ͷ�
		free(temp);
	}
	
	queue->msg_task_queue.queue_len=0;
	queue->msg_task_queue.queue_head.next=&(queue->msg_task_queue.queue_head);
	queue->msg_task_queue.queue_head.last=&(queue->msg_task_queue.queue_head);
	queue->msg_task_queue.queue_tail=&(queue->msg_task_queue.queue_head);
	
	for(;i<queue->data_task_queue.queue_len;i++){
		temp=queue->data_task_queue.queue_head.next;
		queue->data_task_queue.queue_head.next=temp->next;
		//if(temp->job_buf) free(temp->job_buf);        //��job_buf�ǿգ���Ϊ�첽д����Ҫ�ͷ�buf�Ŀռ�
		free(temp);
	}
	
	queue->data_task_queue.queue_len=0;
	queue->data_task_queue.queue_head.next=&(queue->data_task_queue.queue_head);
	queue->data_task_queue.queue_head.last=&(queue->data_task_queue.queue_head);
	queue->data_task_queue.queue_tail=&(queue->data_task_queue.queue_head);
	
	pthread_mutex_unlock(&(queue->lock));
	pthread_mutex_destroy(&(queue->lock));
	pthread_cond_destroy(&(queue->ready));
	
	return MFI_SUCCESS;	
}

//�첽����Ϣ����
//���ܣ������첽����Ϣ�������ڲ����û�ʹ�øýӿڵ������������жϣ�Ҫʹ���첽�ӿڣ�����ʹ�����첽IO����¼�
MfiStatus ReadMsgAsync(MfiSession Mfi, MfiPUInt32 msgtype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventInfo eventinfo=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiPMessage msgp=NULL;
	int i=0;
	
	#ifdef ALL_TEST
	printf("FUNCTION : ReadMsgAsync!\n");
	#endif
	psession=&(RsrcManager->session_list[Mfi]);
	
	//�ж��Ƿ�ʹ�����첽IO����¼�
	for(i=0;i<psession->cfg_amount;i++)
	{
		if(MFI_EVENT_IO_COMPLETION==psession->event_cfg[i].event_type)
			break;
	}
	
	if(i>=psession->cfg_amount || psession->event_cfg[i].is_mechanism_en==NOMECH)
		return MFI_ERROR_INV_SETUP;

	if(pthread_mutex_trylock(&(psession->msg_rfifo.lock))==0){
//		printf("ReadMsgAsync:1!\n");
		if(psession->msg_rfifo.empty==MFI_FALSE)
		{
			//��fifo����Ϣ
//			printf("ReadMsgAsync:2!\n");
			msgp=psession->msg_rfifo.buf[psession->msg_rfifo.rindex];
			if(psession->msg_rfifo.rindex < psession->msg_rfifo.buf_len-1)
				psession->msg_rfifo.rindex++;
			else
				psession->msg_rfifo.rindex=0;
				
			if(psession->msg_rfifo.rindex==psession->msg_rfifo.windex)
				psession->msg_rfifo.empty=MFI_TRUE;
			psession->msg_rfifo.full=MFI_FALSE;
			
			pthread_mutex_unlock(&(psession->msg_rfifo.lock));
			CombMsgChange(msgp, NULL, msgtype, NULL, bufp, retCnt);
			
			//�����첽IO����¼���ӣ��ṩ�û�һ��ѡ�񣬺���ͬ�������Ĵ���Ҳͨ���첽�¼��Ĵ���ʽ����
			eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
			if(eventinfo==NULL)
				return MFI_ERROR_ALLOC;
			eventinfo->ref_count=1;
			eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
			memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //���Գ�ʼ��
			eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=psession->new_jobid;
			psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
			eventinfo->event_attr.async_io_event[2].attr_val.attr_s=*bufp;
			eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=*msgtype;
			eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=*retCnt;
			eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS_SYNC;
			eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_READ_MSG;
			InsertEventInfo(eventinfo);
			
			//�����¼���ָ��ṹ���봦����ƵĶ���
			InsertEventPointer(Mfi, eventinfo);
			
			if(jobId!=NULL) *jobId=eventinfo->event_attr.async_io_event[1].attr_val.attr_ui;
			
			return MFI_SUCCESS_SYNC;
		}
//		printf("ReadMsgAsync:3!\n");
		pthread_mutex_unlock(&(psession->msg_rfifo.lock)); 
	}
//	printf("ReadMsgAsync:8!\n");
	//�����첽������
	if((task=(MfiPAsyncTaskInfo)malloc(sizeof(MfiAsyncTaskInfo)))==NULL){
//		printf("ReadMsgAsync:9!\n");
		return MFI_ERROR_ALLOC;
	}
	
//	printf("ReadMsgAsync:4!\n");
	task->job_id=psession->new_jobid;
	psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
	task->job_type=MFI_ASYNC_READ_MSG;
	task->job_buf=NULL;
	task->buf_size=0;
	
//	printf("ReadMsgAsync:5!\n");
	pthread_mutex_lock(&(psession->rd_queue.lock));
	task->next=psession->rd_queue.msg_task_queue.queue_tail->next;
	task->last=psession->rd_queue.msg_task_queue.queue_tail;
	task->next->last=task;
	task->last->next=task;
	psession->rd_queue.msg_task_queue.queue_tail=task;
	psession->rd_queue.msg_task_queue.queue_len++;
	pthread_cond_signal(&(psession->rd_queue.ready));
	pthread_mutex_unlock(&(psession->rd_queue.lock));
	
//	printf("ReadMsgAsync:6!\n");
	if(jobId!=NULL)  *jobId=task->job_id;
	
	return MFI_SUCCESS;
}

//�첽д��Ϣ����
//buf�����ڻ�ȡ���첽IO����¼�֮����ܱ��û��ͷ�
MfiStatus WriteMsgAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgtype, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventInfo eventinfo=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiUInt32 ip;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	//�ж��Ƿ�ʹ�����첽IO����¼�
	for(i=0;i<psession->cfg_amount;i++)
	{
		if(MFI_EVENT_IO_COMPLETION==psession->event_cfg[i].event_type)
			break;
	}
	
	if(i>=psession->cfg_amount || psession->event_cfg[i].is_mechanism_en==NOMECH)
		return MFI_ERROR_INV_SETUP;
		
	if(priorty==0) priorty=APP_MSG_DEF_PORITY;
	Mfi_to_Ip(Mfi,&ip);
//	printf("WriteMsgAsync:1!\n");
	
	if(pthread_mutex_trylock(&DPFreamMsg_lock)==0) //���Ի�ȡ�ײ�ͨ��֡FIFO����1��������ռ��֡ģ��
	{
//		printf("WriteMsgAsync:2!\n");
		//���÷�֡��������ȡ��2�����Ựת��Ϊip������֡�ռ䣬��֡��д��fifo���ͷ���2
		DecmposeFreamMsg(ip, &msgTxFifo, priorty, APP_MSG_CLASS, msgtype, buf, retCnt,0);
		pthread_mutex_unlock(&DPFreamMsg_lock);//�ͷ���1		
		
//		printf("WriteMsgAsync:3!\n");
		
		//�����첽IO����¼���ӣ�����첽д���û�buf���ſ��Ա��ͷţ�
		eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
		if(eventinfo==NULL)
			return MFI_ERROR_ALLOC;
		eventinfo->ref_count=1;
		eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
		memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //���Գ�ʼ��
		eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=psession->new_jobid;
		psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
		eventinfo->event_attr.async_io_event[2].attr_val.attr_s=buf;       //�û�����buf
		eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=APP_MSG_CLASS;
		eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=msgtype;
		eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=retCnt;   //buf�ĳ���
		eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS_SYNC;
		eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_WRITE_MSG;
		InsertEventInfo(eventinfo);
		
		//�����¼���ָ��ṹ���봦����ƵĶ���
		InsertEventPointer(Mfi, eventinfo);
		
//		printf("WriteMsgAsync:4!\n");
		
		if(jobId!=NULL)  *jobId=eventinfo->event_attr.async_io_event[1].attr_val.attr_ui;
		
		return MFI_SUCCESS_SYNC;
	}

//	printf("WriteMsgAsync:5!\n");
	//�����첽д����
	if((task=(MfiPAsyncTaskInfo)malloc(sizeof(MfiAsyncTaskInfo)))==NULL)
		return MFI_ERROR_ALLOC;
		
	task->job_id=psession->new_jobid;
	psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
	task->job_type=MFI_ASYNC_WRITE_MSG;
	task->job_buf=buf;
	task->buf_size=retCnt;
	task->priorty=priorty;
	task->type=msgtype;
	task->mdclass=APP_MSG_CLASS;
	task->addr=ip;
	
	pthread_mutex_lock(&(psession->wt_queue.lock));
	task->next=psession->wt_queue.msg_task_queue.queue_tail->next;
	task->last=psession->wt_queue.msg_task_queue.queue_tail;
	task->next->last=task;
	task->last->next=task;
	psession->wt_queue.msg_task_queue.queue_tail=task;
	psession->wt_queue.msg_task_queue.queue_len++;
	pthread_cond_signal(&(psession->wt_queue.ready));
	pthread_mutex_unlock(&(psession->wt_queue.lock));
	
//	printf("WriteMsgAsync:6!\n");
	if(jobId!=NULL)  *jobId=task->job_id;
	return MFI_SUCCESS;	
}

//�첽�����ݺ���
MfiStatus ReadDataAsync(MfiSession Mfi, MfiPUInt32 datatype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventInfo eventinfo=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiPdata datap=NULL;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	//�ж��Ƿ�ʹ�����첽IO����¼�
	for(i=0;i<psession->cfg_amount;i++)
	{
		if(MFI_EVENT_IO_COMPLETION==psession->event_cfg[i].event_type)
			break;
	}
	
	if(i>=psession->cfg_amount || psession->event_cfg[i].is_mechanism_en==NOMECH)
		return MFI_ERROR_INV_SETUP;

	if(pthread_mutex_trylock(&(psession->data_rfifo.lock))==0){
		if(psession->data_rfifo.empty==MFI_FALSE)
		{
			//��fifo����Ϣ
			datap=psession->data_rfifo.buf[psession->data_rfifo.rindex];
			if(psession->data_rfifo.rindex < psession->data_rfifo.buf_len-1)
				psession->data_rfifo.rindex++;
			else
				psession->data_rfifo.rindex=0;
				
			if(psession->data_rfifo.rindex==psession->data_rfifo.windex)
				psession->data_rfifo.empty=MFI_TRUE;
			psession->data_rfifo.full=MFI_FALSE;
			
			pthread_mutex_unlock(&(psession->data_rfifo.lock));
			CombDataChange(datap, NULL, datatype, NULL, bufp, retCnt);	
			
			//�����첽IO����¼���ӣ��ṩ�û�һ��ѡ�񣬺���ͬ�������Ĵ���Ҳͨ���첽�¼��Ĵ���ʽ����
			eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
			if(eventinfo==NULL)
				return MFI_ERROR_ALLOC;
			eventinfo->ref_count=1;
			eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
			memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //���Գ�ʼ��
			eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=psession->new_jobid;
			psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
			eventinfo->event_attr.async_io_event[2].attr_val.attr_s=*bufp;
			eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=*datatype;
			eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=*retCnt;   //ע������޸�
			eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS_SYNC;
			eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_READ_DATA;
			InsertEventInfo(eventinfo);
			
			//�����¼���ָ��ṹ���봦����ƵĶ���
			InsertEventPointer(Mfi, eventinfo);
			
			if(jobId!=NULL)  *jobId=eventinfo->event_attr.async_io_event[1].attr_val.attr_ui;
			
			return MFI_SUCCESS_SYNC;
		}
		pthread_mutex_unlock(&(psession->data_rfifo.lock)); 
	}
	
	//�����첽������
	if((task=(MfiPAsyncTaskInfo)malloc(sizeof(MfiAsyncTaskInfo)))==NULL)
		return MFI_ERROR_ALLOC;
		
	task->job_id=psession->new_jobid;
	psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
	task->job_type=MFI_ASYNC_READ_DATA;
	task->job_buf=NULL;
	task->buf_size=0;
	
	pthread_mutex_lock(&(psession->rd_queue.lock));
	task->next=psession->rd_queue.data_task_queue.queue_tail->next;
	task->last=psession->rd_queue.data_task_queue.queue_tail;
	task->next->last=task;
	task->last->next=task;
	psession->rd_queue.data_task_queue.queue_tail=task;
	psession->rd_queue.data_task_queue.queue_len++;
	pthread_cond_signal(&(psession->rd_queue.ready));
	pthread_mutex_unlock(&(psession->rd_queue.lock));
	
	if(jobId!=NULL)  *jobId=task->job_id;
	
	return MFI_SUCCESS;
}

//�첽д���ݺ���
//buf�����ڻ�ȡ���첽IO����¼�֮����ܱ��û��ͷ�
MfiStatus WriteDataAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 datatype, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventInfo eventinfo=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiUInt32 ip;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	//�ж��Ƿ�ʹ�����첽IO����¼�
	for(i=0;i<psession->cfg_amount;i++)
	{
		if(MFI_EVENT_IO_COMPLETION==psession->event_cfg[i].event_type)
			break;
	}
	
	if(i>=psession->cfg_amount || psession->event_cfg[i].is_mechanism_en==NOMECH)
		return MFI_ERROR_INV_SETUP;
		
	if(priorty==0) priorty=APP_DATA_DEF_PORITY;
	Mfi_to_Ip(Mfi,&ip);
	
	if(pthread_mutex_trylock(&DPFreamData_lock)==0) //���Ի�ȡ�ײ�ͨ��֡FIFO����1��������ռ��֡ģ��
	{
		//���÷�֡��������ȡ��2�����Ựת��Ϊip������֡�ռ䣬��֡��д��fifo���ͷ���2
		DecmposeFreamData(ip, &dataTxFifo, priorty, APP_DATA_CLASS, datatype, buf, retCnt,0);
		
		pthread_mutex_unlock(&DPFreamData_lock);//�ͷ���1		
		
		//�����첽IO����¼���ӣ�����첽д���û�buf���ſ��Ա��ͷţ�
		eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
		if(eventinfo==NULL)
			return MFI_ERROR_ALLOC;
		eventinfo->ref_count=1;
		eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
		memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //���Գ�ʼ��
		eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=psession->new_jobid;
		psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
		eventinfo->event_attr.async_io_event[2].attr_val.attr_s=buf;       //�û�����buf
		eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=APP_DATA_CLASS; 
		eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=datatype;   //type
		eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=retCnt;   //buf�ĳ���
		eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS_SYNC;
		eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_WRITE_DATA;
		InsertEventInfo(eventinfo);
		
		//�����¼���ָ��ṹ���봦����ƵĶ���
		InsertEventPointer(Mfi, eventinfo);
		
		if(jobId!=NULL)  *jobId=eventinfo->event_attr.async_io_event[1].attr_val.attr_ui;
		
		return MFI_SUCCESS_SYNC;
	}

	//�����첽д����
	if((task=(MfiPAsyncTaskInfo)malloc(sizeof(MfiAsyncTaskInfo)))==NULL)
		return MFI_ERROR_ALLOC;
		
	task->job_id=psession->new_jobid;
	psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
	task->job_type=MFI_ASYNC_WRITE_DATA;
	task->job_buf=buf;
	task->buf_size=retCnt;
	task->priorty=priorty;
	task->type=datatype;
	task->mdclass=APP_DATA_CLASS;
	task->addr=ip;
	
	pthread_mutex_lock(&(psession->wt_queue.lock));
	task->next=psession->wt_queue.data_task_queue.queue_tail->next;
	task->last=psession->wt_queue.data_task_queue.queue_tail;
	task->next->last=task;
	task->last->next=task;
	psession->wt_queue.data_task_queue.queue_tail=task;
	psession->wt_queue.data_task_queue.queue_len++;
	pthread_cond_signal(&(psession->wt_queue.ready));
	pthread_mutex_unlock(&(psession->wt_queue.lock));
	
	if(jobId!=NULL)  *jobId=task->job_id;
	return MFI_SUCCESS;	
}

//�첽���������߳�
void* thr_asyncread(void* arg)
{
	MfiPSessionInfo psession=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiSession Mfi;
	MfiPEventInfo eventinfo=NULL;
	MfiPMessage msgp=NULL;
	MfiPdata datap=NULL;
	MfiPBuf buf=NULL;
	MfiUInt32 retCnt=0,msgtype=0,datatype=0,msgclass=0,dataclass=0;
	MfiUInt32 addr=0;
	int status=0;
	
	//�Ӳ�����ȡ���ỰID��������
	Mfi=*((MfiSession*)arg);
	psession=&(RsrcManager->session_list[Mfi]);
	
	#ifdef PTHREAD_TEST
	printf("thr_asyncread start: %d~~~~\n",Mfi);
	#endif
	
	while(psession->pthread_id.asyncread_thr != -1){
		//�첽����Ϣ������д���
		pthread_mutex_lock(&(psession->rd_queue.lock));

		if(psession->rd_queue.msg_task_queue.queue_len!=0){
			if((status=pthread_mutex_trylock(&(psession->msg_rfifo.lock)))==0){
				if(psession->msg_rfifo.empty==MFI_FALSE)
				{
					//ɾ���첽����
					task=psession->rd_queue.msg_task_queue.queue_head.next;
					task->last->next=task->next;
					task->next->last=task->last;
					--psession->rd_queue.msg_task_queue.queue_len;
					if(task==psession->rd_queue.msg_task_queue.queue_tail)
						psession->rd_queue.msg_task_queue.queue_tail=task->last;					
					pthread_mutex_unlock(&(psession->rd_queue.lock));
					
					//��fifo����Ϣ
					msgp=psession->msg_rfifo.buf[psession->msg_rfifo.rindex];
					if(psession->msg_rfifo.rindex < psession->msg_rfifo.buf_len-1)
						psession->msg_rfifo.rindex++;
					else
						psession->msg_rfifo.rindex=0;
						
					if(psession->msg_rfifo.rindex==psession->msg_rfifo.windex)
						psession->msg_rfifo.empty=MFI_TRUE;
					psession->msg_rfifo.full=MFI_FALSE;
					
					pthread_mutex_unlock(&(psession->msg_rfifo.lock));
					CombMsgChange(msgp, &msgclass, &msgtype, &addr, &buf, &retCnt);
						
					//�����첽IO����¼����
					//�첽IO�¼�����ȫ���¼���һ���첽IO�¼�ֻ��������һ���Ự�����Բ��ز�ѯ�¼�ʹ������ȷ�����Ƿ�ʹ��
					//���Խ���Ѱ��Ӧ�Ự���¼����ýṹ��ȷ�����Ƿ�ʹ��
					while(eventinfo==NULL)
						eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
					eventinfo->ref_count=1;
					eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
					memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //���Գ�ʼ��
					eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=task->job_id;
					eventinfo->event_attr.async_io_event[2].attr_val.attr_s=buf;  //�������ݵ�buf
					eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=msgclass;
					eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=msgtype;
					eventinfo->event_attr.async_io_event[5].attr_val.attr_ui=addr;
					eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=retCnt;   //ע������޸� �������ݵĳ���
					eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS;
					eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_READ_MSG;
					InsertEventInfo(eventinfo);
					free(task);
					
					//�����¼���ָ��ṹ���봦����ƵĶ���
					InsertEventPointer(Mfi, eventinfo);
					eventinfo=NULL;
				}
				else
				{
					pthread_mutex_unlock(&(psession->msg_rfifo.lock));
					pthread_mutex_unlock(&(psession->rd_queue.lock));
				}
			}
			else
				pthread_mutex_unlock(&(psession->rd_queue.lock));
		}
		else{
			if(psession->rd_queue.data_task_queue.queue_len==0)
				pthread_cond_wait(&(psession->rd_queue.ready),&(psession->rd_queue.lock));
				
			pthread_mutex_unlock(&(psession->rd_queue.lock));
		}
		
		if(psession->pthread_id.asyncread_thr == -1)
			break;
			
		//�첽�����ݶ��д���
		pthread_mutex_lock(&(psession->rd_queue.lock));
		
		if(psession->rd_queue.data_task_queue.queue_len!=0){
			if((status=pthread_mutex_trylock(&(psession->data_rfifo.lock)))==0){
				if(psession->data_rfifo.empty==MFI_FALSE)
				{
					//ɾ���첽����
					task=psession->rd_queue.data_task_queue.queue_head.next;
					task->last->next=task->next;
					task->next->last=task->last;
					--psession->rd_queue.data_task_queue.queue_len;
					if(task==psession->rd_queue.data_task_queue.queue_tail)
						psession->rd_queue.data_task_queue.queue_tail=task->last;
					pthread_mutex_unlock(&(psession->rd_queue.lock));					
					//��fifo������
					datap=psession->data_rfifo.buf[psession->data_rfifo.rindex];
					if(psession->data_rfifo.rindex < psession->data_rfifo.buf_len-1)
						psession->data_rfifo.rindex++;
					else
						psession->data_rfifo.rindex=0;
						
					if(psession->data_rfifo.rindex==psession->data_rfifo.windex)
						psession->data_rfifo.empty=MFI_TRUE;
					psession->data_rfifo.full=MFI_FALSE;
					
					pthread_mutex_unlock(&(psession->data_rfifo.lock));
					CombDataChange(datap, &dataclass, &datatype, &addr, &buf, &retCnt);	
							
					//�����첽IO����¼����
					while(eventinfo==NULL)
						eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
					eventinfo->ref_count=1;
					eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
					memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //���Գ�ʼ��
					eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=task->job_id;
					eventinfo->event_attr.async_io_event[2].attr_val.attr_s=buf;  //�������ݵ�buf
					eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=dataclass;
					eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=datatype;
					eventinfo->event_attr.async_io_event[5].attr_val.attr_ui=addr;
					eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=retCnt;   //ע������޸� �������ݵĳ���
					eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS;
					eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_READ_DATA;
					InsertEventInfo(eventinfo);
					free(task);
					
					//�����¼���ָ��ṹ���봦����ƵĶ���
					InsertEventPointer(Mfi, eventinfo);
					eventinfo=NULL;
				}
				else
				{
					pthread_mutex_unlock(&(psession->data_rfifo.lock));
					pthread_mutex_unlock(&(psession->rd_queue.lock));
				}
			}
			else
				pthread_mutex_unlock(&(psession->rd_queue.lock));
		}
		else{
			if(psession->rd_queue.msg_task_queue.queue_len==0)
				pthread_cond_wait(&(psession->rd_queue.ready),&(psession->rd_queue.lock));
				
			pthread_mutex_unlock(&(psession->rd_queue.lock));
		}
		
	}

	#ifdef PTHREAD_TEST
	printf("thr_asyncread stop: %d~~~~\n",Mfi);
	#endif
	
	return NULL;
}

//�첽д�������߳�
void* thr_asyncwrite(void* arg)
{
	MfiPSessionInfo psession=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiSession Mfi;
	MfiPEventInfo eventinfo=NULL;
	MfiUInt32 ip;
	int status=0;
	
	//�Ӳ�����ȡ���ỰID��������
	Mfi=*((MfiSession*)arg);
	psession=&(RsrcManager->session_list[Mfi]);
	
	#ifdef PTHREAD_TEST
	printf("thr_asyncwrite start: %d~~~~\n",Mfi);
	#endif
	while(psession->pthread_id.asyncwrite_thr != -1){
		//�첽д��Ϣ������д���
		pthread_mutex_lock(&(psession->wt_queue.lock));

		if(psession->wt_queue.msg_task_queue.queue_len!=0){
			if((status=pthread_mutex_trylock(&DPFreamMsg_lock))==0) //���Ի�ȡ�ײ�ͨ��֡FIFO����1��������ռ��֡ģ��
			{   
				//ɾ���첽����
				task=psession->wt_queue.msg_task_queue.queue_head.next;
				task->last->next=task->next;
				task->next->last=task->last;
				--psession->wt_queue.msg_task_queue.queue_len;
				if(task==psession->wt_queue.msg_task_queue.queue_tail)
					psession->wt_queue.msg_task_queue.queue_tail=task->last;
					
				pthread_mutex_unlock(&(psession->wt_queue.lock));
				
				//���÷�֡��������ȡ��2�����Ựת��Ϊip������֡�ռ䣬��֡��д��fifo���ͷ���2
				DecmposeFreamMsg(task->addr, &msgTxFifo, task->priorty, task->mdclass, task->type, task->job_buf, task->buf_size,0);
				
				pthread_mutex_unlock(&DPFreamMsg_lock);//�ͷ���1		
				
				//�����첽IO����¼���ӣ�����첽д���û�buf���ſ��Ա��ͷţ�
				while(eventinfo==NULL)
					eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
				eventinfo->ref_count=1;
				eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
				memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //���Գ�ʼ��
				eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=task->job_id;
				eventinfo->event_attr.async_io_event[2].attr_val.attr_s=task->job_buf;       //�û�����buf
				eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=task->mdclass; 
				eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=task->type;
				eventinfo->event_attr.async_io_event[5].attr_val.attr_ui=task->addr;
				eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=task->buf_size;   //buf�ĳ���
				eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS;
				eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_WRITE_MSG;
				InsertEventInfo(eventinfo);
				free(task);
				
				//�����¼���ָ��ṹ���봦����ƵĶ���
				InsertEventPointer(Mfi, eventinfo);
				eventinfo=NULL;
			}
			else{
				pthread_mutex_unlock(&(psession->wt_queue.lock));
			}
		}
		else{
			if(psession->wt_queue.data_task_queue.queue_len==0)
				pthread_cond_wait(&(psession->wt_queue.ready),&(psession->wt_queue.lock));
				
			pthread_mutex_unlock(&(psession->wt_queue.lock)); 
		}
		
		if(psession->pthread_id.asyncwrite_thr == -1)
			break;
			
		//�첽д���ݶ��д���
		pthread_mutex_lock(&(psession->wt_queue.lock));

		if(psession->wt_queue.data_task_queue.queue_len!=0){
			if((status=pthread_mutex_trylock(&DPFreamData_lock))==0)  //���Ի�ȡ�ײ�ͨ��֡FIFO����1��������ռ��֡ģ��
			{  
				//ɾ���첽����
				task=psession->wt_queue.data_task_queue.queue_head.next;
				task->last->next=task->next;
				task->next->last=task->last;
				--psession->wt_queue.data_task_queue.queue_len;
				if(task==psession->wt_queue.data_task_queue.queue_tail)
					psession->wt_queue.data_task_queue.queue_tail=task->last;
					
				pthread_mutex_unlock(&(psession->wt_queue.lock));
								
				DecmposeFreamData(task->addr, &dataTxFifo, task->priorty, task->mdclass, task->type, task->job_buf, task->buf_size,0);
				
				pthread_mutex_unlock(&DPFreamData_lock);//�ͷ���1
				
				//�����첽IO����¼���ӣ�����첽д���û�buf���ſ��Ա��ͷţ�
				while(eventinfo==NULL)
					eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
				eventinfo->ref_count=1;
				eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
				memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //���Գ�ʼ��
				eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=task->job_id;
				eventinfo->event_attr.async_io_event[2].attr_val.attr_s=task->job_buf;       //�û�����buf
				eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=task->mdclass; 
				eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=task->type;
				eventinfo->event_attr.async_io_event[5].attr_val.attr_ui=task->addr;
				eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=task->buf_size;   //buf�ĳ���
				eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS;
				eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_WRITE_DATA;
				InsertEventInfo(eventinfo);
				free(task);
				
				//�����¼���ָ��ṹ���봦����ƵĶ���
				InsertEventPointer(Mfi, eventinfo);
				eventinfo=NULL;
			}
			else{
				pthread_mutex_unlock(&(psession->wt_queue.lock));
			}
		}
		else{
			if(psession->wt_queue.msg_task_queue.queue_len==0)
				pthread_cond_wait(&(psession->wt_queue.ready),&(psession->wt_queue.lock));
				
			pthread_mutex_unlock(&(psession->wt_queue.lock)); 
		}
	}
	
	#ifdef PTHREAD_TEST
	printf("thr_asyncwrite stop: %d~~~~\n",Mfi);
	#endif
	return NULL;
	
}

//�ú����ڹرջỰʱ���ڹر��첽д�߳�
MfiStatus ForCloseAsyncWtThr(MfiSession Mfi)
{
	MfiPAsyncTaskInfo task=NULL;
	MfiPSessionInfo psession=NULL;
	
	psession=&(RsrcManager->session_list[Mfi]);
	while(task==NULL)
		task=(MfiPAsyncTaskInfo)malloc(sizeof(MfiAsyncTaskInfo));
		
	task->job_id=psession->new_jobid;
	psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
	task->job_type=MFI_ASYNC_WRITE_MSG;
	task->job_buf=NULL;
	task->buf_size=0;
	
	pthread_mutex_lock(&(psession->wt_queue.lock));
	task->next=psession->wt_queue.msg_task_queue.queue_tail->next;
	task->last=psession->wt_queue.msg_task_queue.queue_tail;
	task->next->last=task;
	task->last->next=task;
	psession->wt_queue.msg_task_queue.queue_tail=task;
	psession->wt_queue.msg_task_queue.queue_len++;
	pthread_cond_signal(&(psession->wt_queue.ready));
	pthread_mutex_unlock(&(psession->wt_queue.lock));
	
	return MFI_SUCCESS;
}

//�ú����ڹرջỰʱ���ڹر��첽���߳�
MfiStatus ForCloseAsyncRdThr(MfiSession Mfi)
{
	MfiPAsyncTaskInfo task=NULL;
	MfiPSessionInfo psession=NULL;
	
	psession=&(RsrcManager->session_list[Mfi]);
	while(task==NULL)
		task=(MfiPAsyncTaskInfo)malloc(sizeof(MfiAsyncTaskInfo));
		
	task->job_id=psession->new_jobid;
	psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
	task->job_type=MFI_ASYNC_READ_MSG;
	task->job_buf=NULL;
	task->buf_size=0;
	
	pthread_mutex_lock(&(psession->rd_queue.lock));
	task->next=psession->rd_queue.msg_task_queue.queue_tail->next;
	task->last=psession->rd_queue.msg_task_queue.queue_tail;
	task->next->last=task;
	task->last->next=task;
	psession->rd_queue.msg_task_queue.queue_tail=task;
	psession->rd_queue.msg_task_queue.queue_len++;
	pthread_cond_signal(&(psession->rd_queue.ready));
	pthread_mutex_unlock(&(psession->rd_queue.lock));
	
	return MFI_SUCCESS;
}

MfiStatus SysReadMsg(MfiSession Mfi, MfiPUInt32 msgclass, MfiPUInt32 msgtype, MfiPUInt32 msgsrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiUInt32 timeout)
{
	MfiPSessionInfo psession=NULL;
	struct timespec now;
	int i=0;
	MfiPMessage msgp=NULL;
	
	psession=&(RsrcManager->session_list[Mfi]);
	if(timeout==MFI_TMO_USEATTRVAL)
	{
		if((i=FindAttr(MFI_ATTR_TMO_VALUE, RsrcManager->session_list[Mfi].rsrc_attr, RsrcManager->session_list[Mfi].attr_amount))==-1){
			return MFI_ERROR_NSUP_ATTR;
		}
		timeout=RsrcManager->session_list[Mfi].rsrc_attr[i].attr_val.attr_ui;
	}
	
	if(timeout>MFI_TMO_IMMEDIATE && timeout<MFI_TMO_INFINITE){
		clock_gettime(CLOCK_REALTIME,&now);
		now.tv_sec=now.tv_sec+(timeout*1000000+now.tv_nsec)/1000000000;
		now.tv_nsec=(timeout*1000000+now.tv_nsec)%1000000000;
	}
		
	pthread_mutex_lock(&(psession->msg_rfifo.lock));
	while(psession->msg_rfifo.empty==MFI_TRUE){
		if(timeout==MFI_TMO_IMMEDIATE){
			pthread_mutex_unlock(&(psession->msg_rfifo.lock));
			return MFI_ERROR_NO_DATA;
		}
		else if(timeout==MFI_TMO_INFINITE){
			pthread_cond_wait(&(psession->msg_rfifo.ready),&(psession->msg_rfifo.lock));
		}
		else{
			if(pthread_cond_timedwait(&(psession->msg_rfifo.ready),&(psession->msg_rfifo.lock),&now)==ETIMEDOUT)
				goto error;
		}
	}
	
	//��fifo��ȡ��Ϣ
	msgp=psession->msg_rfifo.buf[psession->msg_rfifo.rindex];
	if(psession->msg_rfifo.rindex < psession->msg_rfifo.buf_len-1)
		psession->msg_rfifo.rindex++;
	else
		psession->msg_rfifo.rindex=0;
		
	if(psession->msg_rfifo.rindex==psession->msg_rfifo.windex)
		psession->msg_rfifo.empty=MFI_TRUE;
	psession->msg_rfifo.full=MFI_FALSE;
	
	pthread_mutex_unlock(&(psession->msg_rfifo.lock));
	CombMsgChange(msgp, msgclass, msgtype, msgsrcaddr, bufp, retCnt);
	
	return MFI_SUCCESS;
	
	error:
		pthread_mutex_unlock(&(psession->msg_rfifo.lock));
		return MFI_ERROR_TMO;
}

MfiStatus SysReadData(MfiSession Mfi, MfiPUInt32 dataclass, MfiPUInt32 datatype, MfiPUInt32 datasrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiUInt32 timeout)
{
	MfiPSessionInfo psession=NULL;
	struct timespec now;
	MfiPdata datap=NULL;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	if(timeout==MFI_TMO_USEATTRVAL)
	{
		if((i=FindAttr(MFI_ATTR_TMO_VALUE, RsrcManager->session_list[Mfi].rsrc_attr, RsrcManager->session_list[Mfi].attr_amount))==-1){
			return MFI_ERROR_NSUP_ATTR;
		}
		timeout=RsrcManager->session_list[Mfi].rsrc_attr[i].attr_val.attr_ui;
	}
	
	if(timeout>MFI_TMO_IMMEDIATE && timeout<MFI_TMO_INFINITE){
		clock_gettime(CLOCK_REALTIME,&now);
		now.tv_sec=now.tv_sec+(timeout*1000000+now.tv_nsec)/1000000000;
		now.tv_nsec=(timeout*1000000+now.tv_nsec)%1000000000;
	}
	
	pthread_mutex_lock(&(psession->data_rfifo.lock));
	while(psession->data_rfifo.empty==MFI_TRUE){
		if(timeout==MFI_TMO_IMMEDIATE){
			pthread_mutex_unlock(&(psession->data_rfifo.lock));
			return MFI_ERROR_NO_DATA;
		}
		else if(timeout==MFI_TMO_INFINITE){
			pthread_cond_wait(&(psession->data_rfifo.ready),&(psession->data_rfifo.lock));
		}
		else{
			if(pthread_cond_timedwait(&(psession->data_rfifo.ready),&(psession->data_rfifo.lock),&now)==ETIMEDOUT)
				goto error;
		}
	}
	
	//��fifo��ȡ��Ϣ
	datap=psession->data_rfifo.buf[psession->data_rfifo.rindex];
	if(psession->data_rfifo.rindex < psession->data_rfifo.buf_len-1)
		psession->data_rfifo.rindex++;
	else
		psession->data_rfifo.rindex=0;
		
	if(psession->data_rfifo.rindex==psession->data_rfifo.windex)
		psession->data_rfifo.empty=MFI_TRUE;
	psession->data_rfifo.full=MFI_FALSE;
	
	pthread_mutex_unlock(&(psession->data_rfifo.lock));
	CombDataChange(datap, dataclass, datatype, datasrcaddr, bufp, retCnt);	
	
	return MFI_SUCCESS;
	
	error:
		pthread_mutex_unlock(&(psession->data_rfifo.lock));
		return MFI_ERROR_TMO;
}

//������Դ(ϵͳ)ͬ��д�ӿ�
MfiStatus SysWriteMsg(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgclass, MfiUInt32 msgtype, MfiUInt32 msgdstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 timeout, MfiUInt32 sendtype)
{
	MfiPSessionInfo psession=NULL;
	struct timespec now;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	if(timeout==MFI_TMO_USEATTRVAL)
	{
		if((i=FindAttr(MFI_ATTR_TMO_VALUE, RsrcManager->session_list[Mfi].rsrc_attr, RsrcManager->session_list[Mfi].attr_amount))==-1){
			return MFI_ERROR_NSUP_ATTR;
		}
		timeout=RsrcManager->session_list[Mfi].rsrc_attr[i].attr_val.attr_ui;
	}
	
	if(timeout>MFI_TMO_IMMEDIATE && timeout<MFI_TMO_INFINITE){
		clock_gettime(CLOCK_REALTIME,&now);
		now.tv_sec=now.tv_sec+(timeout*1000000+now.tv_nsec)/1000000000;
		now.tv_nsec=(timeout*1000000+now.tv_nsec)%1000000000;
	}
		
	if(timeout==MFI_TMO_IMMEDIATE){
		if(pthread_mutex_trylock(&DPFreamMsg_lock)!=0)   //��ȡ�ײ�ͨ��֡FIFO����1
			return MFI_ERROR_TMO;
	}
	else if(timeout==MFI_TMO_INFINITE)
		pthread_mutex_lock(&DPFreamMsg_lock);            //��ȡ�ײ�ͨ��֡FIFO����1
	else{
		if(pthread_mutex_timedlock(&DPFreamMsg_lock,&now)==ETIMEDOUT)  //��ȡ�ײ�ͨ��֡FIFO����1��������ռ��֡ģ��
			return MFI_ERROR_TMO;
	}
		
	//���÷�֡��������ȡ��2�����Ựת��Ϊip������֡�ռ䣬��֡��д��fifo���ͷ���2
	DecmposeFreamMsg(msgdstaddr, &msgTxFifo, priorty, msgclass, msgtype, buf, retCnt,sendtype);
	
	pthread_mutex_unlock(&DPFreamMsg_lock);//�ͷ���1
	return MFI_SUCCESS;
}

//ͬ��д���ݺ���
//���ܣ��ú�������������Դ�����ݵķ���
MfiStatus SysWriteData(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 dataclass, MfiUInt32 datatype, MfiUInt32 datadstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 timeout, MfiUInt32 sendtype)
{
	MfiPSessionInfo psession=NULL;
	struct timespec now;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	if(timeout==MFI_TMO_USEATTRVAL)
	{
		if((i=FindAttr(MFI_ATTR_TMO_VALUE, RsrcManager->session_list[Mfi].rsrc_attr, RsrcManager->session_list[Mfi].attr_amount))==-1){
			return MFI_ERROR_NSUP_ATTR;
		}
		timeout=RsrcManager->session_list[Mfi].rsrc_attr[i].attr_val.attr_ui;
	}
	
	if(timeout>MFI_TMO_IMMEDIATE && timeout<MFI_TMO_INFINITE){
		clock_gettime(CLOCK_REALTIME,&now);
		now.tv_sec=now.tv_sec+(timeout*1000000+now.tv_nsec)/1000000000;
		now.tv_nsec=(timeout*1000000+now.tv_nsec)%1000000000;
	}
		
	if(timeout==MFI_TMO_IMMEDIATE){
		if(pthread_mutex_trylock(&(DPFreamData_lock))!=0)   //��ȡ�ײ�ͨ��֡FIFO����1
			return MFI_ERROR_TMO;
	}
	else if(timeout==MFI_TMO_INFINITE)
		pthread_mutex_lock(&(DPFreamData_lock));            //��ȡ�ײ�ͨ��֡FIFO����1
	else{
		if(pthread_mutex_timedlock(&DPFreamData_lock,&now)==ETIMEDOUT)  //��ȡ�ײ�ͨ��֡FIFO����1��������ռ��֡ģ��
			return MFI_ERROR_TMO;
	}
		
	//���÷�֡��������ȡ��2�����Ựת��Ϊip������֡�ռ䣬��֡��д��fifo���ͷ���2
	DecmposeFreamData(datadstaddr, &dataTxFifo, priorty, dataclass, datatype, buf, retCnt,sendtype);
	
	pthread_mutex_unlock(&DPFreamData_lock);//�ͷ���1
	return MFI_SUCCESS;
}

//�����첽����Ϣ����
//���ܣ������첽����Ϣ�������ڲ����û�ʹ�øýӿڵ������������жϣ�Ҫʹ���첽�ӿڣ�����ʹ�����첽IO����¼�
MfiStatus SysReadMsgAsync(MfiSession Mfi, MfiPUInt32 msgclass, MfiPUInt32 msgtype, MfiPUInt32 msgsrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventInfo eventinfo=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiPMessage msgp=NULL;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	//�ж��Ƿ�ʹ�����첽IO����¼�
	for(i=0;i<psession->cfg_amount;i++)
	{
		if(MFI_EVENT_IO_COMPLETION==psession->event_cfg[i].event_type)
			break;
	}
	
	if(i>=psession->cfg_amount || psession->event_cfg[i].is_mechanism_en==NOMECH)
		return MFI_ERROR_INV_SETUP;

	if(pthread_mutex_trylock(&(psession->msg_rfifo.lock))==0){
		if(psession->msg_rfifo.empty==MFI_FALSE)
		{
			//��fifo����Ϣ
			msgp=psession->msg_rfifo.buf[psession->msg_rfifo.rindex];
			if(psession->msg_rfifo.rindex < psession->msg_rfifo.buf_len-1)
				psession->msg_rfifo.rindex++;
			else
				psession->msg_rfifo.rindex=0;
				
			if(psession->msg_rfifo.rindex==psession->msg_rfifo.windex)
				psession->msg_rfifo.empty=MFI_TRUE;
			psession->msg_rfifo.full=MFI_FALSE;
			
			pthread_mutex_unlock(&(psession->msg_rfifo.lock));
			CombMsgChange(msgp, msgclass, msgtype, msgsrcaddr, bufp, retCnt);
			
			//�����첽IO����¼���ӣ��ṩ�û�һ��ѡ�񣬺���ͬ�������Ĵ���Ҳͨ���첽�¼��Ĵ���ʽ����
			eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
			if(eventinfo==NULL)
				return MFI_ERROR_ALLOC;
			eventinfo->ref_count=1;
			eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
			memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //���Գ�ʼ��
			eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=psession->new_jobid;
			psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
			eventinfo->event_attr.async_io_event[2].attr_val.attr_s=*bufp;
			eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=*msgclass;
			eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=*msgtype;
			eventinfo->event_attr.async_io_event[5].attr_val.attr_ui=*msgsrcaddr;
			eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=*retCnt;
			eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS_SYNC;
			eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_READ_MSG;
			InsertEventInfo(eventinfo);
			
			//�����¼���ָ��ṹ���봦����ƵĶ���
			InsertEventPointer(Mfi, eventinfo);
			
			if(jobId!=NULL)  *jobId=eventinfo->event_attr.async_io_event[1].attr_val.attr_ui;
			
			return MFI_SUCCESS_SYNC;
		}
		pthread_mutex_unlock(&(psession->msg_rfifo.lock)); 
	}
	
	//�����첽������
	if((task=(MfiPAsyncTaskInfo)malloc(sizeof(MfiAsyncTaskInfo)))==NULL)
		return MFI_ERROR_ALLOC;
		
	task->job_id=psession->new_jobid;
	psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
	task->job_type=MFI_ASYNC_READ_MSG;
	task->job_buf=NULL;
	task->buf_size=0;
	
	pthread_mutex_lock(&(psession->rd_queue.lock));
	task->next=psession->rd_queue.msg_task_queue.queue_tail->next;
	task->last=psession->rd_queue.msg_task_queue.queue_tail;
	task->next->last=task;
	task->last->next=task;
	psession->rd_queue.msg_task_queue.queue_tail=task;
	psession->rd_queue.msg_task_queue.queue_len++;
	pthread_cond_signal(&(psession->rd_queue.ready));
	pthread_mutex_unlock(&(psession->rd_queue.lock));
	
	if(jobId!=NULL)  *jobId=task->job_id;
	
	return MFI_SUCCESS;
}

//�첽д��Ϣ����
//buf�����ڻ�ȡ���첽IO����¼�֮����ܱ��û��ͷ�
MfiStatus SysWriteMsgAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgclass, MfiUInt32 msgtype, MfiUInt32 msgdstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventInfo eventinfo=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiUInt32 ip;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	//�ж��Ƿ�ʹ�����첽IO����¼�
	for(i=0;i<psession->cfg_amount;i++)
	{
		if(MFI_EVENT_IO_COMPLETION==psession->event_cfg[i].event_type)
			break;
	}
	
	if(i>=psession->cfg_amount || psession->event_cfg[i].is_mechanism_en==NOMECH)
		return MFI_ERROR_INV_SETUP;
		
	if(pthread_mutex_trylock(&DPFreamMsg_lock)==0) //���Ի�ȡ�ײ�ͨ��֡FIFO����1��������ռ��֡ģ��
	{
		//���÷�֡��������ȡ��2�����Ựת��Ϊip������֡�ռ䣬��֡��д��fifo���ͷ���2
		DecmposeFreamMsg(msgdstaddr, &msgTxFifo, priorty, msgclass, msgtype, buf, retCnt,0);
		pthread_mutex_unlock(&DPFreamMsg_lock);//�ͷ���1		
		
		//�����첽IO����¼���ӣ�����첽д���û�buf���ſ��Ա��ͷţ�
		eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
		if(eventinfo==NULL)
			return MFI_ERROR_ALLOC;
		eventinfo->ref_count=1;
		eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
		memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //���Գ�ʼ��
		eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=psession->new_jobid;
		psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
		eventinfo->event_attr.async_io_event[2].attr_val.attr_s=buf;       //�û�����buf
		eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=msgclass;
		eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=msgtype;
		eventinfo->event_attr.async_io_event[5].attr_val.attr_ui=msgdstaddr;
		eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=retCnt;
		eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS_SYNC;
		eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_WRITE_MSG;
		InsertEventInfo(eventinfo);
		
		//�����¼���ָ��ṹ���봦����ƵĶ���
		InsertEventPointer(Mfi, eventinfo);
		
		if(jobId!=NULL)  *jobId=eventinfo->event_attr.async_io_event[1].attr_val.attr_ui;
		
		return MFI_SUCCESS_SYNC;
	}

	//�����첽д����
	if((task=(MfiPAsyncTaskInfo)malloc(sizeof(MfiAsyncTaskInfo)))==NULL)
		return MFI_ERROR_ALLOC;
		
	task->job_id=psession->new_jobid;
	psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
	task->job_type=MFI_ASYNC_WRITE_MSG;
	task->job_buf=buf;
	task->buf_size=retCnt;
	task->priorty=priorty;
	task->type=msgtype;
	task->mdclass=msgclass;
	task->addr=msgdstaddr;
	
	pthread_mutex_lock(&(psession->wt_queue.lock));
	task->next=psession->wt_queue.msg_task_queue.queue_tail->next;
	task->last=psession->wt_queue.msg_task_queue.queue_tail;
	task->next->last=task;
	task->last->next=task;
	psession->wt_queue.msg_task_queue.queue_tail=task;
	psession->wt_queue.msg_task_queue.queue_len++;
	pthread_cond_signal(&(psession->wt_queue.ready));
	pthread_mutex_unlock(&(psession->wt_queue.lock));
	
	if(jobId!=NULL)  *jobId=task->job_id;
	return MFI_SUCCESS;	
}

//�첽�����ݺ���
MfiStatus SysReadDataAsync(MfiSession Mfi, MfiPUInt32 dataclass, MfiPUInt32 datatype, MfiPUInt32 datasrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventInfo eventinfo=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiPdata datap=NULL;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	//�ж��Ƿ�ʹ�����첽IO����¼�
	for(i=0;i<psession->cfg_amount;i++)
	{
		if(MFI_EVENT_IO_COMPLETION==psession->event_cfg[i].event_type)
			break;
	}
	
	if(i>=psession->cfg_amount || psession->event_cfg[i].is_mechanism_en==NOMECH)
		return MFI_ERROR_INV_SETUP;

	if(pthread_mutex_trylock(&(psession->data_rfifo.lock))==0){
		if(psession->data_rfifo.empty==MFI_FALSE)
		{
			//��fifo����Ϣ
			datap=psession->data_rfifo.buf[psession->data_rfifo.rindex];
			if(psession->data_rfifo.rindex < psession->data_rfifo.buf_len-1)
				psession->data_rfifo.rindex++;
			else
				psession->data_rfifo.rindex=0;
				
			if(psession->data_rfifo.rindex==psession->data_rfifo.windex)
				psession->data_rfifo.empty=MFI_TRUE;
			psession->data_rfifo.full=MFI_FALSE;
			
			pthread_mutex_unlock(&(psession->data_rfifo.lock));
			CombDataChange(datap, NULL, datatype, NULL, bufp, retCnt);	
			
			//�����첽IO����¼���ӣ��ṩ�û�һ��ѡ�񣬺���ͬ�������Ĵ���Ҳͨ���첽�¼��Ĵ���ʽ����
			eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
			if(eventinfo==NULL)
				return MFI_ERROR_ALLOC;
			eventinfo->ref_count=1;
			eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
			memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //���Գ�ʼ��
			eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=psession->new_jobid;
			psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
			eventinfo->event_attr.async_io_event[2].attr_val.attr_s=*bufp;
			eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=*dataclass;
			eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=*datatype;
			eventinfo->event_attr.async_io_event[5].attr_val.attr_ui=*datasrcaddr;
			eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=*retCnt;   //ע������޸�
			eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS_SYNC;
			eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_READ_DATA;
			InsertEventInfo(eventinfo);
			
			//�����¼���ָ��ṹ���봦����ƵĶ���
			InsertEventPointer(Mfi, eventinfo);
			
			if(jobId!=NULL)  *jobId=eventinfo->event_attr.async_io_event[1].attr_val.attr_ui;
			
			return MFI_SUCCESS_SYNC;
		}
		pthread_mutex_unlock(&(psession->data_rfifo.lock)); 
	}
	
	//�����첽������
	if((task=(MfiPAsyncTaskInfo)malloc(sizeof(MfiAsyncTaskInfo)))==NULL)
		return MFI_ERROR_ALLOC;
		
	task->job_id=psession->new_jobid;
	psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
	task->job_type=MFI_ASYNC_READ_DATA;
	task->job_buf=NULL;
	task->buf_size=0;
	
	pthread_mutex_lock(&(psession->rd_queue.lock));
	task->next=psession->rd_queue.data_task_queue.queue_tail->next;
	task->last=psession->rd_queue.data_task_queue.queue_tail;
	task->next->last=task;
	task->last->next=task;
	psession->rd_queue.data_task_queue.queue_tail=task;
	psession->rd_queue.data_task_queue.queue_len++;
	pthread_cond_signal(&(psession->rd_queue.ready));
	pthread_mutex_unlock(&(psession->rd_queue.lock));
	
	if(jobId!=NULL)  *jobId=task->job_id;
	
	return MFI_SUCCESS;
}

//�첽д���ݺ���
//buf�����ڻ�ȡ���첽IO����¼�֮����ܱ��û��ͷ�
MfiStatus SysWriteDataAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 dataclass, MfiUInt32 datatype, MfiUInt32 datadstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventInfo eventinfo=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiUInt32 ip;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	//�ж��Ƿ�ʹ�����첽IO����¼�
	for(i=0;i<psession->cfg_amount;i++)
	{
		if(MFI_EVENT_IO_COMPLETION==psession->event_cfg[i].event_type)
			break;
	}
	
	if(i>=psession->cfg_amount || psession->event_cfg[i].is_mechanism_en==NOMECH)
		return MFI_ERROR_INV_SETUP;
		
	if(pthread_mutex_trylock(&DPFreamData_lock)==0) //���Ի�ȡ�ײ�ͨ��֡FIFO����1��������ռ��֡ģ��
	{
		//���÷�֡��������ȡ��2�����Ựת��Ϊip������֡�ռ䣬��֡��д��fifo���ͷ���2
		DecmposeFreamData(datadstaddr, &dataTxFifo, priorty, dataclass, datatype, buf, retCnt,0);
		
		pthread_mutex_unlock(&DPFreamData_lock);//�ͷ���1		
		
		//�����첽IO����¼���ӣ�����첽д���û�buf���ſ��Ա��ͷţ�
		eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
		if(eventinfo==NULL)
			return MFI_ERROR_ALLOC;
		eventinfo->ref_count=1;
		eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
		memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //���Գ�ʼ��
		eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=psession->new_jobid;
		psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
		eventinfo->event_attr.async_io_event[2].attr_val.attr_s=buf;       //�û�����buf
		eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=dataclass;   //class
		eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=datatype;   //type
		eventinfo->event_attr.async_io_event[5].attr_val.attr_ui=datadstaddr;   //addr
		eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=retCnt;   //buf�ĳ���
		eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS_SYNC;
		eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_WRITE_DATA;
		InsertEventInfo(eventinfo);
		
		//�����¼���ָ��ṹ���봦����ƵĶ���
		InsertEventPointer(Mfi, eventinfo);
		
		if(jobId!=NULL)  *jobId=eventinfo->event_attr.async_io_event[1].attr_val.attr_ui;
		
		return MFI_SUCCESS_SYNC;
	}

	//�����첽д����
	if((task=(MfiPAsyncTaskInfo)malloc(sizeof(MfiAsyncTaskInfo)))==NULL)
		return MFI_ERROR_ALLOC;
		
	task->job_id=psession->new_jobid;
	psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
	task->job_type=MFI_ASYNC_WRITE_DATA;
	task->job_buf=buf;
	task->buf_size=retCnt;
	task->priorty=priorty;
	task->mdclass=dataclass;
	task->type=datatype;
	task->addr=datadstaddr;
	
	pthread_mutex_lock(&(psession->wt_queue.lock));
	task->next=psession->wt_queue.data_task_queue.queue_tail->next;
	task->last=psession->wt_queue.data_task_queue.queue_tail;
	task->next->last=task;
	task->last->next=task;
	psession->wt_queue.data_task_queue.queue_tail=task;
	psession->wt_queue.data_task_queue.queue_len++;
	pthread_cond_signal(&(psession->wt_queue.ready));
	pthread_mutex_unlock(&(psession->wt_queue.lock));
	
	if(jobId!=NULL)  *jobId=task->job_id;
	return MFI_SUCCESS;	
}