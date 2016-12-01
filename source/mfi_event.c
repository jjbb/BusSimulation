#include "mfi_event.h"
#include "mfi_rsrc_manager.h"
#include "mfi_test_define.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

//�¼�ʹ������
//���ܣ����������ϵͳ�����лỰ�����¼���ʹ�������
//      ��Դ�������а���ָ��������ָ�룻
//      event_en��Ա��ÿһλ��Ӧһ���Ự�Ը��¼���ʹ�����
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

/*�����л�FIFO��ʼ������*/
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

/*�����л�FIFO�������*/
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

/*�����л�FIFOд����*/
//���ܣ���һ���¼��Ĵ������ƴӹ�������л������л���ʱ�����øú������л��������FIFO��
//      �������źţ����ѹ����̣߳��ȴ������߳�ȡ��������
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

/*�����л�FIFO������*/
//���ܣ������̴߳�FIFO�ж�ȡ�л����񣬲�ѯ���л�Ϊ�ص����Ƶ��¼�����������������ж�Ӧ���¼�
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

//�¼�ָ����г�ʼ������
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

//�¼�ָ�������������
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

//��ѯ�¼�����
//���ܣ����¼���Ϣ�����в�ѯָ���¼�event_id�������¼���Ϣ�ṹ��ָ��
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

//����¼���Ϣ����
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
		AttrFree(attr, attr_num); //�ͷ��ַ����ԵĿռ�
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
	AttrFree(attr, attr_amount); //�ͷ��ַ����ԵĿռ�
	
	if(event_info==RsrcManager->event_closing.insert_point)
		RsrcManager->event_closing.insert_point=event_info->last;
		
	event_info->last->next=event_info->next;
	event_info->next->last=event_info->last;
	pthread_rwlock_unlock(&(RsrcManager->event_closing.queue_lock));
	
	free(event_info);
	
	return MFI_SUCCESS;
}
*/

//�¼��رպ���
//���ܣ����ڴ����¼��Ĺرղ���
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
	event_info->next->last=event_info->last;        //head��lastָ����β����β��nextָ��head
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
	AttrFree(attr, attr_num); //�ͷ��ַ����ԵĿռ�	
	free(event_info);
	
	return MFI_SUCCESS;
}

//�¼�ʹ�ܺ���
//���ܣ�ʹ��һ��ָ�����¼�����ָ���Ĵ������ƣ�ͬʱ�������л���������
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
	
	//�жϻỰ�Ƿ�֧�ָ��¼�
	if(i>=psession->cfg_amount)
		return MFI_ERROR_INV_EVENT;
		
	if(mechanism==MFI_HNDLR)
	{
		/*�ж��Ƿ�ע���˻ص�����*/
		if(psession->event_cfg[i].handler==NULL)
			return MFI_ERROR_HNDLR_NINSTALLED;
		
		/*�״�ʹ�ܻص����ƣ������ص��߳�*/
		if((psession->event_mech_en & MFI_HNDLR) == 0){
//			printf("EnableEvent:1!!!!!\n");
			if(0!=pthread_create(&pid,NULL,thr_event_callback,&(RsrcManager->session_list[Mfi].session)))   //�����ص����ƴ����߳� 
				return MFI_ERROR_SYSTEM_ERROR;
			else{
				psession->pthread_id.callback_thr=pid;
				psession->event_mech_en|=MFI_HNDLR;
			}
		}
//		printf("EnableEvent:2!!!!!\n");
		//�ӹ�������л����ص�����ʱ�����ѹ����̣߳����л����������ӣ��ȴ������߳�ȡ����������Ӧ�¼�
		if(psession->event_cfg[i].is_mechanism_en==MFI_SUSPEND_HNDLR){
			MechChangeFifoWrite(&(psession->mech_fifo),eventType);
		}
	}
	else if(mechanism==MFI_SUSPEND_HNDLR)
	{
		/*�״�ʹ�ܹ�����ƣ���ʼ�������л����У����������߳�*/
		if(psession->event_mech_en & MFI_SUSPEND_HNDLR == 0){
			if((status=MfiMechChangeFifoInit(&(psession->mech_fifo)))!=MFI_SUCCESS) //��һ��ʹ�ܹ�����ƣ���ʼ�������л��������
				return status;
			if(0!=pthread_create(&pid,NULL,thr_event_suspend,&(RsrcManager->session_list[Mfi].session)))   //����������ƴ����߳�
				return MFI_ERROR_SYSTEM_ERROR;
			else{
				psession->pthread_id.suspend_thr=pid;
				psession->event_mech_en|=MFI_SUSPEND_HNDLR;
			}
		}
	}
	else if(mechanism==MFI_QUEUE){
		//�ж�֮ǰ�Ĵ��������Ƿ��ǹ������  �������ֻ���л���
		if(psession->event_cfg[i].is_mechanism_en==MFI_SUSPEND_HNDLR)
			return MFI_ERROR_INV_SETUP;
		psession->event_mech_en|=MFI_QUEUE;
	}
	else{
		//����������Ч
		return MFI_ERROR_INV_MECH;
	}
	
