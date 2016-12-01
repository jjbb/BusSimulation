#include "mfi_rsrc_manager.h"
#include "mfi_attribute.h"
#include "mfi_operations.h"
#include "mfi_session.h"
#include "mfi_module_info.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <regex.h>

//��Դ�������Ĳ���������
//���ܣ���ʼ����Դ������ʱ������������Դ�����������еĲ���
MfiOperations RMOperations={
	RMFindRsrc,RMFindNext,NULL,RMOpen,RMClose,RsrcSetAttribute,RsrcGetAttribute,NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	};

//��Դ������ָ��
//���ܣ�ָ�򴴽�����Դ������������ȫ�ֻ�ȡ��Դ������
MfiPRsrcManager RsrcManager=NULL, RsrcManagerShadow=NULL;

//��Դ�����������ӿ�
//���ܣ���ʼ����Դ����������ʼ������ƽ̨����ʼ�������ײ�����
MfiStatus MfiOpenDefaultRM (MfiPSession Mfi)
{
	int i=0;
	MfiSession mfi=0;
	MfiStatus status;
	MfiPModuleRsrcNodeInfo* temp;
	
	if(RsrcManager!=NULL){
		*Mfi=RsrcManager->rmsession;
		return MFI_SUCCESS;
	}
	else if(RsrcManagerShadow!=NULL){
		if((status=RMOpen(RsrcManager->bus_rsrc->rsrc_name,&mfi))==MFI_SUCCESS){
			MfiEnableEvent(mfi, MFI_EVENT_EXCEPTION, MFI_QUEUE);
			RsrcManager=RsrcManagerShadow;
			*Mfi=RsrcManager->rmsession;
		}
		else
			status=MFI_ERROR_OPEN_RSRC;
		return status;
	}
	
	/*RsrcManager Init*/
	RsrcManager=(MfiPRsrcManager)malloc(sizeof(MfiRsrcManager));
	RsrcManagerShadow=RsrcManager;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_ALLOC;
	
	memset(RsrcManager,0,sizeof(MfiRsrcManager));
	memcpy(RsrcManager->rsrc_attr,RMAttr,sizeof(RMAttr));
	RsrcManager->attr_amount=RM_ATTR_NUM;
	AttrInit(RsrcManager->rsrc_attr, RM_ATTR_NUM);   //��ʼ������Ĭ��ֵ
	
	srand((unsigned)time(NULL));
	RsrcManager->rmsession=rand()%(RM_MAX_SESSION_ID-RM_MIN_SESSION_ID+1)+RM_MIN_SESSION_ID;         //256-356֮�����������Դ�������ĻỰID
	if((status=RM_Attr_Init())!=MFI_SUCCESS){
		goto error1;
	}
	RsrcManager->rsrc_opt=RMOperations;            //��ʼ������������
	
	//��ʼ���Ự����̬�����ʼ��/�Ựʹ��״̬��ʼ��
	for(i=0;i<SESSION_MAX_NUM-1;i++){
		RsrcManager->session_list[i].next=i+1;
		RsrcManager->session_list[i].session=i;
		SesStatusInit(&(RsrcManager->session_list[i]));
	}
	RsrcManager->session_list[i].next=0;	
	RsrcManager->session_list[i].session=i;
	SesStatusInit(&(RsrcManager->session_list[i]));
	
	//��ʼ��������Դ����
	MfiFindListQueueInit(&(RsrcManager->findlist_queue));
	//��ʼ���¼���Ϣ�ṹ����
	if(pthread_rwlock_init(&(RsrcManager->event_closing.queue_lock),MFI_NULL)!=0){
		status=MFI_ERROR_SYSTEM_ERROR;
		goto error2;
	}
	RsrcManager->event_closing.queue_head.event_id=EVENT_MIN_ID-1;
	RsrcManager->event_closing.insert_point=&(RsrcManager->event_closing.queue_head);
	RsrcManager->event_closing.queue_head.next=&(RsrcManager->event_closing.queue_head);
	RsrcManager->event_closing.queue_head.last=&(RsrcManager->event_closing.queue_head);
	RsrcManager->event_en=all_event_en;
	RsrcManager->event_amount=sizeof(all_event_en)/sizeof(MfiEventEnInfo);

	/*Bus rsrc Init*/
	//���߳�ʼ��
	if((status=Rsrc_Bus_Init(&(RsrcManager->bus_rsrc)))!=MFI_SUCCESS){
		goto error3;
	}
	
	/*open bus rsrc*/
	status=RMOpen(RsrcManager->bus_rsrc->rsrc_name,&mfi);
	if(status!=MFI_SUCCESS){
		status=MFI_ERROR_OPEN_RSRC;
		goto error4;
	}
		
	/*enable exception event*/
	MfiEnableEvent(mfi, MFI_EVENT_EXCEPTION, MFI_QUEUE);
	
	/*module rsrc init*/
	temp=&(RsrcManager->module_rsrc);
	for(i=0;i<Module.number;i++){
		status=Rsrc_Module_Init(temp, i);
		if(status!=MFI_SUCCESS)
			goto error5;
			temp=&((*temp)->next);
	}
	
	
	return MFI_SUCCESS;
	error5:
		while(RsrcManager->module_rsrc){
			temp=&(RsrcManager->module_rsrc);
			RsrcManager->module_rsrc=RsrcManager->module_rsrc->next;
			Rsrc_Module_Free(&(RsrcManager->module_rsrc));
		}
	error4:
		Rsrc_Bus_Free(&(RsrcManager->bus_rsrc));
	error3:
		pthread_rwlock_destroy(&(RsrcManager->event_closing.queue_lock));
	error2:
		for(i=0;i<SESSION_MAX_NUM;i++)
			SesStatusClear(&(RsrcManager->session_list[i]));
	error1:
		free(RsrcManager);		
		RsrcManager=NULL;
		RsrcManagerShadow=NULL;
		return status;
}

