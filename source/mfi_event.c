#include "mfi_event.h"
#include "mfi_rsrc_manager.h"
#include "mfi_test_define.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

//事件使能数组
//功能：该数组包含系统内所有会话所有事件的使能情况；
//      资源管理器中包含指向该数组的指针；
//      event_en成员的每一位对应一个会话对该事件的使能情况
MfiEventEnInfo all_event_en[10]={
	{MFI_EVENT_IO_COMPLETION,NO_VAL},
	{MFI_EVENT_TRIG0,NO_VAL},
	{MFI_EVENT_TRIG1,NO_VAL},
	{MFI_EVENT_TRIG2,NO_VAL},
	{MFI_EVENT_TRIG3,NO_VAL},
	{MFI_EVENT_TRIG4,NO_VAL},
	{MFI_EVENT_TRIG5,NO_VAL},
	{MFI_EVENT_TRIG6,NO_VAL},
	{MFI_EVENT_TRIG7,NO_VAL},
	{MFI_EVENT_EXCEPTION,NO_VAL},
};

/*机制切换FIFO初始化函数*/
MfiStatus MfiMechChangeFifoInit(MfiPMechChangeFifo pfifo)
{
	pfifo->empty  = MFI_TRUE;
	pfifo->full   = MFI_FALSE;
	pfifo->buf_len = MECHCHANGEFIFOLEN;
	pfifo->rindex = 0;
	pfifo->windex = 0;
	
	if(pthread_mutex_init(&(pfifo->lock),MFI_NULL)!=0){
		return MFI_ERROR_SYSTEM_ERROR;
	}
	
	if(pthread_cond_init(&(pfifo->ready),MFI_NULL)!=0){
		pthread_mutex_destroy(&(pfifo->lock));
		return MFI_ERROR_SYSTEM_ERROR;
	}
	
	return MFI_SUCCESS;
}

/*机制切换FIFO清除函数*/
void MfiMechChangeFifoDelete(MfiPMechChangeFifo pfifo)
{
	pthread_mutex_lock(&(pfifo->lock));
	pfifo->empty  = MFI_TRUE;
	pfifo->full   = MFI_FALSE;
	pfifo->rindex = 0;
	pfifo->windex = 0;
	pthread_mutex_unlock(&(pfifo->lock));
	pthread_mutex_destroy(&(pfifo->lock));
	pthread_cond_destroy(&(pfifo->ready));
	
	return;
}

/*机制切换FIFO写函数*/
//功能：当一个事件的处理机制从挂起机制切换到队列机制时，调用该函数将切换任务加入FIFO，
//      并发送信号，唤醒挂起线程，等待挂起线程取出任务处理
MfiBoolean MechChangeFifoWrite(MfiPMechChangeFifo pfifo,MfiEventType eventtype)
{
	pthread_mutex_lock(&(pfifo->lock));
	
	if(pfifo->full==MFI_TRUE){
		pthread_mutex_unlock(&(pfifo->lock));
		return MFI_FALSE;
	}
	
	pfifo->buf[pfifo->windex]=eventtype;
	
	if (pfifo->windex < (pfifo->buf_len-1))
		pfifo->windex++;
	else
		pfifo->windex=0;
	
	if (pfifo->windex == pfifo->rindex)
		pfifo->full=MFI_TRUE;
	
	pfifo->empty=MFI_FALSE;

	pthread_cond_signal(&(pfifo->ready));
	pthread_mutex_unlock(&(pfifo->lock));
	
	return MFI_TRUE;
}

/*机制切换FIFO读函数*/
//功能：挂起线程从FIFO中读取切换任务，查询被切换为回调机制的事件，并处理挂起队列中对应的事件
MfiBoolean MechChangeFifoRead(MfiPMechChangeFifo pfifo,MfiEventType* peventtype)
{
	pthread_mutex_lock(&(pfifo->lock));
	
	while (pfifo->empty==MFI_TRUE){
		pthread_cond_wait(&(pfifo->ready),&(pfifo->lock));
	}
	
	*peventtype=pfifo->buf[pfifo->rindex];

	if (pfifo->rindex < (pfifo->buf_len-1))
		pfifo->rindex++;
	else
		pfifo->rindex=0;
	
	if (pfifo->rindex == pfifo->windex)
		pfifo->empty=MFI_TRUE;
		
	pfifo->full=MFI_FALSE;
	
	pthread_mutex_unlock(&(pfifo->lock));
		
	return MFI_TRUE;
}

//事件指针队列初始化函数
MfiStatus EventPointerQueueInit(MfiPEventPointerQueue queue)
{
	if(pthread_mutex_init(&(queue->lock),MFI_NULL)!=0){
		return MFI_ERROR_SYSTEM_ERROR;
	}
	
	if(pthread_cond_init(&(queue->ready),MFI_NULL)!=0){
		pthread_mutex_destroy(&(queue->lock));
		return MFI_ERROR_SYSTEM_ERROR;
	}
	
	queue->queue_len=0;
	queue->queue_head.next=&(queue->queue_head);
	queue->queue_head.last=&(queue->queue_head);
	queue->queue_tail=&(queue->queue_head);
	queue->wait_type=0;
	
	return MFI_SUCCESS;
}

//事件指针队列清理函数
MfiStatus EventPointerQueueDelete(MfiPEventPointerQueue queue)
{
	MfiPEventPointer temp=NULL;
	int i=0;
	
	pthread_mutex_lock(&(queue->lock));
	for(;i<queue->queue_len;i++){
		temp=queue->queue_head.next;
		queue->queue_head.next=temp->next;
		EventClose(temp->event_info);
		free(temp);
	}
	
	queue->queue_len=0;
	queue->queue_head.next=&(queue->queue_head);
	queue->queue_head.last=&(queue->queue_head);
	queue->queue_tail=&(queue->queue_head);
	queue->wait_type=0;
	pthread_mutex_unlock(&(queue->lock));
	
	pthread_mutex_destroy(&(queue->lock));
	pthread_cond_destroy(&(queue->ready));
	
	return MFI_SUCCESS;	
}

//查询事件函数
//功能：在事件信息队列中查询指定事件event_id，返回事件信息结构的指针
MfiPEventInfo Event_Find(MfiPEventQueue event_queue,MfiEvent event_id)
{
	int i=0;
	MfiPEventInfo temp=event_queue->queue_head.next;
	
	pthread_rwlock_rdlock(&(event_queue->queue_lock));
	while((i<event_queue->queue_len)&&(temp->event_id!=event_id))
	{
		i++;
		temp=temp->next;
	}
	
	pthread_rwlock_unlock(&(event_queue->queue_lock));
	
	return temp;
}

