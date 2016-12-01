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

//同步读消息函数
//功能：该函数用于开放的普通资源（非总线资源）的消息的读取
//      (对于总线资源，还需获取消息的源地址、消息的大类（可能为错误或其他类型）等)
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
	
	//从fifo读取消息
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

//同步读数据函数
//功能：该函数用于开放的普通资源（非总线资源）的数据的读取
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
	
	//从fifo读取消息
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

//同步写消息函数
//功能：该函数用于开放的普通资源（非总线资源）的消息的发送
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
		if(pthread_mutex_trylock(&DPFreamMsg_lock)!=0)   //获取底层通信帧FIFO的锁1
			return MFI_ERROR_TMO;
	}
	else if(timeout==MFI_TMO_INFINITE)
		pthread_mutex_lock(&DPFreamMsg_lock);            //获取底层通信帧FIFO的锁1
	else{
		if(pthread_mutex_timedlock(&DPFreamMsg_lock,&now)==ETIMEDOUT)  //获取底层通信帧FIFO的锁1，用于抢占分帧模块
			return MFI_ERROR_TMO;
	}
	
	if(priorty==0) priorty=APP_MSG_DEF_PORITY;
	//调用分帧函数：获取锁2，将会话转换为ip，分配帧空间，分帧，写入fifo，释放锁2
	Mfi_to_Ip(Mfi,&ip);
	//sendflag==0:普通发送;1:固定时间窗发送
	DecmposeFreamMsg(ip, &msgTxFifo, priorty, APP_MSG_CLASS, msgtype, buf, retCnt,sendtype);
	
	pthread_mutex_unlock(&DPFreamMsg_lock);//释放锁1
	return MFI_SUCCESS;
}

//同步写数据函数
//功能：该函数用于开放的普通资源（非总线资源）的数据的发送
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
		if(pthread_mutex_trylock(&(DPFreamData_lock))!=0)   //获取底层通信帧FIFO的锁1
			return MFI_ERROR_TMO;
	}
	else if(timeout==MFI_TMO_INFINITE)
		pthread_mutex_lock(&(DPFreamData_lock));            //获取底层通信帧FIFO的锁1
	else{
		if(pthread_mutex_timedlock(&DPFreamData_lock,&now)==ETIMEDOUT)  //获取底层通信帧FIFO的锁1，用于抢占分帧模块
			return MFI_ERROR_TMO;
	}
		
	if(priorty==0) priorty=APP_DATA_DEF_PORITY;
	//调用分帧函数：获取锁2，将会话转换为ip，分配帧空间，分帧，写入fifo，释放锁2
	Mfi_to_Ip(Mfi,&ip);
	//sendflag==0:普通发送;1:固定时间窗发送
	DecmposeFreamData(ip, &dataTxFifo, priorty, APP_DATA_CLASS, datatype, buf, retCnt, sendtype);
	
	pthread_mutex_unlock(&DPFreamData_lock);//释放锁1
	return MFI_SUCCESS;
}

//异步任务队列初始化函数
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