//	printf("EnableEvent:event_mech_en=%d\n",psession->event_mech_en);
//	printf("EnableEvent:3!!!!!\n");
	//�ж��¼��Ƿ��Ѿ�ʹ��
	if(psession->event_cfg[i].is_mechanism_en>0){
		psession->event_cfg[i].is_mechanism_en=mechanism;
		return MFI_SUCCESS_EVENT_EN;
	}
		
	psession->event_cfg[i].is_mechanism_en=mechanism;
	
	//����ʹ���¼�������Դ�������е��¼�ʹ�ܹ��������Ӧ�¼���Ӧ�Ự��λ��1
	for(i=0;i<RsrcManager->event_amount;i++){
		if(eventType!=RsrcManager->event_en[i].event_type)
			continue;
		
		RsrcManager->event_en[i].event_en |= (1<<Mfi);
		break;
	}
	
	return MFI_SUCCESS;
}

//ʧ���¼�����
//���ܣ�ʧ��ĳһ�¼���ʹ�ò������¼������ټ�����Ӧ�Ĵ������У�
//      ���������ڶ����е��¼�������������ǵ���discard����
MfiStatus DisableEvent(MfiSession Mfi, MfiEventType eventType)
{
	MfiStatus status;
	MfiPSessionInfo psession=NULL;
	int i=0,j=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	if(eventType==MFI_ALL_ENABLED_EVENTS)
	{
		//ע��˳������forѭ�����ɽ���
		//ʧ���¼�������Դ�������е��¼�ʹ�ܹ��������Ӧ�¼���Ӧ�Ự��λ��0
		for(i=0;i<RsrcManager->event_amount;i++)
			RsrcManager->event_en[i].event_en &= (~(1<<Mfi));
			
		//���ô�������ΪNOMECH
		for(i=0;i<psession->cfg_amount;i++)	
			psession->event_cfg[i].is_mechanism_en=NOMECH;
				
		return MFI_SUCCESS;
	}
	
	for(i=0;i<psession->cfg_amount;i++)
	{
		if(eventType==psession->event_cfg[i].event_type)
			break;
	}
	
	//�жϻỰ�Ƿ�֧�ָ��¼�
	if(i>=psession->cfg_amount)
		return MFI_ERROR_INV_EVENT;
	
	if(psession->event_cfg[i].is_mechanism_en==NOMECH)
		return MFI_SUCCESS_EVENT_DIS;
		
	//ʧ���¼�������Դ�������е��¼�ʹ�ܹ��������Ӧ�¼���Ӧ�Ự��λ��0
	for(j=0;i<RsrcManager->event_amount;j++){
		if(eventType!=RsrcManager->event_en[j].event_type)
			continue;
		
		RsrcManager->event_en[j].event_en &= (~(1<<Mfi));
		break;
	}
	
	psession->event_cfg[i].is_mechanism_en=NOMECH;
	
	return MFI_SUCCESS;
}