//清除事件信息队列
MfiStatus EventQueueDelete(void)
{
	MfiPAttribute attr=NULL;
	MfiUInt32 attr_num=0;
	MfiPEventInfo event_info=NULL;
		
	pthread_rwlock_wrlock(&(RsrcManager->event_closing.queue_lock));

	while(RsrcManager->event_closing.queue_head.next!=(&RsrcManager->event_closing.queue_head))
	{
		event_info=RsrcManager->event_closing.queue_head.next;
		RsrcManager->event_closing.queue_head.next=event_info->next;
		
		switch(event_info->event_type)
		{
			case MFI_EVENT_IO_COMPLETION:
				attr=event_info->event_attr.async_io_event;
				attr_num=ASYNC_EVENT_ATTR_NUM;
			break;
			case MFI_EVENT_TRIG0: case MFI_EVENT_TRIG1: case MFI_EVENT_TRIG2:
			case MFI_EVENT_TRIG3: case MFI_EVENT_TRIG4: case MFI_EVENT_TRIG5:
			case MFI_EVENT_TRIG6: case MFI_EVENT_TRIG7:
				attr=event_info->event_attr.trig_event;
				attr_num=TRIG_EVENT_ATTR_NUM;
			break;
			case MFI_EVENT_EXCEPTION:
				attr=event_info->event_attr.fault_detect_event;
				attr_num=FAULT_EVENT_ATTR_NUM;
			break;
		}
		AttrFree(attr, attr_num); //释放字符属性的空间
		free(event_info);
	}
	
	RsrcManager->event_closing.queue_len=0;
	pthread_rwlock_unlock(&(RsrcManager->event_closing.queue_lock));
	return MFI_SUCCESS;
}

/*
MfiStatus EventClose(MfiObject Mfi)
{
	MfiPEventInfo event_info;
	MfiPAttribute attr=NULL;
	MfiUInt32 attr_num=0;
	
	event_info=Event_Find(&(RsrcManager->event_closing),Mfi);
	if(event_info==NULL)
		return MFI_ERROR_INV_OBJECT;
	
	pthread_rwlock_wrlock(&(RsrcManager->event_closing.queue_lock));
	if(event_info->ref_count>1)
	{
		event_info->ref_count--;
		pthread_rwlock_unlock(&(RsrcManager->event_closing.queue_lock));
		return MFI_SUCCESS;
	}

	switch(event_info->event_type)
	{
		case MFI_EVENT_IO_COMPLETION:
			attr=event_info->event_attr.async_io_event;
			attr_num=ASYNC_EVENT_ATTR_NUM;
			break;
		case MFI_EVENT_TRIG0,MFI_EVENT_TRIG1,MFI_EVENT_TRIG2,MFI_EVENT_TRIG3,MFI_EVENT_TRIG4,MFI_EVENT_TRIG5,MFI_EVENT_TRIG6,MFI_EVENT_TRIG7:
			attr=event_info->event_attr.trig_event;
			attr_num=TRIG_EVENT_ATTR_NUM;
			break;
		case MFI_EVENT_EXCEPTION:
			attr=event_info->event_attr.fault_detect_event;
			attr_num=FAULT_EVENT_ATTR_NUM;
			break;
	}
	AttrFree(attr, attr_amount); //释放字符属性的空间
	
	if(event_info==RsrcManager->event_closing.insert_point)
		RsrcManager->event_closing.insert_point=event_info->last;
		
	event_info->last->next=event_info->next;
	event_info->next->last=event_info->last;
	pthread_rwlock_unlock(&(RsrcManager->event_closing.queue_lock));
	
	free(event_info);
	
	return MFI_SUCCESS;
}
*/

//事件关闭函数
//功能：用于处理事件的关闭操作
MfiStatus EventClose(MfiPEventInfo event_info)
{
	MfiPAttribute attr=NULL;
	MfiUInt32 attr_num=0;
	
	if(event_info==NULL) return MFI_SUCCESS;
		
	pthread_rwlock_wrlock(&(RsrcManager->event_closing.queue_lock));
	if(event_info->ref_count>1)
	{
		event_info->ref_count--;
		pthread_rwlock_unlock(&(RsrcManager->event_closing.queue_lock));
		return MFI_SUCCESS;
	}
	
	if(event_info==RsrcManager->event_closing.insert_point)
		RsrcManager->event_closing.insert_point=event_info->last;
		
	event_info->last->next=event_info->next;
	event_info->next->last=event_info->last;        //head的last指向链尾，链尾的next指向head
	RsrcManager->event_closing.queue_len--;
	pthread_rwlock_unlock(&(RsrcManager->event_closing.queue_lock));

	switch(event_info->event_type)
	{
		case MFI_EVENT_IO_COMPLETION:
			attr=event_info->event_attr.async_io_event;
			attr_num=ASYNC_EVENT_ATTR_NUM;
			break;
		case MFI_EVENT_TRIG0: case MFI_EVENT_TRIG1: case MFI_EVENT_TRIG2:
		case MFI_EVENT_TRIG3: case MFI_EVENT_TRIG4: case MFI_EVENT_TRIG5:
		case MFI_EVENT_TRIG6: case MFI_EVENT_TRIG7:
			attr=event_info->event_attr.trig_event;
			attr_num=TRIG_EVENT_ATTR_NUM;
			break;
		case MFI_EVENT_EXCEPTION:
			attr=event_info->event_attr.fault_detect_event;
			attr_num=FAULT_EVENT_ATTR_NUM;
			break;
	}
	AttrFree(attr, attr_num); //释放字符属性的空间	
	free(event_info);
	
	return MFI_SUCCESS;
}