//�ر���Դ����������
MfiStatus RMClose(MfiObject Mfi)
{
	int i=0;
	MfiPModuleRsrcNodeInfo temp;
	
	//RsrcManager->rmsession=-1;  //9.14�޸ģ�ע��
	//AttrFree(RsrcManager->rsrc_attr, RsrcManager->attr_amount); //�ͷ��ַ����ԵĿռ�  9.19��ע��
	//AttrInit(RsrcManager->rsrc_attr, RM_ATTR_NUM);   //9.14�޸���������ʼ������Ĭ��ֵ  9.19��ע��
	
	//�ر����лỰ
	for(i=1;i<SESSION_MAX_NUM;i++)
		if(SesStatusCheck(&(RsrcManager->session_list[i]))==INUSE){
			SesStatusFree(&(RsrcManager->session_list[i]));
			RsrcSessionClose(i);
		}
		
	//ɾ��������Դ
	Rsrc_Bus_Delete(&(RsrcManager->bus_rsrc));
	
	//ɾ��ģ����Դ
	//9.14�޸�
	/*
	while(NULL!=(temp=RsrcManager->module_rsrc)){
		RsrcManager->module_rsrc=temp->next;
		Rsrc_Module_Delete(&temp);
	}
	*/
	while(NULL!=temp){
		Rsrc_Module_Delete(&temp);
		temp=temp->next;
	}
	
	//�رղ�����Դ����
	MfiFindListQueueDelete(&(RsrcManager->findlist_queue));
	
	//�ر��¼���Ϣ�ṹ(�Ự�ر�ʱ����һ�����������ǻ��кܾ�֮ǰδclose���¼���Ϣ��������)
	EventQueueDelete();
	
	
	//9.14�޸ģ�ע��
	//free(RsrcManager);
	RsrcManager=NULL;
	
	//����¼�ʹ�ܽṹ
	//�����ڹرջỰʱ����������лỰ���¼�ʹ��������˴����账��
	
	return MFI_SUCCESS;
}