//异步任务队列清理函数
MfiStatus AsyncQueueDelete(MfiPAsyncQueue queue)
{
	MfiPAsyncTaskInfo temp=NULL;
	int i=0;
	
	pthread_mutex_lock(&(queue->lock));
	for(;i<queue->msg_task_queue.queue_len;i++){
		temp=queue->msg_task_queue.queue_head.next;
		queue->msg_task_queue.queue_head.next=temp->next;
		//if(temp->job_buf) free(temp->job_buf);        //若job_buf非空，则为异步写，需要释放buf的空间 5.20改：异步写的buf由用户释放
		free(temp);
	}
	
	queue->msg_task_queue.queue_len=0;
	queue->msg_task_queue.queue_head.next=&(queue->msg_task_queue.queue_head);
	queue->msg_task_queue.queue_head.last=&(queue->msg_task_queue.queue_head);
	queue->msg_task_queue.queue_tail=&(queue->msg_task_queue.queue_head);
	
	for(;i<queue->data_task_queue.queue_len;i++){
		temp=queue->data_task_queue.queue_head.next;
		queue->data_task_queue.queue_head.next=temp->next;
		//if(temp->job_buf) free(temp->job_buf);        //若job_buf非空，则为异步写，需要释放buf的空间
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

//异步读消息函数
//功能：用于异步读消息；函数内部对用户使用该接口的条件进行了判断，要使用异步接口，必须使能了异步IO完成事件
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
	
	//判断是否使能了异步IO完成事件
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
			//从fifo读消息
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
			
			//产生异步IO完成事件入队（提供用户一个选择，忽略同步读出的处理，也通过异步事件的处理方式处理）
			eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
			if(eventinfo==NULL)
				return MFI_ERROR_ALLOC;
			eventinfo->ref_count=1;
			eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
			memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //属性初始化
			eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=psession->new_jobid;
			psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
			eventinfo->event_attr.async_io_event[2].attr_val.attr_s=*bufp;
			eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=*msgtype;
			eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=*retCnt;
			eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS_SYNC;
			eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_READ_MSG;
			InsertEventInfo(eventinfo);
			
			//产生事件的指针结构加入处理机制的队列
			InsertEventPointer(Mfi, eventinfo);
			
			if(jobId!=NULL) *jobId=eventinfo->event_attr.async_io_event[1].attr_val.attr_ui;
			
			return MFI_SUCCESS_SYNC;
		}
//		printf("ReadMsgAsync:3!\n");
		pthread_mutex_unlock(&(psession->msg_rfifo.lock)); 
	}
//	printf("ReadMsgAsync:8!\n");
	//产生异步读任务
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

//异步写消息函数
//buf必须在获取到异步IO完成事件之后才能被用户释放
MfiStatus WriteMsgAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgtype, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventInfo eventinfo=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiUInt32 ip;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	//判断是否使能了异步IO完成事件
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
	
	if(pthread_mutex_trylock(&DPFreamMsg_lock)==0) //尝试获取底层通信帧FIFO的锁1，用于抢占分帧模块
	{
//		printf("WriteMsgAsync:2!\n");
		//调用分帧函数：获取锁2，将会话转换为ip，分配帧空间，分帧，写入fifo，释放锁2
		DecmposeFreamMsg(ip, &msgTxFifo, priorty, APP_MSG_CLASS, msgtype, buf, retCnt,0);
		pthread_mutex_unlock(&DPFreamMsg_lock);//释放锁1		
		
//		printf("WriteMsgAsync:3!\n");
		
		//产生异步IO完成事件入队（完成异步写的用户buf，才可以被释放）
		eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
		if(eventinfo==NULL)
			return MFI_ERROR_ALLOC;
		eventinfo->ref_count=1;
		eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
		memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //属性初始化
		eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=psession->new_jobid;
		psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
		eventinfo->event_attr.async_io_event[2].attr_val.attr_s=buf;       //用户发送buf
		eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=APP_MSG_CLASS;
		eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=msgtype;
		eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=retCnt;   //buf的长度
		eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS_SYNC;
		eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_WRITE_MSG;
		InsertEventInfo(eventinfo);
		
		//产生事件的指针结构加入处理机制的队列
		InsertEventPointer(Mfi, eventinfo);
		
//		printf("WriteMsgAsync:4!\n");
		
		if(jobId!=NULL)  *jobId=eventinfo->event_attr.async_io_event[1].attr_val.attr_ui;
		
		return MFI_SUCCESS_SYNC;
	}

//	printf("WriteMsgAsync:5!\n");
	//产生异步写任务
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

//异步读数据函数
MfiStatus ReadDataAsync(MfiSession Mfi, MfiPUInt32 datatype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventInfo eventinfo=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiPdata datap=NULL;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	//判断是否使能了异步IO完成事件
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
			//从fifo读消息
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
			
			//产生异步IO完成事件入队（提供用户一个选择，忽略同步读出的处理，也通过异步事件的处理方式处理）
			eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
			if(eventinfo==NULL)
				return MFI_ERROR_ALLOC;
			eventinfo->ref_count=1;
			eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
			memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //属性初始化
			eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=psession->new_jobid;
			psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
			eventinfo->event_attr.async_io_event[2].attr_val.attr_s=*bufp;
			eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=*datatype;
			eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=*retCnt;   //注意后续修改
			eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS_SYNC;
			eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_READ_DATA;
			InsertEventInfo(eventinfo);
			
			//产生事件的指针结构加入处理机制的队列
			InsertEventPointer(Mfi, eventinfo);
			
			if(jobId!=NULL)  *jobId=eventinfo->event_attr.async_io_event[1].attr_val.attr_ui;
			
			return MFI_SUCCESS_SYNC;
		}
		pthread_mutex_unlock(&(psession->data_rfifo.lock)); 
	}
	
	//产生异步读任务
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