//事件使能函数
//功能：使能一个指定的事件类型指定的处理机制，同时可用于切换处理机制
MfiStatus EnableEvent(MfiSession Mfi, MfiEventType eventType, MfiUInt16 mechanism)
{
	MfiStatus status;
	MfiPSessionInfo psession=NULL;
	pthread_t pid;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);

	for(;i<psession->cfg_amount;i++)
	{
		if(eventType==psession->event_cfg[i].event_type)
			break;
	}
	
	//判断会话是否支持该事件
	if(i>=psession->cfg_amount)
		return MFI_ERROR_INV_EVENT;
		
	if(mechanism==MFI_HNDLR)
	{
		/*判断是否注册了回调函数*/
		if(psession->event_cfg[i].handler==NULL)
			return MFI_ERROR_HNDLR_NINSTALLED;
		
		/*首次使能回调机制，创建回调线程*/
		if((psession->event_mech_en & MFI_HNDLR) == 0){
//			printf("EnableEvent:1!!!!!\n");
			if(0!=pthread_create(&pid,NULL,thr_event_callback,&(RsrcManager->session_list[Mfi].session)))   //创建回调机制处理线程 
				return MFI_ERROR_SYSTEM_ERROR;
			else{
				psession->pthread_id.callback_thr=pid;
				psession->event_mech_en|=MFI_HNDLR;
			}
		}
//		printf("EnableEvent:2!!!!!\n");
		//从挂起机制切换到回调机制时，唤醒挂起线程，把切换这个任务入队，等待挂起线程取出并处理对应事件
		if(psession->event_cfg[i].is_mechanism_en==MFI_SUSPEND_HNDLR){
			MechChangeFifoWrite(&(psession->mech_fifo),eventType);
		}
	}
	else if(mechanism==MFI_SUSPEND_HNDLR)
	{
		/*首次使能挂起机制，初始化机制切换队列，创建挂起线程*/
		if(psession->event_mech_en & MFI_SUSPEND_HNDLR == 0){
			if((status=MfiMechChangeFifoInit(&(psession->mech_fifo)))!=MFI_SUCCESS) //第一次使能挂起机制，初始化机制切换任务队列
				return status;
			if(0!=pthread_create(&pid,NULL,thr_event_suspend,&(RsrcManager->session_list[Mfi].session)))   //创建挂起机制处理线程
				return MFI_ERROR_SYSTEM_ERROR;
			else{
				psession->pthread_id.suspend_thr=pid;
				psession->event_mech_en|=MFI_SUSPEND_HNDLR;
			}
		}
	}
	else if(mechanism==MFI_QUEUE){
		//判断之前的处理机制是否是挂起机制  挂起机制只能切换成
		if(psession->event_cfg[i].is_mechanism_en==MFI_SUSPEND_HNDLR)
			return MFI_ERROR_INV_SETUP;
		psession->event_mech_en|=MFI_QUEUE;
	}
	else{
		//处理机制无效
		return MFI_ERROR_INV_MECH;
	}
	
//	printf("EnableEvent:event_mech_en=%d\n",psession->event_mech_en);
//	printf("EnableEvent:3!!!!!\n");
	//判断事件是否已经使能
	if(psession->event_cfg[i].is_mechanism_en>0){
		psession->event_cfg[i].is_mechanism_en=mechanism;
		return MFI_SUCCESS_EVENT_EN;
	}
		
	psession->event_cfg[i].is_mechanism_en=mechanism;
	
	//初次使能事件，将资源管理器中的事件使能管理数组对应事件对应会话的位置1
	for(i=0;i<RsrcManager->event_amount;i++){
		if(eventType!=RsrcManager->event_en[i].event_type)
			continue;
		
		RsrcManager->event_en[i].event_en |= (1<<Mfi);
		break;
	}
	
	return MFI_SUCCESS;
}

//失能事件函数
//功能：失能某一事件，使得产生的事件不会再加入相应的处理队列，
//      但是遗留在队列中的事件并不清除，除非调用discard函数
MfiStatus DisableEvent(MfiSession Mfi, MfiEventType eventType)
{
	MfiStatus status;
	MfiPSessionInfo psession=NULL;
	int i=0,j=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	if(eventType==MFI_ALL_ENABLED_EVENTS)
	{
		//注意顺序，两个for循环不可交换
		//失能事件，将资源管理器中的事件使能管理数组对应事件对应会话的位置0
		for(i=0;i<RsrcManager->event_amount;i++)
			RsrcManager->event_en[i].event_en &= (~(1<<Mfi));
			
		//设置处理机制为NOMECH
		for(i=0;i<psession->cfg_amount;i++)	
			psession->event_cfg[i].is_mechanism_en=NOMECH;
				
		return MFI_SUCCESS;
	}
	
	for(i=0;i<psession->cfg_amount;i++)
	{
		if(eventType==psession->event_cfg[i].event_type)
			break;
	}
	
	//判断会话是否支持该事件
	if(i>=psession->cfg_amount)
		return MFI_ERROR_INV_EVENT;
	
	if(psession->event_cfg[i].is_mechanism_en==NOMECH)
		return MFI_SUCCESS_EVENT_DIS;
		
	//失能事件，将资源管理器中的事件使能管理数组对应事件对应会话的位置0
	for(j=0;i<RsrcManager->event_amount;j++){
		if(eventType!=RsrcManager->event_en[j].event_type)
			continue;
		
		RsrcManager->event_en[j].event_en &= (~(1<<Mfi));
		break;
	}
	
	psession->event_cfg[i].is_mechanism_en=NOMECH;
	
	return MFI_SUCCESS;
}