//����Դ����
//���ܣ�����Դ�ĻỰ
//���øú���ʱ���Ѿ��ԻỰ����Ч�Լ��Ự�Ƿ�֧�ָò��������ж�
MfiStatus RMOpen(MfiRsrc name, MfiPSession Mfi)
{
	MfiPModuleRsrcNodeInfo node;
	MfiInt32 temp=0;
	MfiStatus status;
	pthread_t pid;
	MfiPSessionInfo psession=NULL;
	
	//��ģ����Դ����������ȶ���Դ�ַ������ҵ�Ҫ�򿪵�ģ��
	for(node=RsrcManager->module_rsrc;node!=NULL;node=node->next)
	{
		if(0==strcmp(node->rsrc_name,name)){
			if(node->session!=-1){
				*Mfi=node->session;    //�ѷ���Ự��ֱ�ӷ���
				return MFI_SUCCESS;
			}
			else{
				//��ǰ��Դδ����Ự
				if(RsrcManager->session_list[0].next==0)
					return MFI_ERROR_SESN_NUM;  //ϵͳ���Ѿ��ﵽ������ƵĻỰ����
				
				/*Init new session*/
				psession=&(RsrcManager->session_list[RsrcManager->session_list[0].next]);
				
				/*find the msg_rfifo attr & init msgfifo*/
				if((temp=FindAttr(MFI_ATTR_MSG_RFIFO_LEN, node->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1) //�ô�ʹ��node����ȡ����
					return MFI_ERROR_NSUP_ATTR;
				if((status=MfiMsgFifoInit(&(psession->msg_rfifo), temp))!=MFI_SUCCESS)
					return status;

				/*find the data_rfifo attr & init datafifo*/
				if((temp=FindAttr(MFI_ATTR_DATA_RFIFO_LEN, node->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					return MFI_ERROR_NSUP_ATTR;
				}
				if((status=MfiDataFifoInit(&(psession->data_rfifo), temp))!=MFI_SUCCESS){
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					return status;
				}
				
				/*init attrbuf,�������Ի���*/
				if((status=MfiAttrGetBufInit(&(psession->attr_get_buf)))!=MFI_SUCCESS){
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					MfiDataFifoDelete(&(psession->data_rfifo));
					return status;
				}
				
				/*init event cfg info*/
				psession->event_cfg=(MfiPEventCfgInfo)malloc(MODULE_RSRC_EVENT_NUM*sizeof(MfiEventCfgInfo));
	
				if(psession->event_cfg==NULL){
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					MfiDataFifoDelete(&(psession->data_rfifo));
					MfiAttrGetBufDelete(&(psession->attr_get_buf));
					return MFI_ERROR_ALLOC;   
				}
				memcpy(psession->event_cfg,ModuleRsrcEvent,sizeof(ModuleRsrcEvent));
				psession->cfg_amount=MODULE_RSRC_EVENT_NUM;
				psession->event_mech_en=NOMECH;
				
				/*init event queue*/
				if((status=EventPointerQueueInit(&(psession->queue_mech)))!=MFI_SUCCESS){
					free(psession->event_cfg);
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					MfiDataFifoDelete(&(psession->data_rfifo));
					MfiAttrGetBufDelete(&(psession->attr_get_buf));
					return status;   					
				}
				
				if((status=EventPointerQueueInit(&(psession->callback_mech)))!=MFI_SUCCESS){
					EventPointerQueueDelete(&(psession->queue_mech)); 
					free(psession->event_cfg);
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					MfiDataFifoDelete(&(psession->data_rfifo));
					MfiAttrGetBufDelete(&(psession->attr_get_buf));
					return status;   					
				}
				
				if((status=EventPointerQueueInit(&(psession->suspend_mech)))!=MFI_SUCCESS){
					EventPointerQueueDelete(&(psession->callback_mech)); 
					EventPointerQueueDelete(&(psession->queue_mech)); 
					free(psession->event_cfg);
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					MfiDataFifoDelete(&(psession->data_rfifo));
					MfiAttrGetBufDelete(&(psession->attr_get_buf));
					return status;
				}
				/*���ڻ����л����������*/ //�����л����еĳ�ʼ���ƶ����¼�ʹ�ܽӿ���ִ��
	/*			if((status=MfiMechChangeFifoInit(&(psession->mech_fifo)))!=MFI_SUCCESS){
					EventPointerQueueDelete(&(psession->suspend_mech)); 
					EventPointerQueueDelete(&(psession->callback_mech)); 
					EventPointerQueueDelete(&(psession->queue_mech)); 
					free(psession->event_cfg);
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					MfiDataFifoDelete(&(psession->data_rfifo));
					MfiAttrGetBufDelete(&(psession->attr_get_buf));
					return status;										
				}
				*/
				/*init async io queue*/
				if((status=AsyncQueueInit(&(psession->rd_queue)))!=MFI_SUCCESS){
//					MfiMechChangeFifoDelete(&(psession->mech_fifo));
					EventPointerQueueDelete(&(psession->suspend_mech)); 
					EventPointerQueueDelete(&(psession->callback_mech)); 
					EventPointerQueueDelete(&(psession->queue_mech)); 
					free(psession->event_cfg);
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					MfiDataFifoDelete(&(psession->data_rfifo));
					MfiAttrGetBufDelete(&(psession->attr_get_buf));
					return status;					
				}
				
				if((status=AsyncQueueInit(&(psession->wt_queue)))!=MFI_SUCCESS){
					AsyncQueueDelete(&(psession->rd_queue));
	//				MfiMechChangeFifoDelete(&(psession->mech_fifo));
					EventPointerQueueDelete(&(psession->suspend_mech)); 
					EventPointerQueueDelete(&(psession->callback_mech)); 
					EventPointerQueueDelete(&(psession->queue_mech)); 
					free(psession->event_cfg);
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					MfiDataFifoDelete(&(psession->data_rfifo));
					MfiAttrGetBufDelete(&(psession->attr_get_buf));
					return status;					
				}
				
				*Mfi=node->session=RsrcManager->session_list[0].next;  //������Դ�ĻỰID
				RsrcManager->session_list[0].next=RsrcManager->session_list[node->session].next;

				SesStatusSetUSE(psession);
				psession->rsrc=node;
				psession->rsrc_type=MODULE_RSRC_TYPE;
				psession->rsrc_attr=node->rsrc_attr;
				psession->attr_amount=node->attr_amount;
				psession->rsrc_opt=node->rsrc_opt;
				psession->new_jobid=0;
				
				//�����첽�����߳�
				if(0!=pthread_create(&pid,NULL,thr_asyncread,&(RsrcManager->session_list[*Mfi].session)))   
				{		
					return MFI_ERROR_ASYNC_THR_FAIL;
				}
				else
				{
					psession->pthread_id.asyncread_thr=pid;
				}
				
				//�����첽�����߳�
				if(0!=pthread_create(&pid,NULL,thr_asyncwrite,&(RsrcManager->session_list[*Mfi].session)))   
				{		
					return MFI_ERROR_ASYNC_THR_FAIL;
				}
				else
				{
					psession->pthread_id.asyncwrite_thr=pid;
				}

				return MFI_SUCCESS;
			}
		}
			
	}
	
	if(0==strcmp(RsrcManager->bus_rsrc->rsrc_name,name)){
		if(RsrcManager->bus_rsrc->session!=-1){
			*Mfi=RsrcManager->bus_rsrc->session;    //�ѷ���Ự��ֱ�ӷ���
			return MFI_SUCCESS;
		}
		else{
				if(RsrcManager->session_list[0].next==0)
					return MFI_ERROR_SESN_NUM;
				
				/*Init new session*/
				psession=&(RsrcManager->session_list[RsrcManager->session_list[0].next]);
				//����fifo������ԣ���ʼ����fifo
				if((temp=FindAttr(MFI_ATTR_MSG_RFIFO_LEN, RsrcManager->bus_rsrc->rsrc_attr, BUS_RSRC_ATTR_NUM))==-1)
					return MFI_ERROR_NSUP_ATTR;
				if((status=MfiMsgFifoInit(&(psession->msg_rfifo), temp))!=MFI_SUCCESS)
					return status;

				if((temp=FindAttr(MFI_ATTR_DATA_RFIFO_LEN, RsrcManager->bus_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					return MFI_ERROR_NSUP_ATTR;
				}
				if((status=MfiDataFifoInit(&(psession->data_rfifo), temp))!=MFI_SUCCESS){
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					return status;
				}
				
				if((status=MfiAttrGetBufInit(&(psession->attr_get_buf)))!=MFI_SUCCESS){
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					MfiDataFifoDelete(&(psession->data_rfifo));
					return status;
				}
				
				//��ʼ���¼����ýṹ
				psession->event_cfg=(MfiPEventCfgInfo)malloc(BUS_RSRC_EVENT_NUM*sizeof(MfiEventCfgInfo));
	
				if(psession->event_cfg==NULL){
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					MfiDataFifoDelete(&(psession->data_rfifo));
					MfiAttrGetBufDelete(&(psession->attr_get_buf));
					return MFI_ERROR_ALLOC;   
				}
				memcpy(psession->event_cfg,BusRsrcEvent,sizeof(BusRsrcEvent)); 
				psession->cfg_amount=BUS_RSRC_EVENT_NUM;
				psession->event_mech_en=NOMECH;
				
				if((status=EventPointerQueueInit(&(psession->queue_mech)))!=MFI_SUCCESS){
					free(psession->event_cfg);
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					MfiDataFifoDelete(&(psession->data_rfifo));
					MfiAttrGetBufDelete(&(psession->attr_get_buf));
					return status;   					
				}
				
				if((status=EventPointerQueueInit(&(psession->callback_mech)))!=MFI_SUCCESS){
					EventPointerQueueDelete(&(psession->queue_mech)); 
					free(psession->event_cfg);
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					MfiDataFifoDelete(&(psession->data_rfifo));
					MfiAttrGetBufDelete(&(psession->attr_get_buf));
					return status;   					
				}
				
				if((status=EventPointerQueueInit(&(psession->suspend_mech)))!=MFI_SUCCESS){
					EventPointerQueueDelete(&(psession->callback_mech)); 
					EventPointerQueueDelete(&(psession->queue_mech)); 
					free(psession->event_cfg);
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					MfiDataFifoDelete(&(psession->data_rfifo));
					MfiAttrGetBufDelete(&(psession->attr_get_buf));
					return status;   					
				}
/*
				if((status=MfiMechChangeFifoInit(&(psession->mech_fifo)))!=MFI_SUCCESS){
					EventPointerQueueDelete(&(psession->suspend_mech)); 
					EventPointerQueueDelete(&(psession->callback_mech)); 
					EventPointerQueueDelete(&(psession->queue_mech)); 
					free(psession->MfiPEventCfgInfo);
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					MfiDataFifoDelete(&(psession->data_rfifo));
					MfiAttrGetBufDelete(&(psession->attr_get_buf));
					return status;										
				}
	*/			
				if((status=AsyncQueueInit(&(psession->rd_queue)))!=MFI_SUCCESS){
	//				MfiMechChangeFifoDelete(&(psession->mech_fifo));
					EventPointerQueueDelete(&(psession->suspend_mech)); 
					EventPointerQueueDelete(&(psession->callback_mech)); 
					EventPointerQueueDelete(&(psession->queue_mech)); 
					free(psession->event_cfg);
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					MfiDataFifoDelete(&(psession->data_rfifo));
					MfiAttrGetBufDelete(&(psession->attr_get_buf));
					return status;					
				}
				
				if((status=AsyncQueueInit(&(psession->wt_queue)))!=MFI_SUCCESS){
					AsyncQueueDelete(&(psession->rd_queue));
		//			MfiMechChangeFifoDelete(&(psession->mech_fifo));
					EventPointerQueueDelete(&(psession->suspend_mech)); 
					EventPointerQueueDelete(&(psession->callback_mech)); 
					EventPointerQueueDelete(&(psession->queue_mech)); 
					free(psession->event_cfg);
					MfiMsgFifoDelete(&(psession->msg_rfifo));
					MfiDataFifoDelete(&(psession->data_rfifo));
					MfiAttrGetBufDelete(&(psession->attr_get_buf));
					return status;					
				}
				
				*Mfi=RsrcManager->bus_rsrc->session=RsrcManager->session_list[0].next;
				RsrcManager->session_list[0].next=RsrcManager->session_list[RsrcManager->bus_rsrc->session].next;

				SesStatusSetUSE(psession);
				psession->rsrc=RsrcManager->bus_rsrc;
				psession->rsrc_type=BUS_RSRC_TYPE;
				psession->rsrc_attr=RsrcManager->bus_rsrc->rsrc_attr;
				psession->attr_amount=RsrcManager->bus_rsrc->attr_amount;
				psession->rsrc_opt=RsrcManager->bus_rsrc->rsrc_opt;
				
				//�����첽�����߳�
				if(0!=pthread_create(&pid,NULL,thr_asyncread,&(RsrcManager->session_list[*Mfi].session)))   
				{		
					return MFI_ERROR_ASYNC_THR_FAIL;
				}
				else
				{
					psession->pthread_id.asyncread_thr=pid;
				}
				
				//�����첽�����߳�
				if(0!=pthread_create(&pid,NULL,thr_asyncwrite,&(RsrcManager->session_list[*Mfi].session)))   
				{		
					return MFI_ERROR_ASYNC_THR_FAIL;
				}
				else
				{
					psession->pthread_id.asyncwrite_thr=pid;
				}
				
				return MFI_SUCCESS;
		}
	}
	
	return MFI_ERROR_RSRC_NFOUND;   //δ�ҵ���Դ
}

MfiStatus RM_Attr_Init(void)
{
	MfiStatus status=MFI_SUCCESS;
	MfiString tmp=NULL;
	int i=0;
		
	if((i=FindAttr(MFI_ATTR_RM_SESSION, RsrcManager->rsrc_attr, RM_ATTR_NUM))==-1)
		return MFI_ERROR_NSUP_ATTR;
	RsrcManager->rsrc_attr[i].attr_val.attr_ui=RsrcManager->rmsession; //������Դ�������ĻỰID����

	if((i=FindAttr(MFI_ATTR_RSRC_INFO, RsrcManager->rsrc_attr, RM_ATTR_NUM))==-1)
		return MFI_ERROR_NSUP_ATTR;
	RsrcManager->rsrc_attr[i].attr_val.attr_s=((MfiString)Module.Module_Info_p)-2; //����ģ����Դ��Ϣ -2Ϊ���������ֽڵİ�����Ϣ
	
	if((i=FindAttr(MFI_ATTR_RSRC_NUM, RsrcManager->rsrc_attr, RM_ATTR_NUM))==-1)
		return MFI_ERROR_NSUP_ATTR;
	RsrcManager->rsrc_attr[i].attr_val.attr_ui=Module.number; //����ģ����Դ����
	
	return MFI_SUCCESS;
}

MfiStatus MfiFindListQueueInit(MfiRsrcFindListQueue* queue)
{
	queue->queue_len=0;
	queue->min_id=FINDLIST_MIN_ID;
	queue->queue_head.next=&(queue->queue_head);
	queue->queue_head.last=&(queue->queue_head);
//	queue->queue_tail=&(queue->queue_head);
	
	return MFI_SUCCESS;
}

MfiStatus MfiFindListQueueDelete(MfiRsrcFindListQueue* queue)
{
	int i=0;
	MfiPRsrcFindListNode temp;
	
	for(;i<queue->queue_len;++i){
		temp=queue->queue_head.next;
		queue->queue_head.next=temp->next;
		free(temp);
	}
	queue->queue_len=0;
	queue->min_id=FINDLIST_MIN_ID;
	queue->queue_head.next=&(queue->queue_head);
	queue->queue_head.last=&(queue->queue_head);
//	queue->queue_tail=&(queue->queue_head);
	
	return MFI_SUCCESS;
}

MfiStatus RMFindRsrc(MfiString expr, MfiPFindList Mfi, MfiPUInt32 retCnt, MfiChar desc[])
{
	MfiPModuleRsrcNodeInfo node;
	MfiPRsrcFindListNode list=NULL;
	int cflags=REG_EXTENDED;  //ƥ����ʽ��α�����
	regmatch_t pmatch;
	MfiUInt32 nmatch=1;
	regex_t reg;
	int status,flag=0;
	
	regcomp(&reg,expr,cflags); //����������ʽ,���reg
	
	for(node=RsrcManager->module_rsrc;node!=NULL;node=node->next){
		status=regexec(&reg,node->rsrc_name,nmatch,&pmatch,0);  //����ƥ�� 
		if(status!=REG_NOMATCH){
			if(flag==0){
				list=calloc(1,sizeof(MfiRsrcFindListNode));
				if(list==NULL)
					return MFI_ERROR_ALLOC;
				list->findlist_id=RsrcManager->findlist_queue.min_id;
				RsrcManager->findlist_queue.min_id>=FINDLIST_MAX_ID ? (RsrcManager->findlist_queue.min_id=FINDLIST_MIN_ID) : (++RsrcManager->findlist_queue.min_id);
				list->rsrc_amount=0;
				list->rsrc_index=0;
				list->last=&(RsrcManager->findlist_queue.queue_head);
				list->next=RsrcManager->findlist_queue.queue_head.next;
				list->last->next=list;
				list->next->last=list;
				RsrcManager->findlist_queue.queue_len++;
				flag=1;
			}
			list->rsrc_name[list->rsrc_amount++]=node->rsrc_name;
		}
	}
	
	status=regexec(&reg,RsrcManager->bus_rsrc->rsrc_name,nmatch,&pmatch,0);  //����ƥ�� 
	if(status!=REG_NOMATCH){
		if(flag==0){
			list=calloc(1,sizeof(MfiRsrcFindListNode));
			if(list==NULL)
				return MFI_ERROR_ALLOC;
			list->findlist_id=RsrcManager->findlist_queue.min_id;
			RsrcManager->findlist_queue.min_id>=FINDLIST_MAX_ID ? (RsrcManager->findlist_queue.min_id=FINDLIST_MIN_ID) : (++RsrcManager->findlist_queue.min_id);
			list->rsrc_amount=0;
			list->rsrc_index=1;
			list->last=&(RsrcManager->findlist_queue.queue_head);
			list->next=RsrcManager->findlist_queue.queue_head.next;
			list->last->next=list;
			list->next->last=list;
			RsrcManager->findlist_queue.queue_len++;
			flag=1;
		}
		list->rsrc_name[list->rsrc_amount++]=RsrcManager->bus_rsrc->rsrc_name;
	}
	
	if(flag==0){
		*retCnt=0;
		return MFI_ERROR_RSRC_NFOUND;
	}
	
	*Mfi=list->findlist_id;
	*retCnt=list->rsrc_amount;
	strcpy(desc,list->rsrc_name[0]);
	return MFI_SUCCESS;
}

MfiStatus RMFindNext(MfiFindList Mfi, MfiChar desc[])
{
	MfiPRsrcFindListNode list=NULL;
	
	list=RsrcManager->findlist_queue.queue_head.next;
	while(list!=&(RsrcManager->findlist_queue.queue_head) && list->findlist_id!=Mfi)
		list=list->next;
	
	if(list->findlist_id==Mfi && list->rsrc_index<list->rsrc_amount){
		strcpy(desc,list->rsrc_name[list->rsrc_index++]);
		return MFI_SUCCESS;
	}
	else
		return MFI_ERROR_RSRC_NFOUND;
}

MfiStatus FindListClose(MfiFindList Mfi)
{
	MfiPRsrcFindListNode list=NULL;
	
	list=RsrcManager->findlist_queue.queue_head.next;
	while(list!=&(RsrcManager->findlist_queue.queue_head) && list->findlist_id!=Mfi)
		list=list->next;
	
	if(list->findlist_id==Mfi){
		list->last->next=list->next;
		list->next->last=list->last;
		free(list);
	}

	return MFI_SUCCESS;
}