//异步写数据函数
//buf必须在获取到异步IO完成事件之后才能被用户释放
MfiStatus WriteDataAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 datatype, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventInfo eventinfo=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiUInt32 ip;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	//判断是否使能了异步IO完成事件
	for(i=0;i<psession->cfg_amount;i++)
	{
		if(MFI_EVENT_IO_COMPLETION==psession->event_cfg[i].event_type)
			break;
	}
	
	if(i>=psession->cfg_amount || psession->event_cfg[i].is_mechanism_en==NOMECH)
		return MFI_ERROR_INV_SETUP;
		
	if(priorty==0) priorty=APP_DATA_DEF_PORITY;
	Mfi_to_Ip(Mfi,&ip);
	
	if(pthread_mutex_trylock(&DPFreamData_lock)==0) //尝试获取底层通信帧FIFO的锁1，用于抢占分帧模块
	{
		//调用分帧函数：获取锁2，将会话转换为ip，分配帧空间，分帧，写入fifo，释放锁2
		DecmposeFreamData(ip, &dataTxFifo, priorty, APP_DATA_CLASS, datatype, buf, retCnt,0);
		
		pthread_mutex_unlock(&DPFreamData_lock);//释放锁1		
		
		//产生异步IO完成事件入队（完成异步写的用户buf，才可以被释放）
		eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
		if(eventinfo==NULL)
			return MFI_ERROR_ALLOC;
		eventinfo->ref_count=1;
		eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
		memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //属性初始化
		eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=psession->new_jobid;
		psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
		eventinfo->event_attr.async_io_event[2].attr_val.attr_s=buf;       //用户发送buf
		eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=APP_DATA_CLASS; 
		eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=datatype;   //type
		eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=retCnt;   //buf的长度
		eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS_SYNC;
		eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_WRITE_DATA;
		InsertEventInfo(eventinfo);
		
		//产生事件的指针结构加入处理机制的队列
		InsertEventPointer(Mfi, eventinfo);
		
		if(jobId!=NULL)  *jobId=eventinfo->event_attr.async_io_event[1].attr_val.attr_ui;
		
		return MFI_SUCCESS_SYNC;
	}

	//产生异步写任务
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