//获取事件函数
//功能：从队列机制事件队列中获取指定事件类型的事件，若当前没有该事件，则根据函数参数阻塞等待或者返回。
MfiStatus WaitOnEvent(MfiSession Mfi, MfiEventType inEventType, MfiUInt32 timeout,MfiPEventType outEventType, MfiPEvent outContext)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventPointer temp=NULL;
	struct timespec now;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);	
	if(timeout==MFI_TMO_USEATTRVAL)
	{
		//获取超时属性中设置的超时值
		if((i=FindAttr(MFI_ATTR_TMO_VALUE, RsrcManager->session_list[Mfi].rsrc_attr, RsrcManager->session_list[Mfi].attr_amount))==-1){
			return MFI_ERROR_NSUP_ATTR;
		}
		timeout=RsrcManager->session_list[Mfi].rsrc_attr[i].attr_val.attr_ui;
	}
	
	if(inEventType==MFI_ALL_ENABLED_EVENTS)
	{
		for(i=0;i<psession->cfg_amount;i++)
		{
			if(psession->event_cfg[i].is_mechanism_en==QUEUEMECH)
				break;
		}
		
		//当没有事件被配置为队列机制并且超时参数非立即返回时，错误返回
		if((i>=psession->cfg_amount)&&(timeout!=MFI_TMO_IMMEDIATE))
			return MFI_ERROR_INV_SETUP; //无效的参数设置
				
		if(timeout==MFI_TMO_IMMEDIATE){
			pthread_mutex_lock(&(psession->queue_mech.lock));
			if(psession->queue_mech.queue_len==0){
				pthread_mutex_unlock(&(psession->queue_mech.lock));
				return MFI_ERROR_TMO;
			}
			
			if(psession->queue_mech.queue_len==1)
				psession->queue_mech.queue_tail=&(psession->queue_mech.queue_head);
			temp=psession->queue_mech.queue_head.next;      //从队首取出事件
			psession->queue_mech.queue_head.next=temp->next;
			temp->next->last=&(psession->queue_mech.queue_head);
			psession->queue_mech.queue_len--;
			pthread_mutex_unlock(&(psession->queue_mech.lock));
			*outEventType=temp->event_info->event_type;
			*outContext=temp->event_info->event_id;
			free(temp);
			temp=NULL;
			return MFI_SUCCESS;
		}
		
		if(timeout==MFI_TMO_INFINITE){
			pthread_mutex_lock(&(psession->queue_mech.lock));
			psession->queue_mech.wait_type=MFI_ALL_ENABLED_EVENTS;//事件产生线程，判断等待类型为任意，则直接入队，并发送信号
			while(psession->queue_mech.queue_len==0){
				pthread_cond_wait(&(psession->queue_mech.ready),&(psession->queue_mech.lock));
			}
			
			if(psession->queue_mech.queue_len==1)
				psession->queue_mech.queue_tail=&(psession->queue_mech.queue_head);
			temp=psession->queue_mech.queue_head.next;      //从队首取出事件
			psession->queue_mech.queue_head.next=temp->next;
			temp->next->last=&(psession->queue_mech.queue_head);
			psession->queue_mech.queue_len--;
			pthread_mutex_unlock(&(psession->queue_mech.lock));
			*outEventType=temp->event_info->event_type;
			*outContext=temp->event_info->event_id;
			free(temp);
			temp=NULL;
			return MFI_SUCCESS;			
		}
		
		//获取系统时间，并设置计算超时时间
		clock_gettime(CLOCK_REALTIME,&now);
		now.tv_sec=now.tv_sec+(timeout*1000000+now.tv_nsec)/1000000000;
		now.tv_nsec=(timeout*1000000+now.tv_nsec)%1000000000;		
		
		pthread_mutex_lock(&(psession->queue_mech.lock));
		psession->queue_mech.wait_type=MFI_ALL_ENABLED_EVENTS;
		while(psession->queue_mech.queue_len==0){
			if(pthread_cond_timedwait(&(psession->queue_mech.ready),&(psession->queue_mech.lock),&now)==ETIMEDOUT)
				goto error;   //等待超时
		}
		if(psession->queue_mech.queue_len==1)
			psession->queue_mech.queue_tail=&(psession->queue_mech.queue_head);
		temp=psession->queue_mech.queue_head.next;      //从队首取出事件
		psession->queue_mech.queue_head.next=temp->next;
		temp->next->last=&(psession->queue_mech.queue_head);
		psession->queue_mech.queue_len--;
		pthread_mutex_unlock(&(psession->queue_mech.lock));
		*outEventType=temp->event_info->event_type;
		*outContext=temp->event_info->event_id;
		free(temp);
		temp=NULL;
		return MFI_SUCCESS;	
	}
	else
	{
		for(i=0;i<psession->cfg_amount;i++)
		{
			if(inEventType==psession->event_cfg[i].event_type)
				break;
		}
	
		//判断会话是否支持该事件
		if(i>=psession->cfg_amount)
			return MFI_ERROR_INV_EVENT;
		else if(psession->event_cfg[i].is_mechanism_en!=QUEUEMECH)
		{
			//当获取事件没有配置为队列机制时，若设置成阻塞模式，造成无用等待，此处也可自动将timeout修改为立即返回
			if(timeout!=MFI_TMO_IMMEDIATE)
				return MFI_ERROR_INV_SETUP; //无效的参数设置
		}
			
		if(timeout==MFI_TMO_IMMEDIATE)
		{
			pthread_mutex_lock(&(psession->queue_mech.lock));
			//从队首到队尾进行查找
			temp=psession->queue_mech.queue_head.next;
			for(i=0;i<psession->queue_mech.queue_len;i++){
				if(temp->event_info->event_type!=inEventType)
					temp=temp->next;
				else{
					//找到指定事件，出队
					if(temp==psession->queue_mech.queue_tail)
						psession->queue_mech.queue_tail=temp->last;
					temp->last->next=temp->next;
					temp->next->last=temp->last;
					psession->queue_mech.queue_len--;
					pthread_mutex_unlock(&(psession->queue_mech.lock));
					*outEventType=temp->event_info->event_type;
					*outContext=temp->event_info->event_id;
					free(temp);
					temp=NULL;
					return MFI_SUCCESS;	
				}
			}
			return MFI_ERROR_TMO;
		}
		
		if(timeout==MFI_TMO_INFINITE)
		{
			pthread_mutex_lock(&(psession->queue_mech.lock));
			temp=psession->queue_mech.queue_head.next;
			for(i=0;i<psession->queue_mech.queue_len;i++){
				if(temp->event_info->event_type!=inEventType)
					temp=temp->next;
				else{
					if(temp==psession->queue_mech.queue_tail)
						psession->queue_mech.queue_tail=temp->last;
					temp->last->next=temp->next;
					temp->next->last=temp->last;
					psession->queue_mech.queue_len--;
					pthread_mutex_unlock(&(psession->queue_mech.lock));
					*outEventType=temp->event_info->event_type;
					*outContext=temp->event_info->event_id;
					free(temp);
					temp=NULL;
					return MFI_SUCCESS;	
				}
			}			
			
			//标记正在等待的事件类型，事件产生线程将事件入队时，比对事件是否为等待事件，一致则发送信号唤醒wait函数
			//该部分功能在InsertEventPointer()函数中实现
			psession->queue_mech.wait_type=inEventType;
			pthread_cond_wait(&(psession->queue_mech.ready),&(psession->queue_mech.lock));
			while(1){
				//从队尾开始查询是否有符合要求的新事件插入
				temp=psession->queue_mech.queue_tail;
				for(i=0;i<psession->queue_mech.queue_len;i++)
				{
					if(temp->event_info->event_type!=inEventType)
						temp=temp->last;
					else{
						//找到指定事件
						if(temp==psession->queue_mech.queue_tail)
							psession->queue_mech.queue_tail=temp->last;
						temp->last->next=temp->next;
						temp->next->last=temp->last;
						psession->queue_mech.queue_len--;
						psession->queue_mech.wait_type=MFI_ALL_ENABLED_EVENTS;
						pthread_mutex_unlock(&(psession->queue_mech.lock));
						*outEventType=temp->event_info->event_type;
						*outContext=temp->event_info->event_id;
						free(temp);
						temp=NULL;
						return MFI_SUCCESS;
					}
				}
				pthread_cond_wait(&(psession->queue_mech.ready),&(psession->queue_mech.lock));
			}
		}
		
		pthread_mutex_lock(&(psession->queue_mech.lock));
		temp=psession->queue_mech.queue_head.next;
		for(i=0;i<psession->queue_mech.queue_len;i++){
			if(temp->event_info->event_type!=inEventType)
				temp=temp->next;
			else{
				if(temp==psession->queue_mech.queue_tail)
					psession->queue_mech.queue_tail=temp->last;
				temp->last->next=temp->next;
				temp->next->last=temp->last;
				psession->queue_mech.queue_len--;
				pthread_mutex_unlock(&(psession->queue_mech.lock));
				*outEventType=temp->event_info->event_type;
				*outContext=temp->event_info->event_id;
				free(temp);
				temp=NULL;
				return MFI_SUCCESS;	
			}
		}			

		clock_gettime(CLOCK_REALTIME,&now);
		now.tv_sec=now.tv_sec+(timeout*1000000+now.tv_nsec)/1000000000;
		now.tv_nsec=(timeout*1000000+now.tv_nsec)%1000000000;		
			
		psession->queue_mech.wait_type=inEventType;//事件产生线程，判断等待类型为特定事件，入队，比对事件是否一致，一致则发送信号
		
		if(pthread_cond_timedwait(&(psession->queue_mech.ready),&(psession->queue_mech.lock),&now)==ETIMEDOUT)
			goto error;   //等待超时
			
		while(1){
			temp=psession->queue_mech.queue_tail;
			for(i=0;i<i<psession->queue_mech.queue_len;i++)
			{
				if(temp->event_info->event_type!=inEventType)
					temp=temp->last;
				else{
					if(temp==psession->queue_mech.queue_tail)
						psession->queue_mech.queue_tail=temp->last;
					temp->last->next=temp->next;
					temp->next->last=temp->last;
					psession->queue_mech.queue_len--;
					psession->queue_mech.wait_type=MFI_ALL_ENABLED_EVENTS;
					pthread_mutex_unlock(&(psession->queue_mech.lock));
					*outEventType=temp->event_info->event_type;
					*outContext=temp->event_info->event_id;
					free(temp);
					temp=NULL;
					return MFI_SUCCESS;	
				}
			}
			
			if(pthread_cond_timedwait(&(psession->queue_mech.ready),&(psession->queue_mech.lock),&now)==ETIMEDOUT)
				goto error;   //等待超时
		}
	}
	
	error:
		pthread_mutex_unlock(&(psession->queue_mech.lock));
		return MFI_ERROR_TMO;
}