//��ȡ�¼�����
//���ܣ��Ӷ��л����¼������л�ȡָ���¼����͵��¼�������ǰû�и��¼�������ݺ������������ȴ����߷��ء�
MfiStatus WaitOnEvent(MfiSession Mfi, MfiEventType inEventType, MfiUInt32 timeout,MfiPEventType outEventType, MfiPEvent outContext)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventPointer temp=NULL;
	struct timespec now;
	int i=0;
	
	psession=&(RsrcManager->session_list[Mfi]);	
	if(timeout==MFI_TMO_USEATTRVAL)
	{
		//��ȡ��ʱ���������õĳ�ʱֵ
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
		
		//��û���¼�������Ϊ���л��Ʋ��ҳ�ʱ��������������ʱ�����󷵻�
		if((i>=psession->cfg_amount)&&(timeout!=MFI_TMO_IMMEDIATE))
			return MFI_ERROR_INV_SETUP; //��Ч�Ĳ�������
				
		if(timeout==MFI_TMO_IMMEDIATE){
			pthread_mutex_lock(&(psession->queue_mech.lock));
			if(psession->queue_mech.queue_len==0){
				pthread_mutex_unlock(&(psession->queue_mech.lock));
				return MFI_ERROR_TMO;
			}
			
			if(psession->queue_mech.queue_len==1)
				psession->queue_mech.queue_tail=&(psession->queue_mech.queue_head);
			temp=psession->queue_mech.queue_head.next;      //�Ӷ���ȡ���¼�
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
			psession->queue_mech.wait_type=MFI_ALL_ENABLED_EVENTS;//�¼������̣߳��жϵȴ�����Ϊ���⣬��ֱ����ӣ��������ź�
			while(psession->queue_mech.queue_len==0){
				pthread_cond_wait(&(psession->queue_mech.ready),&(psession->queue_mech.lock));
			}
			
			if(psession->queue_mech.queue_len==1)
				psession->queue_mech.queue_tail=&(psession->queue_mech.queue_head);
			temp=psession->queue_mech.queue_head.next;      //�Ӷ���ȡ���¼�
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
		
		//��ȡϵͳʱ�䣬�����ü��㳬ʱʱ��
		clock_gettime(CLOCK_REALTIME,&now);
		now.tv_sec=now.tv_sec+(timeout*1000000+now.tv_nsec)/1000000000;
		now.tv_nsec=(timeout*1000000+now.tv_nsec)%1000000000;		
		
		pthread_mutex_lock(&(psession->queue_mech.lock));
		psession->queue_mech.wait_type=MFI_ALL_ENABLED_EVENTS;
		while(psession->queue_mech.queue_len==0){
			if(pthread_cond_timedwait(&(psession->queue_mech.ready),&(psession->queue_mech.lock),&now)==ETIMEDOUT)
				goto error;   //�ȴ���ʱ
		}
		if(psession->queue_mech.queue_len==1)
			psession->queue_mech.queue_tail=&(psession->queue_mech.queue_head);
		temp=psession->queue_mech.queue_head.next;      //�Ӷ���ȡ���¼�
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
	
		//�жϻỰ�Ƿ�֧�ָ��¼�
		if(i>=psession->cfg_amount)
			return MFI_ERROR_INV_EVENT;
		else if(psession->event_cfg[i].is_mechanism_en!=QUEUEMECH)
		{
			//����ȡ�¼�û������Ϊ���л���ʱ�������ó�����ģʽ��������õȴ����˴�Ҳ���Զ���timeout�޸�Ϊ��������
			if(timeout!=MFI_TMO_IMMEDIATE)
				return MFI_ERROR_INV_SETUP; //��Ч�Ĳ�������
		}
			
		if(timeout==MFI_TMO_IMMEDIATE)
		{
			pthread_mutex_lock(&(psession->queue_mech.lock));
			//�Ӷ��׵���β���в���
			temp=psession->queue_mech.queue_head.next;
			for(i=0;i<psession->queue_mech.queue_len;i++){
				if(temp->event_info->event_type!=inEventType)
					temp=temp->next;
				else{
					//�ҵ�ָ���¼�������
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
			
			//������ڵȴ����¼����ͣ��¼������߳̽��¼����ʱ���ȶ��¼��Ƿ�Ϊ�ȴ��¼���һ�������źŻ���wait����
			//�ò��ֹ�����InsertEventPointer()������ʵ��
			psession->queue_mech.wait_type=inEventType;
			pthread_cond_wait(&(psession->queue_mech.ready),&(psession->queue_mech.lock));
			while(1){
				//�Ӷ�β��ʼ��ѯ�Ƿ��з���Ҫ������¼�����
				temp=psession->queue_mech.queue_tail;
				for(i=0;i<psession->queue_mech.queue_len;i++)
				{
					if(temp->event_info->event_type!=inEventType)
						temp=temp->last;
					else{
						//�ҵ�ָ���¼�
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
			
		psession->queue_mech.wait_type=inEventType;//�¼������̣߳��жϵȴ�����Ϊ�ض��¼�����ӣ��ȶ��¼��Ƿ�һ�£�һ�������ź�
		
		if(pthread_cond_timedwait(&(psession->queue_mech.ready),&(psession->queue_mech.lock),&now)==ETIMEDOUT)
			goto error;   //�ȴ���ʱ
			
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
				goto error;   //�ȴ���ʱ
		}
	}
	
	error:
		pthread_mutex_unlock(&(psession->queue_mech.lock));
		return MFI_ERROR_TMO;
}

/*******************************************************************************/
//����һ���Ự���Ѿ�������ָ���¼�����ָ���������Ƶ��¼�,�����ڶ��л��ƺ͹������
//eventType:ָ���¼�����   MFI_ALL_ENABLED_EVENTS �������¼�ID
//mechanism:�¼���������   MFI_QUEUE MFI_SUSPEND_HNDLR MFI_ALL_MECH
/*******************************************************************************/
MfiStatus DiscardEvents(MfiSession Mfi, MfiEventType eventType, MfiUInt16 mechanism)
{
	MfiStatus status;
	MfiPSessionInfo psession=NULL;
	MfiPEventPointer temp=NULL,temp1=NULL;
	int i=0,n=0;
	
	psession=&(RsrcManager->session_list[Mfi]);
	
	if((mechanism!=MFI_QUEUE)&&(mechanism!=MFI_SUSPEND_HNDLR)&&(mechanism!=MFI_ALL_MECH))
		return MFI_ERROR_INV_MECH;             //����ָ������
		
	//��������Ѳ����¼�
	if(eventType==MFI_ALL_ENABLED_EVENTS)
	{
		if((mechanism==MFI_ALL_MECH)||(mechanism==MFI_QUEUE))
		{
			//�������л��ƶ���
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
			//!!����������ƶ���(���ڿ��ܹ����߳����ڴ��������е��¼������Թ����߳�ÿ��ȡ�¼�ʱ�������ж϶��еĳ����Ƿ�Ϊ0)
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
	
	//���ָ���¼�
	for(i=0;i<psession->cfg_amount;i++)
	{
		if(eventType==psession->event_cfg[i].event_type)
			break;
	}
	
	//�жϻỰ�Ƿ�֧�ָ��¼�
	if(i>=psession->cfg_amount)
		return MFI_ERROR_INV_EVENT;
	
	if((mechanism==MFI_ALL_MECH)||(mechanism==MFI_QUEUE))
	{
		//�������л��ƶ���
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
		//�ж϶�β
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
		//����������ƶ���(���ڿ��ܹ����߳����ڴ��������е��¼������Թ����߳�ÿ��ȡ�¼�ʱ�������ж϶��еĳ����Ƿ�Ϊ0)
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
		//�ж϶�β
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

//ע�ắ��
//���ܣ�Ϊָ���¼�ע��ص�����
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
	
	//�жϻỰ�Ƿ�֧�ָ��¼�
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

//���ܣ�ɾ����ע��Ļص�����
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
	
	//�жϻỰ�Ƿ�֧�ָ��¼�
	if(i>=psession->cfg_amount)
		return MFI_ERROR_INV_EVENT;		
			
	if((handler!=psession->event_cfg[i].handler)||(userHandle!=psession->event_cfg[i].userhandle))
		return MFI_ERROR_INV_HNDLR_REF;
	
	psession->event_cfg[i].handler=NULL;
	psession->event_cfg[i].userhandle=NULL;
		
	if(psession->event_cfg[i].is_mechanism_en!=CALLBACKMECH)
		return MFI_SUCCESS;

  //����ǻص����ƣ���ʧ�ܸ��¼�
	for(i=0;i<RsrcManager->event_amount;i++){
		if(eventType!=RsrcManager->event_en[i].event_type)
			continue;
		
		RsrcManager->event_en[i].event_en &= (~(1<<Mfi));//ʧ���¼�������Դ�������е��¼�ʹ�ܹ��������Ӧ�¼���Ӧ�Ự��λ��0
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

//�ص����ƴ����߳�
void* thr_event_callback(void* arg)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventPointer temp=NULL;
	MfiSession Mfi;
	int i=0;
	
	//�Ӳ�����ȡ���ỰID��������
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
		//����
		if(psession->pthread_id.callback_thr == -1){
			pthread_mutex_unlock(&(psession->callback_mech.lock));
			break;
		}
//		printf("pthread_deal:1!!!!!\n");
		//�ص�������һ�����¼���ȡ������
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
		
		//�ûỰ��֧�ֵ�ǰ�¼�����
		if(i>=psession->cfg_amount){
			free(temp);
			temp=NULL;
			continue;
		}
		
	//	printf("pthread_deal:3!!!!!\n");
		//�ø��¼��Ĵ��������Ѿ����ǻص�����
		//5.9:��������ʱ�������ýṹ����ҲҪ�Ӷ�д�����ص��̻߳��ѯ�ýṹ�����߳̿����޸ĸýṹ���¼������߳�Ҳ��Ҫ��ѯ�ýṹ�Ի�ȡ�¼���ʹ�ܻ���
		if(psession->event_cfg[i].is_mechanism_en!=CALLBACKMECH){
//			printf("pthread_deal:4!!!!!\n");
			EventClose(temp->event_info);
//			printf("pthread_deal:5!!!!!\n");
			free(temp);
			temp=NULL;
//			printf("pthread_deal:6!!!!!\n");
			continue;
		}
		
		//���ûص���������
		//5.9����������ʱ���ɶԷ���״̬�����ж�
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

//������ƴ����߳�
//���ڹ����߳����ҵ���Ҫ�������¼�ʱ���Ѿ����¼�ָ��ṹ���ӣ�
//����discard֮���ȡ����ɾ�������¼����ر��¼�����Ϣ�ṹ���Թ����߳��ڴ������¼�û��Ӱ��
void* thr_event_suspend(void* arg)
{
	MfiPSessionInfo psession=NULL;
	MfiPEventPointer temp=NULL,temp1=NULL;
	MfiSession Mfi;
	MfiEventType event_type=0;
	int i=0,j=0,len=0,temp_len=0;
	
	//�Ӳ�����ȡ���ỰID��������
	Mfi=*((MfiSession*)arg);
	psession=&(RsrcManager->session_list[Mfi]);
	
	#ifdef PTHREAD_TEST
	printf("thr_event_suspend start: %d~~~~\n",Mfi);
	#endif
//	pthread_cleanup_push(cleanup, &(psession->suspend_mech.lock)); 
		
	while(psession->pthread_id.suspend_thr != -1){
		//�ӻ����л������л�ȡ��������û������ʱ����
		MechChangeFifoRead(&(psession->mech_fifo),&event_type);
		//����
		if(psession->pthread_id.suspend_thr == -1) break;
		
		for(j=0;j<psession->cfg_amount;j++)
		{
			if(event_type==psession->event_cfg[j].event_type)
				break;
		}

		while(psession->pthread_id.suspend_thr != -1)
		{
			pthread_mutex_lock(&(psession->suspend_mech.lock));
			//len���ڼ�¼�ϴ�ִ�е��ô�ʱ�Ķ��г��ȣ�
			//�����г�����len��Ϊ1ʱ���������µ��¼����뵽�˹�������л��߹�����б����������´Ӷ�ͷ��ʼ��ѯ
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
			
			//��ǰ���������û����Ҫ�������¼�
			if(i>=temp_len){
				pthread_mutex_unlock(&(psession->suspend_mech.lock));
				i=0;
				len=0;
				temp_len=0;
				break;
			}
			
			//�¼�����
			temp->last->next=temp->next;
			temp->next->last=temp->last;
			temp1=temp;
			temp=temp->next;
			psession->suspend_mech.queue_len--;
			if(temp1==psession->suspend_mech.queue_tail)
				psession->suspend_mech.queue_tail=temp1->last;
			pthread_mutex_unlock(&(psession->suspend_mech.lock));
			
			//���¼��Ĵ��������Ѿ��Ļع�����ƣ���ɾ����ǰ�¼���Ϣ�ṹ��ʣ��ͬ���¼����ڹ��������
			if(psession->event_cfg[j].is_mechanism_en==SUSPENDMECH){
				i=0;
				len=0;
				temp_len=0;
				EventClose(temp1->event_info);
				free(temp1);
				break;
			}
			//���¼��Ĵ��������Ѿ���Ϊ���л��޻��ƣ���ɾ��������������и����¼�
			else if(psession->event_cfg[j].is_mechanism_en==QUEUEMECH || psession->event_cfg[j].is_mechanism_en==NOMECH)
			{
				EventClose(temp1->event_info);
				free(temp1);
				i++;
				continue;
			}
			
			//5.9����������ʱ���ɶԷ���״̬�����ж�
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

//�¼���Ϣ�ṹ��Ӻ���
//���ܣ�Ϊ�²������¼������¼�id�������¼���Ϣ�ṹ����id�Ĵ�С˳����뵽�¼���Ϣ�����ĺ���λ��
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

//�¼�ָ��ṹ��Ӻ���
//���ܣ�Ϊ�²������¼������¼�ָ��ṹ�����ݶ�Ӧ�Ự�Ը��¼������ã����¼�ָ��ṹ���뵽��Ӧ�Ĵ��������У�
//      ��Ϊ���л���ʱ�����жϵȴ��¼������Ƿ�Ϊ��ǰ�������¼���������������waitonEvent�ӿڵ��̣߳�
//      ��Ϊ�ص�����ʱ����Ӳ����ѻص��̣߳���Ϊ�������ʱ����ӡ�
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
	
	//�жϻỰ�Ƿ�֧�ָ��¼�
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

//�ú����ڹرջỰʱ���ڹرջص��߳�
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

//�ú����ڹرջỰʱ���ڹرչ����߳�
MfiStatus ForCloseSuspendThr(MfiSession Mfi)
{
		MfiPSessionInfo psession=NULL;
		MfiPEventPointer eventp=NULL;
		
		psession=&(RsrcManager->session_list[Mfi]);
		
		MechChangeFifoWrite(&(psession->mech_fifo),0);
		
		return MFI_SUCCESS;
}