//异步读任务处理线程
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
	
	//从参数中取出会话ID，并保存
	Mfi=*((MfiSession*)arg);
	psession=&(RsrcManager->session_list[Mfi]);
	
	#ifdef PTHREAD_TEST
	printf("thr_asyncread start: %d~~~~\n",Mfi);
	#endif
	
	while(psession->pthread_id.asyncread_thr != -1){
		//异步读消息任务队列处理
		pthread_mutex_lock(&(psession->rd_queue.lock));

		if(psession->rd_queue.msg_task_queue.queue_len!=0){
			if((status=pthread_mutex_trylock(&(psession->msg_rfifo.lock)))==0){
				if(psession->msg_rfifo.empty==MFI_FALSE)
				{
					//删除异步任务
					task=psession->rd_queue.msg_task_queue.queue_head.next;
					task->last->next=task->next;
					task->next->last=task->last;
					--psession->rd_queue.msg_task_queue.queue_len;
					if(task==psession->rd_queue.msg_task_queue.queue_tail)
						psession->rd_queue.msg_task_queue.queue_tail=task->last;					
					pthread_mutex_unlock(&(psession->rd_queue.lock));
					
					//从fifo读消息
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
						
					//产生异步IO完成事件入队
					//异步IO事件并非全局事件，一个异步IO事件只可能属于一个会话，所以不必查询事件使能数组确认其是否使能
					//可以仅查寻对应会话的事件配置结构，确认其是否使能
					while(eventinfo==NULL)
						eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
					eventinfo->ref_count=1;
					eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
					memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //属性初始化
					eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=task->job_id;
					eventinfo->event_attr.async_io_event[2].attr_val.attr_s=buf;  //读出数据的buf
					eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=msgclass;
					eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=msgtype;
					eventinfo->event_attr.async_io_event[5].attr_val.attr_ui=addr;
					eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=retCnt;   //注意后续修改 读出数据的长度
					eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS;
					eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_READ_MSG;
					InsertEventInfo(eventinfo);
					free(task);
					
					//产生事件的指针结构加入处理机制的队列
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
			
		//异步读数据队列处理
		pthread_mutex_lock(&(psession->rd_queue.lock));
		
		if(psession->rd_queue.data_task_queue.queue_len!=0){
			if((status=pthread_mutex_trylock(&(psession->data_rfifo.lock)))==0){
				if(psession->data_rfifo.empty==MFI_FALSE)
				{
					//删除异步任务
					task=psession->rd_queue.data_task_queue.queue_head.next;
					task->last->next=task->next;
					task->next->last=task->last;
					--psession->rd_queue.data_task_queue.queue_len;
					if(task==psession->rd_queue.data_task_queue.queue_tail)
						psession->rd_queue.data_task_queue.queue_tail=task->last;
					pthread_mutex_unlock(&(psession->rd_queue.lock));					
					//从fifo读数据
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
							
					//产生异步IO完成事件入队
					while(eventinfo==NULL)
						eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
					eventinfo->ref_count=1;
					eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
					memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //属性初始化
					eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=task->job_id;
					eventinfo->event_attr.async_io_event[2].attr_val.attr_s=buf;  //读出数据的buf
					eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=dataclass;
					eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=datatype;
					eventinfo->event_attr.async_io_event[5].attr_val.attr_ui=addr;
					eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=retCnt;   //注意后续修改 读出数据的长度
					eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS;
					eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_READ_DATA;
					InsertEventInfo(eventinfo);
					free(task);
					
					//产生事件的指针结构加入处理机制的队列
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

//异步写任务处理线程
void* thr_asyncwrite(void* arg)
{
	MfiPSessionInfo psession=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiSession Mfi;
	MfiPEventInfo eventinfo=NULL;
	MfiUInt32 ip;
	int status=0;
	
	//从参数中取出会话ID，并保存
	Mfi=*((MfiSession*)arg);
	psession=&(RsrcManager->session_list[Mfi]);
	
	#ifdef PTHREAD_TEST
	printf("thr_asyncwrite start: %d~~~~\n",Mfi);
	#endif
	while(psession->pthread_id.asyncwrite_thr != -1){
		//异步写消息任务队列处理
		pthread_mutex_lock(&(psession->wt_queue.lock));

		if(psession->wt_queue.msg_task_queue.queue_len!=0){
			if((status=pthread_mutex_trylock(&DPFreamMsg_lock))==0) //尝试获取底层通信帧FIFO的锁1，用于抢占分帧模块
			{   
				//删除异步任务
				task=psession->wt_queue.msg_task_queue.queue_head.next;
				task->last->next=task->next;
				task->next->last=task->last;
				--psession->wt_queue.msg_task_queue.queue_len;
				if(task==psession->wt_queue.msg_task_queue.queue_tail)
					psession->wt_queue.msg_task_queue.queue_tail=task->last;
					
				pthread_mutex_unlock(&(psession->wt_queue.lock));
				
				//调用分帧函数：获取锁2，将会话转换为ip，分配帧空间，分帧，写入fifo，释放锁2
				DecmposeFreamMsg(task->addr, &msgTxFifo, task->priorty, task->mdclass, task->type, task->job_buf, task->buf_size,0);
				
				pthread_mutex_unlock(&DPFreamMsg_lock);//释放锁1		
				
				//产生异步IO完成事件入队（完成异步写的用户buf，才可以被释放）
				while(eventinfo==NULL)
					eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
				eventinfo->ref_count=1;
				eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
				memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //属性初始化
				eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=task->job_id;
				eventinfo->event_attr.async_io_event[2].attr_val.attr_s=task->job_buf;       //用户发送buf
				eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=task->mdclass; 
				eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=task->type;
				eventinfo->event_attr.async_io_event[5].attr_val.attr_ui=task->addr;
				eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=task->buf_size;   //buf的长度
				eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS;
				eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_WRITE_MSG;
				InsertEventInfo(eventinfo);
				free(task);
				
				//产生事件的指针结构加入处理机制的队列
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
			
		//异步写数据队列处理
		pthread_mutex_lock(&(psession->wt_queue.lock));

		if(psession->wt_queue.data_task_queue.queue_len!=0){
			if((status=pthread_mutex_trylock(&DPFreamData_lock))==0)  //尝试获取底层通信帧FIFO的锁1，用于抢占分帧模块
			{  
				//删除异步任务
				task=psession->wt_queue.data_task_queue.queue_head.next;
				task->last->next=task->next;
				task->next->last=task->last;
				--psession->wt_queue.data_task_queue.queue_len;
				if(task==psession->wt_queue.data_task_queue.queue_tail)
					psession->wt_queue.data_task_queue.queue_tail=task->last;
					
				pthread_mutex_unlock(&(psession->wt_queue.lock));
								
				DecmposeFreamData(task->addr, &dataTxFifo, task->priorty, task->mdclass, task->type, task->job_buf, task->buf_size,0);
				
				pthread_mutex_unlock(&DPFreamData_lock);//释放锁1
				
				//产生异步IO完成事件入队（完成异步写的用户buf，才可以被释放）
				while(eventinfo==NULL)
					eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
				eventinfo->ref_count=1;
				eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
				memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //属性初始化
				eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=task->job_id;
				eventinfo->event_attr.async_io_event[2].attr_val.attr_s=task->job_buf;       //用户发送buf
				eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=task->mdclass; 
				eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=task->type;
				eventinfo->event_attr.async_io_event[5].attr_val.attr_ui=task->addr;
				eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=task->buf_size;   //buf的长度
				eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS;
				eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_WRITE_DATA;
				InsertEventInfo(eventinfo);
				free(task);
				
				//产生事件的指针结构加入处理机制的队列
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

//该函数在关闭会话时用于关闭异步写线程
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

//该函数在关闭会话时用于关闭异步读线程
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
	
	//从fifo读取消息
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
	
	//从fifo读取消息
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

//总线资源(系统)同步写接口
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
		if(pthread_mutex_trylock(&DPFreamMsg_lock)!=0)   //获取底层通信帧FIFO的锁1
			return MFI_ERROR_TMO;
	}
	else if(timeout==MFI_TMO_INFINITE)
		pthread_mutex_lock(&DPFreamMsg_lock);            //获取底层通信帧FIFO的锁1
	else{
		if(pthread_mutex_timedlock(&DPFreamMsg_lock,&now)==ETIMEDOUT)  //获取底层通信帧FIFO的锁1，用于抢占分帧模块
			return MFI_ERROR_TMO;
	}
		
	//调用分帧函数：获取锁2，将会话转换为ip，分配帧空间，分帧，写入fifo，释放锁2
	DecmposeFreamMsg(msgdstaddr, &msgTxFifo, priorty, msgclass, msgtype, buf, retCnt,sendtype);
	
	pthread_mutex_unlock(&DPFreamMsg_lock);//释放锁1
	return MFI_SUCCESS;
}

//同步写数据函数
//功能：该函数用于总线资源的数据的发送
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
		if(pthread_mutex_trylock(&(DPFreamData_lock))!=0)   //获取底层通信帧FIFO的锁1
			return MFI_ERROR_TMO;
	}
	else if(timeout==MFI_TMO_INFINITE)
		pthread_mutex_lock(&(DPFreamData_lock));            //获取底层通信帧FIFO的锁1
	else{
		if(pthread_mutex_timedlock(&DPFreamData_lock,&now)==ETIMEDOUT)  //获取底层通信帧FIFO的锁1，用于抢占分帧模块
			return MFI_ERROR_TMO;
	}
		
	//调用分帧函数：获取锁2，将会话转换为ip，分配帧空间，分帧，写入fifo，释放锁2
	DecmposeFreamData(datadstaddr, &dataTxFifo, priorty, dataclass, datatype, buf, retCnt,sendtype);
	
	pthread_mutex_unlock(&DPFreamData_lock);//释放锁1
	return MFI_SUCCESS;
}

//总线异步读消息函数
//功能：用于异步读消息；函数内部对用户使用该接口的条件进行了判断，要使用异步接口，必须使能了异步IO完成事件
MfiStatus SysReadMsgAsync(MfiSession Mfi, MfiPUInt32 msgclass, MfiPUInt32 msgtype, MfiPUInt32 msgsrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventInfo eventinfo=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiPMessage msgp=NULL;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	//判断是否使能了异步IO完成事件
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
			//从fifo读消息
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
			
			//产生异步IO完成事件入队（提供用户一个选择，忽略同步读出的处理，也通过异步事件的处理方式处理）
			eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
			if(eventinfo==NULL)
				return MFI_ERROR_ALLOC;
			eventinfo->ref_count=1;
			eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
			memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //属性初始化
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
			
			//产生事件的指针结构加入处理机制的队列
			InsertEventPointer(Mfi, eventinfo);
			
			if(jobId!=NULL)  *jobId=eventinfo->event_attr.async_io_event[1].attr_val.attr_ui;
			
			return MFI_SUCCESS_SYNC;
		}
		pthread_mutex_unlock(&(psession->msg_rfifo.lock)); 
	}
	
	//产生异步读任务
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

//异步写消息函数
//buf必须在获取到异步IO完成事件之后才能被用户释放
MfiStatus SysWriteMsgAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgclass, MfiUInt32 msgtype, MfiUInt32 msgdstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventInfo eventinfo=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiUInt32 ip;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	//判断是否使能了异步IO完成事件
	for(i=0;i<psession->cfg_amount;i++)
	{
		if(MFI_EVENT_IO_COMPLETION==psession->event_cfg[i].event_type)
			break;
	}
	
	if(i>=psession->cfg_amount || psession->event_cfg[i].is_mechanism_en==NOMECH)
		return MFI_ERROR_INV_SETUP;
		
	if(pthread_mutex_trylock(&DPFreamMsg_lock)==0) //尝试获取底层通信帧FIFO的锁1，用于抢占分帧模块
	{
		//调用分帧函数：获取锁2，将会话转换为ip，分配帧空间，分帧，写入fifo，释放锁2
		DecmposeFreamMsg(msgdstaddr, &msgTxFifo, priorty, msgclass, msgtype, buf, retCnt,0);
		pthread_mutex_unlock(&DPFreamMsg_lock);//释放锁1		
		
		//产生异步IO完成事件入队（完成异步写的用户buf，才可以被释放）
		eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
		if(eventinfo==NULL)
			return MFI_ERROR_ALLOC;
		eventinfo->ref_count=1;
		eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
		memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //属性初始化
		eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=psession->new_jobid;
		psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
		eventinfo->event_attr.async_io_event[2].attr_val.attr_s=buf;       //用户发送buf
		eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=msgclass;
		eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=msgtype;
		eventinfo->event_attr.async_io_event[5].attr_val.attr_ui=msgdstaddr;
		eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=retCnt;
		eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS_SYNC;
		eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_WRITE_MSG;
		InsertEventInfo(eventinfo);
		
		//产生事件的指针结构加入处理机制的队列
		InsertEventPointer(Mfi, eventinfo);
		
		if(jobId!=NULL)  *jobId=eventinfo->event_attr.async_io_event[1].attr_val.attr_ui;
		
		return MFI_SUCCESS_SYNC;
	}

	//产生异步写任务
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

//异步读数据函数
MfiStatus SysReadDataAsync(MfiSession Mfi, MfiPUInt32 dataclass, MfiPUInt32 datatype, MfiPUInt32 datasrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventInfo eventinfo=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiPdata datap=NULL;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	//判断是否使能了异步IO完成事件
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
			//从fifo读消息
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
			
			//产生异步IO完成事件入队（提供用户一个选择，忽略同步读出的处理，也通过异步事件的处理方式处理）
			eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
			if(eventinfo==NULL)
				return MFI_ERROR_ALLOC;
			eventinfo->ref_count=1;
			eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
			memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //属性初始化
			eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=psession->new_jobid;
			psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
			eventinfo->event_attr.async_io_event[2].attr_val.attr_s=*bufp;
			eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=*dataclass;
			eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=*datatype;
			eventinfo->event_attr.async_io_event[5].attr_val.attr_ui=*datasrcaddr;
			eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=*retCnt;   //注意后续修改
			eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS_SYNC;
			eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_READ_DATA;
			InsertEventInfo(eventinfo);
			
			//产生事件的指针结构加入处理机制的队列
			InsertEventPointer(Mfi, eventinfo);
			
			if(jobId!=NULL)  *jobId=eventinfo->event_attr.async_io_event[1].attr_val.attr_ui;
			
			return MFI_SUCCESS_SYNC;
		}
		pthread_mutex_unlock(&(psession->data_rfifo.lock)); 
	}
	
	//产生异步读任务
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

//异步写数据函数
//buf必须在获取到异步IO完成事件之后才能被用户释放
MfiStatus SysWriteDataAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 dataclass, MfiUInt32 datatype, MfiUInt32 datadstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventInfo eventinfo=NULL;
	MfiPAsyncTaskInfo task=NULL;
	MfiUInt32 ip;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	//判断是否使能了异步IO完成事件
	for(i=0;i<psession->cfg_amount;i++)
	{
		if(MFI_EVENT_IO_COMPLETION==psession->event_cfg[i].event_type)
			break;
	}
	
	if(i>=psession->cfg_amount || psession->event_cfg[i].is_mechanism_en==NOMECH)
		return MFI_ERROR_INV_SETUP;
		
	if(pthread_mutex_trylock(&DPFreamData_lock)==0) //尝试获取底层通信帧FIFO的锁1，用于抢占分帧模块
	{
		//调用分帧函数：获取锁2，将会话转换为ip，分配帧空间，分帧，写入fifo，释放锁2
		DecmposeFreamData(datadstaddr, &dataTxFifo, priorty, dataclass, datatype, buf, retCnt,0);
		
		pthread_mutex_unlock(&DPFreamData_lock);//释放锁1		
		
		//产生异步IO完成事件入队（完成异步写的用户buf，才可以被释放）
		eventinfo=(MfiPEventInfo)malloc(sizeof(MfiEventInfo));
		if(eventinfo==NULL)
			return MFI_ERROR_ALLOC;
		eventinfo->ref_count=1;
		eventinfo->event_type=MFI_EVENT_IO_COMPLETION;
		memcpy(eventinfo->event_attr.async_io_event,AsyncIOEventAttr,sizeof(AsyncIOEventAttr)); //属性初始化
		eventinfo->event_attr.async_io_event[1].attr_val.attr_ui=psession->new_jobid;
		psession->new_jobid=(psession->new_jobid%0xFFFFFFFF)+1;
		eventinfo->event_attr.async_io_event[2].attr_val.attr_s=buf;       //用户发送buf
		eventinfo->event_attr.async_io_event[3].attr_val.attr_ui=dataclass;   //class
		eventinfo->event_attr.async_io_event[4].attr_val.attr_ui=datatype;   //type
		eventinfo->event_attr.async_io_event[5].attr_val.attr_ui=datadstaddr;   //addr
		eventinfo->event_attr.async_io_event[6].attr_val.attr_ui=retCnt;   //buf的长度
		eventinfo->event_attr.async_io_event[7].attr_val.attr_i=MFI_SUCCESS_SYNC;
		eventinfo->event_attr.async_io_event[8].attr_val.attr_ui=MFI_ASYNC_WRITE_DATA;
		InsertEventInfo(eventinfo);
		
		//产生事件的指针结构加入处理机制的队列
		InsertEventPointer(Mfi, eventinfo);
		
		if(jobId!=NULL)  *jobId=eventinfo->event_attr.async_io_event[1].attr_val.attr_ui;
		
		return MFI_SUCCESS_SYNC;
	}

	//产生异步写任务
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