/*******************************************************************************/
//丢弃一个会话中已经发生的指定事件类型指定处理机制的事件,适用于队列机制和挂起机制
//eventType:指定事件类型   MFI_ALL_ENABLED_EVENTS 或其他事件ID
//mechanism:事件处理机制   MFI_QUEUE MFI_SUSPEND_HNDLR MFI_ALL_MECH
/*******************************************************************************/
MfiStatus DiscardEvents(MfiSession Mfi, MfiEventType eventType, MfiUInt16 mechanism)
{
	MfiStatus status;
	MfiPSessionInfo psession=NULL;
	MfiPEventPointer temp=NULL,temp1=NULL;
	int i=0,n=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	if((mechanism!=MFI_QUEUE)&&(mechanism!=MFI_SUSPEND_HNDLR)&&(mechanism!=MFI_ALL_MECH))
		return MFI_ERROR_INV_MECH;             //机制指定错误
		
	//清除所有已产生事件
	if(eventType==MFI_ALL_ENABLED_EVENTS)
	{
		if((mechanism==MFI_ALL_MECH)||(mechanism==MFI_QUEUE))
		{
			//清理队列机制队列
			pthread_mutex_lock(&(psession->queue_mech.lock));
			for(i=0;i<psession->queue_mech.queue_len;i++){
				temp=psession->queue_mech.queue_tail;
				psession->queue_mech.queue_tail=temp->last;
				EventClose(temp->event_info);
				free(temp);
			}
			psession->queue_mech.queue_len=0;
			psession->queue_mech.queue_tail=&(psession->queue_mech.queue_head);
			psession->queue_mech.queue_head.next=&(psession->queue_mech.queue_head);
			psession->queue_mech.queue_head.last=&(psession->queue_mech.queue_head);
			pthread_mutex_unlock(&(psession->queue_mech.lock));
		}
		
		if((mechanism==MFI_ALL_MECH)||(mechanism==MFI_SUSPEND_HNDLR))	
		{
			//!!清理挂起机制队列(由于可能挂起线程正在处理队列中的事件，所以挂起线程每次取事件时，必须判断队列的长度是否为0)
			pthread_mutex_lock(&(psession->suspend_mech.lock));
			for(i=0;i<psession->suspend_mech.queue_len;i++){
				temp=psession->suspend_mech.queue_tail;
				psession->suspend_mech.queue_tail=temp->last;
				EventClose(temp->event_info);
				free(temp);
			}
			psession->suspend_mech.queue_len=0;
			psession->suspend_mech.queue_tail=&(psession->suspend_mech.queue_head);
			psession->suspend_mech.queue_head.next=&(psession->suspend_mech.queue_head);
			psession->suspend_mech.queue_head.last=&(psession->suspend_mech.queue_head);
			pthread_mutex_unlock(&(psession->suspend_mech.lock));
		}
		
		return MFI_SUCCESS;
	}
	
	//清除指定事件
	for(i=0;i<psession->cfg_amount;i++)
	{
		if(eventType==psession->event_cfg[i].event_type)
			break;
	}
	
	//判断会话是否支持该事件
	if(i>=psession->cfg_amount)
		return MFI_ERROR_INV_EVENT;
	
	if((mechanism==MFI_ALL_MECH)||(mechanism==MFI_QUEUE))
	{
		//清理队列机制队列
		pthread_mutex_lock(&(psession->queue_mech.lock));
		temp=psession->queue_mech.queue_head.next;
		for(i=1;i<psession->queue_mech.queue_len;i++){
			if(temp->event_info->event_type==eventType){
				EventClose(temp->event_info);
				temp1=temp;
				temp=temp->next;
				temp1->last->next=temp1->next;
				temp1->next->last=temp1->last;
				free(temp1);
				n++;
			}
		}
		//判断队尾
		if(i==psession->queue_mech.queue_len){
			if(temp->event_info->event_type==eventType){
				EventClose(temp->event_info);
				psession->queue_mech.queue_tail=temp->last;
				temp->last->next=temp->next;
				temp->next->last=temp->last;
				free(temp);
				n++;
			}
		}
			
		psession->queue_mech.queue_len-=n;
		n=0;
		pthread_mutex_unlock(&(psession->queue_mech.lock));
	}
	
	if((mechanism==MFI_ALL_MECH)||(mechanism==MFI_SUSPEND_HNDLR))
	{
		//清理挂起机制队列(由于可能挂起线程正在处理队列中的事件，所以挂起线程每次取事件时，必须判断队列的长度是否为0)
		pthread_mutex_lock(&(psession->suspend_mech.lock));
		temp=psession->suspend_mech.queue_head.next;
		for(i=1;i<psession->suspend_mech.queue_len;i++){
			if(temp->event_info->event_type==eventType){
				EventClose(temp->event_info);
				temp1=temp;
				temp=temp->next;
				temp1->last->next=temp1->next;
				temp1->next->last=temp1->last;
				free(temp1);
				n++;
			}
		}
		//判断队尾
		if(i==psession->suspend_mech.queue_len){
			if(temp->event_info->event_type==eventType){
				EventClose(temp->event_info);
				psession->suspend_mech.queue_tail=temp->last;
				temp->last->next=temp->next;
				temp->next->last=temp->last;
				free(temp);
				n++;
			}
		}
			
		psession->suspend_mech.queue_len-=n;
		pthread_mutex_unlock(&(psession->suspend_mech.lock));
	}	
	
	return MFI_SUCCESS;	
}

//注册函数
//功能：为指定事件注册回调函数
MfiStatus InstallHandler(MfiSession Mfi, MfiEventType eventType, MfiHndlr handler, MfiAddr userHandle)
{
	MfiStatus status;
	MfiPSessionInfo psession=NULL;
	int i=0;
	
	if(handler==NULL)
		return MFI_ERROR_INV_HNDLR_REF;
		
	psession=&(RsrcManager->session_list[Mfi]);

	for(;i<psession->cfg_amount;i++)
	{
		if(eventType==psession->event_cfg[i].event_type)
			break;
	}
	
	//判断会话是否支持该事件
	if(i>=psession->cfg_amount)
		return MFI_ERROR_INV_EVENT;	
		
	if(NULL==psession->event_cfg[i].handler)
		status=MFI_SUCCESS;
	else
		status=MFI_SUCCESS_HNDLR_REP;
	
	psession->event_cfg[i].handler=handler;
	psession->event_cfg[i].userhandle=userHandle;
	return status;
}

//功能：删除已注册的回调函数
MfiStatus UninstallHandler(MfiSession Mfi, MfiEventType eventType, MfiHndlr handler, MfiAddr userHandle)
{
	MfiStatus status;
	MfiPSessionInfo psession=NULL;
	int i=0;
		
	psession=&(RsrcManager->session_list[Mfi]);

	for(;i<psession->cfg_amount;i++)
	{
		if(eventType==psession->event_cfg[i].event_type)
			break;
	}
	
	//判断会话是否支持该事件
	if(i>=psession->cfg_amount)
		return MFI_ERROR_INV_EVENT;		
			
	if((handler!=psession->event_cfg[i].handler)||(userHandle!=psession->event_cfg[i].userhandle))
		return MFI_ERROR_INV_HNDLR_REF;
	
	psession->event_cfg[i].handler=NULL;
	psession->event_cfg[i].userhandle=NULL;
		
	if(psession->event_cfg[i].is_mechanism_en!=CALLBACKMECH)
		return MFI_SUCCESS;

  //如果是回调机制，则失能该事件
	for(i=0;i<RsrcManager->event_amount;i++){
		if(eventType!=RsrcManager->event_en[i].event_type)
			continue;
		
		RsrcManager->event_en[i].event_en &= (~(1<<Mfi));//失能事件，将资源管理器中的事件使能管理数组对应事件对应会话的位置0
		break;
	}
	
	psession->event_cfg[i].is_mechanism_en=NOMECH;
	return MFI_SUCCESS;
}

/*
void cleanup(void *arg)
{
	pthread_mutex_t* lock=NULL;
	
	lock=(pthread_mutex_t*)arg;
	pthread_mutex_unlock(lock);
	return;
}*/

//回调机制处理线程
void* thr_event_callback(void* arg)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventPointer temp=NULL;
	MfiSession Mfi;
	int i=0;
	
	//从参数中取出会话ID，并保存
	Mfi=*((MfiSession*)arg);
	psession=&(RsrcManager->session_list[Mfi]);

	#ifdef PTHREAD_TEST
	printf("thr_event_callback start: %d~~~~\n",Mfi);
	#endif
//	pthread_cleanup_push(cleanup, &(psession->callback_mech.lock)); 
	
	while(psession->pthread_id.callback_thr != -1){
		pthread_mutex_lock(&(psession->callback_mech.lock));
		while(psession->callback_mech.queue_len==0)
		{
			pthread_cond_wait(&(psession->callback_mech.ready),&(psession->callback_mech.lock));
		}
		//检查点
		if(psession->pthread_id.callback_thr == -1){
			pthread_mutex_unlock(&(psession->callback_mech.lock));
			break;
		}
//		printf("pthread_deal:1!!!!!\n");
		//回调队列中一旦有事件就取出处理
		temp=psession->callback_mech.queue_head.next;
		temp->last->next=temp->next;
		temp->next->last=temp->last;
		psession->callback_mech.queue_len--;
		if(psession->callback_mech.queue_tail==temp)
			psession->callback_mech.queue_tail=temp->last;
		pthread_mutex_unlock(&(psession->callback_mech.lock));
		
//		printf("pthread_deal:2!!!!!\n");
		for(i=0;i<psession->cfg_amount;i++)
		{
			if(temp->event_info->event_type==psession->event_cfg[i].event_type)
				break;
		}
		
		//该会话不支持当前事件类型
		if(i>=psession->cfg_amount){
			free(temp);
			temp=NULL;
			continue;
		}
		
	//	printf("pthread_deal:3!!!!!\n");
		//该该事件的处理机制已经不是回调机制
		//5.9:后续更改时，该配置结构可能也要加读写锁，回调线程会查询该结构，主线程可能修改该结构，事件产生线程也需要查询该结构以获取事件的使能机制
		if(psession->event_cfg[i].is_mechanism_en!=CALLBACKMECH){
//			printf("pthread_deal:4!!!!!\n");
			EventClose(temp->event_info);
//			printf("pthread_deal:5!!!!!\n");
			free(temp);
			temp=NULL;
//			printf("pthread_deal:6!!!!!\n");
			continue;
		}
		
		//调用回调函数处理
		//5.9：后续更改时，可对返回状态进行判断
//		printf("pthread_deal:7!!!!!\n");
		if(psession->event_cfg[i].handler!=NULL)
			psession->event_cfg[i].handler(Mfi,temp->event_info->event_type,temp->event_info->event_id,psession->event_cfg[i].userhandle);
		EventClose(temp->event_info);
		free(temp);
		temp=NULL;
	}
	
	//pthread_cleanup_pop(0);//
	
	//pthread_exit(NULL);

	#ifdef PTHREAD_TEST
	printf("thr_event_callback stop: %d~~~~\n",Mfi);
	#endif
	
	return NULL;
	
}

//挂起机制处理线程
//由于挂起线程在找到需要处理的事件时，已经将事件指针结构出队，
//所以discard之后获取锁并删除队内事件，关闭事件的信息结构，对挂起线程在处理的事件没有影响
void* thr_event_suspend(void* arg)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventPointer temp=NULL,temp1=NULL;
	MfiSession Mfi;
	MfiEventType event_type=0;
	int i=0,j=0,len=0,temp_len=0;
	
	//从参数中取出会话ID，并保存
	Mfi=*((MfiSession*)arg);
	psession=&(RsrcManager->session_list[Mfi]);
	
	#ifdef PTHREAD_TEST
	printf("thr_event_suspend start: %d~~~~\n",Mfi);
	#endif
//	pthread_cleanup_push(cleanup, &(psession->suspend_mech.lock)); 
		
	while(psession->pthread_id.suspend_thr != -1){
		//从机制切换队列中获取处理任务；没有任务时阻塞
		MechChangeFifoRead(&(psession->mech_fifo),&event_type);
		//检查点
		if(psession->pthread_id.suspend_thr == -1) break;
		
		for(j=0;j<psession->cfg_amount;j++)
		{
			if(event_type==psession->event_cfg[j].event_type)
				break;
		}

		while(psession->pthread_id.suspend_thr != -1)
		{
			pthread_mutex_lock(&(psession->suspend_mech.lock));
			//len用于记录上次执行到该处时的队列长度；
			//当队列长度与len相差不为1时，表明有新的事件插入到了挂起队列中或者挂起队列被清理。重新从队头开始查询
			if(psession->suspend_mech.queue_len!=len-1){
				temp=psession->suspend_mech.queue_head.next;
				temp_len=psession->suspend_mech.queue_len;
				i=0;
			}
			len=psession->suspend_mech.queue_len;
			
			for(;i<temp_len;i++)
			{
				if(temp->event_info->event_type==event_type)
					break;
				temp=temp->next;
			}
			
			//当前挂起队列中没有需要处理的事件
			if(i>=temp_len){
				pthread_mutex_unlock(&(psession->suspend_mech.lock));
				i=0;
				len=0;
				temp_len=0;
				break;
			}
			
			//事件出队
			temp->last->next=temp->next;
			temp->next->last=temp->last;
			temp1=temp;
			temp=temp->next;
			psession->suspend_mech.queue_len--;
			if(temp1==psession->suspend_mech.queue_tail)
				psession->suspend_mech.queue_tail=temp1->last;
			pthread_mutex_unlock(&(psession->suspend_mech.lock));
			
			//若事件的处理机制已经改回挂起机制，则删除当前事件信息结构，剩余同类事件留在挂起队列中
			if(psession->event_cfg[j].is_mechanism_en==SUSPENDMECH){
				i=0;
				len=0;
				temp_len=0;
				EventClose(temp1->event_info);
				free(temp1);
				break;
			}
			//若事件的处理机制已经改为队列或无机制，则删除挂起队列中所有该类事件
			else if(psession->event_cfg[j].is_mechanism_en==QUEUEMECH || psession->event_cfg[j].is_mechanism_en==NOMECH)
			{
				EventClose(temp1->event_info);
				free(temp1);
				i++;
				continue;
			}
			
			//5.9：后续更改时，可对返回状态进行判断
			psession->event_cfg[j].handler(Mfi,temp1->event_info->event_type,temp1->event_info->event_id,psession->event_cfg[j].userhandle);
			EventClose(temp1->event_info);
			free(temp1);
			i++;
			if(temp==&(psession->suspend_mech.queue_head)){
				i=0;
				len=0;
				temp_len=0;
				break;
			}
		}
	}
	
	#ifdef PTHREAD_TEST
	printf("thr_event_suspend stop: %d~~~~\n",Mfi);
	#endif

	return NULL;

}

//事件信息结构入队函数
//功能：为新产生的事件分配事件id，并将事件信息结构按照id的大小顺序插入到事件信息链表的合适位置
MfiStatus InsertEventInfo(MfiPEventInfo eventInfo)
{
	MfiPAttribute attr=NULL;
	MfiUInt32 attr_num=0;
	
	pthread_rwlock_wrlock(&(RsrcManager->event_closing.queue_lock));
	
	START:
	for(;RsrcManager->event_closing.insert_point->next!=&(RsrcManager->event_closing.queue_head);RsrcManager->event_closing.insert_point=RsrcManager->event_closing.insert_point->next)
	{
		if(RsrcManager->event_closing.insert_point->next->event_id==RsrcManager->event_closing.insert_point->event_id+1)
			continue;
			
		eventInfo->event_id=RsrcManager->event_closing.insert_point->event_id+1;
		eventInfo->next=RsrcManager->event_closing.insert_point->next;
		eventInfo->last=RsrcManager->event_closing.insert_point;
		eventInfo->last->next=eventInfo;
		eventInfo->next->last=eventInfo;
		RsrcManager->event_closing.insert_point=eventInfo;
		RsrcManager->event_closing.queue_len++;
		pthread_rwlock_unlock(&(RsrcManager->event_closing.queue_lock)); 
		return MFI_SUCCESS;	
	}

	if(RsrcManager->event_closing.insert_point->event_id!=EVENT_MAX_ID){
		eventInfo->event_id=RsrcManager->event_closing.insert_point->event_id+1;
		eventInfo->next=RsrcManager->event_closing.insert_point->next;   
		eventInfo->last=RsrcManager->event_closing.insert_point;         
		eventInfo->last->next=eventInfo;                                 
		eventInfo->next->last=eventInfo;
		RsrcManager->event_closing.insert_point=eventInfo;
		RsrcManager->event_closing.queue_len++;
		pthread_rwlock_unlock(&(RsrcManager->event_closing.queue_lock)); 
		return MFI_SUCCESS;	
	}            
	else
	{
		RsrcManager->event_closing.insert_point=&(RsrcManager->event_closing.queue_head);
		goto START;
	}

}

//事件指针结构入队函数
//功能：为新产生的事件创建事件指针结构，根据对应会话对该事件的配置，将事件指针结构加入到对应的处理队列中；
//      当为队列机制时，需判断等待事件类型是否为当前产生的事件，是则唤醒阻塞在waitonEvent接口的线程；
//      当为回调机制时，入队并唤醒回调线程；当为挂起机制时，入队。
MfiStatus InsertEventPointer(MfiSession Mfi, MfiPEventInfo eventInfo)
{
	MfiPEventPointer eventp=NULL;
	MfiPSessionInfo psession=NULL;
	int i=0,j=0;
	unsigned int len=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	if(SesStatusCheck(psession)==NOINUSE)
		return MFI_WARN_NULL_OBJECT;
	
	for(i=0;i<psession->cfg_amount;i++)
	{
		if(eventInfo->event_type==psession->event_cfg[i].event_type)
			break;
	}
	
	//判断会话是否支持该事件
	if(i>=psession->cfg_amount){
		SesStatusFree(psession);
		return MFI_ERROR_INV_EVENT;
	}
		
	if((j=FindAttr(MFI_ATTR_MAX_QUEUE_LENGTH, psession->rsrc_attr, psession->attr_amount))==-1){
		SesStatusFree(psession);
		return MFI_ERROR_NSUP_ATTR;
	}
	else
		len=psession->rsrc_attr[j].attr_val.attr_ui;
		
	if(psession->event_cfg[i].is_mechanism_en==NOMECH){
		SesStatusFree(psession);
		return MFI_ERROR_INV_SETUP;
	}
		
	while(eventp==NULL)
		eventp=(MfiPEventPointer)malloc(sizeof(MfiEventPointer));
		
	eventp->event_info=eventInfo;
	
	if(psession->event_cfg[i].is_mechanism_en==QUEUEMECH)
	{
		pthread_mutex_lock(&(psession->queue_mech.lock));
		if(psession->queue_mech.wait_type==MFI_ALL_ENABLED_EVENTS)
		{
			if(psession->queue_mech.queue_len>=len){
				free(eventp);
				pthread_mutex_unlock(&(psession->queue_mech.lock));
				SesStatusFree(psession);
				return MFI_ERROR_QUEUE_OVERFLOW;
			}
			
			eventp->next=psession->queue_mech.queue_tail->next;
			eventp->last=psession->queue_mech.queue_tail;
			eventp->next->last=eventp;
			eventp->last->next=eventp;
			psession->queue_mech.queue_tail=eventp;
			if(psession->queue_mech.queue_len==0)
				pthread_cond_signal(&(psession->queue_mech.ready));
			++psession->queue_mech.queue_len;
		}
		else{
			if(psession->queue_mech.queue_len>=len && eventInfo->event_type!=psession->queue_mech.wait_type){
				free(eventp);
				pthread_mutex_unlock(&(psession->queue_mech.lock));
				SesStatusFree(psession);
				return MFI_ERROR_QUEUE_OVERFLOW;
			}
			
			eventp->next=psession->queue_mech.queue_tail->next;
			eventp->last=psession->queue_mech.queue_tail;
			eventp->next->last=eventp;
			eventp->last->next=eventp;
			psession->queue_mech.queue_tail=eventp;
			if(eventInfo->event_type==psession->queue_mech.wait_type)
				pthread_cond_signal(&(psession->queue_mech.ready));
			++psession->queue_mech.queue_len;
		}
		pthread_mutex_unlock(&(psession->queue_mech.lock));
		SesStatusFree(psession);
		return MFI_SUCCESS;
	}
	else if(psession->event_cfg[i].is_mechanism_en==MFI_HNDLR)
	{
		pthread_mutex_lock(&(psession->callback_mech.lock));
		if(psession->callback_mech.queue_len>=len){
			free(eventp);
			pthread_mutex_unlock(&(psession->callback_mech.lock));
			SesStatusFree(psession);
			return MFI_ERROR_QUEUE_OVERFLOW;
		}
		eventp->next=psession->callback_mech.queue_tail->next;
		eventp->last=psession->callback_mech.queue_tail;
		eventp->next->last=eventp;
		eventp->last->next=eventp;
		psession->callback_mech.queue_tail=eventp;
		++psession->callback_mech.queue_len;
		pthread_cond_signal(&(psession->callback_mech.ready));
		pthread_mutex_unlock(&(psession->callback_mech.lock));
		SesStatusFree(psession);
		return MFI_SUCCESS;		
	}
	else if(psession->event_cfg[i].is_mechanism_en==MFI_SUSPEND_HNDLR)
	{
		pthread_mutex_lock(&(psession->suspend_mech.lock));
		if(psession->suspend_mech.queue_len>=len){
			free(eventp);
			pthread_mutex_unlock(&(psession->suspend_mech.lock));
			SesStatusFree(psession);
			return MFI_ERROR_QUEUE_OVERFLOW;
		}
		eventp->next=psession->suspend_mech.queue_tail->next;
		eventp->last=psession->suspend_mech.queue_tail;
		eventp->next->last=eventp;
		eventp->last->next=eventp;
		psession->suspend_mech.queue_tail=eventp;
		++psession->suspend_mech.queue_len;
		pthread_mutex_unlock(&(psession->suspend_mech.lock));
		SesStatusFree(psession);
		return MFI_SUCCESS;
	}
	else{
		free(eventp);
		SesStatusFree(psession);
		return MFI_ERROR_INV_SETUP;
	}
}

//该函数在关闭会话时用于关闭回调线程
MfiStatus ForCloseCallBackThr(MfiSession Mfi)
{
		MfiPSessionInfo psession=NULL;
		MfiPEventPointer eventp=NULL;
		
		psession=&(RsrcManager->session_list[Mfi]);
		
		while(eventp==NULL)
			eventp=(MfiPEventPointer)malloc(sizeof(MfiEventPointer));
		
		pthread_mutex_lock(&(psession->callback_mech.lock));
		if(psession->callback_mech.queue_len!=0){
			free(eventp);
			pthread_mutex_unlock(&(psession->callback_mech.lock));
			return MFI_SUCCESS;
		}
		eventp->event_info==NULL;
		eventp->next=psession->callback_mech.queue_tail->next;
		eventp->last=psession->callback_mech.queue_tail;
		eventp->next->last=eventp;
		eventp->last->next=eventp;
		psession->callback_mech.queue_tail=eventp;
		++psession->callback_mech.queue_len;
		pthread_cond_signal(&(psession->callback_mech.ready));
		pthread_mutex_unlock(&(psession->callback_mech.lock));	
		
		return MFI_SUCCESS;
}

//该函数在关闭会话时用于关闭挂起线程
MfiStatus ForCloseSuspendThr(MfiSession Mfi)
{
		MfiPSessionInfo psession=NULL;
		MfiPEventPointer eventp=NULL;
		
		psession=&(RsrcManager->session_list[Mfi]);
		
		MechChangeFifoWrite(&(psession->mech_fifo),0);
		
		return MFI_SUCCESS;
}