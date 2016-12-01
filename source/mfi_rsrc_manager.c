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

//资源管理器的操作函数集
//功能：初始化资源管理器时，用于配置资源管理器所具有的操作
MfiOperations RMOperations={
	RMFindRsrc,RMFindNext,NULL,RMOpen,RMClose,RsrcSetAttribute,RsrcGetAttribute,NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	};

//资源管理器指针
//功能：指向创建的资源管理器，用于全局获取资源管理器
MfiPRsrcManager RsrcManager=NULL, RsrcManagerShadow=NULL;

//资源管理器开启接口
//功能：初始化资源管理器，初始化驱动平台，初始化仪器底层配置
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
	AttrInit(RsrcManager->rsrc_attr, RM_ATTR_NUM);   //初始化属性默认值
	
	srand((unsigned)time(NULL));
	RsrcManager->rmsession=rand()%(RM_MAX_SESSION_ID-RM_MIN_SESSION_ID+1)+RM_MIN_SESSION_ID;         //256-356之间随机产生资源管理器的会话ID
	if((status=RM_Attr_Init())!=MFI_SUCCESS){
		goto error1;
	}
	RsrcManager->rsrc_opt=RMOperations;            //初始化操作函数集
	
	//初始化会话：静态链表初始化/会话使用状态初始化
	for(i=0;i<SESSION_MAX_NUM-1;i++){
		RsrcManager->session_list[i].next=i+1;
		RsrcManager->session_list[i].session=i;
		SesStatusInit(&(RsrcManager->session_list[i]));
	}
	RsrcManager->session_list[i].next=0;	
	RsrcManager->session_list[i].session=i;
	SesStatusInit(&(RsrcManager->session_list[i]));
	
	//初始化查找资源链表
	MfiFindListQueueInit(&(RsrcManager->findlist_queue));
	//初始化事件信息结构队列
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
	//总线初始化
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

//关闭资源管理器函数
MfiStatus RMClose(MfiObject Mfi)
{
	int i=0;
	MfiPModuleRsrcNodeInfo temp;
	
	//RsrcManager->rmsession=-1;  //9.14修改：注释
	//AttrFree(RsrcManager->rsrc_attr, RsrcManager->attr_amount); //释放字符属性的空间  9.19：注释
	//AttrInit(RsrcManager->rsrc_attr, RM_ATTR_NUM);   //9.14修改新增：初始化属性默认值  9.19：注释
	
	//关闭所有会话
	for(i=1;i<SESSION_MAX_NUM;i++)
		if(SesStatusCheck(&(RsrcManager->session_list[i]))==INUSE){
			SesStatusFree(&(RsrcManager->session_list[i]));
			RsrcSessionClose(i);
		}
		
	//删除总线资源
	Rsrc_Bus_Delete(&(RsrcManager->bus_rsrc));
	
	//删除模块资源
	//9.14修改
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
	
	//关闭查找资源链表
	MfiFindListQueueDelete(&(RsrcManager->findlist_queue));
	
	//关闭事件信息结构(会话关闭时做了一部分清理，但是会有很久之前未close的事件信息残留下来)
	EventQueueDelete();
	
	
	//9.14修改：注释
	//free(RsrcManager);
	RsrcManager=NULL;
	
	//清除事件使能结构
	//由于在关闭会话时，清楚了所有会话的事件使能情况，此处无需处理
	
	return MFI_SUCCESS;
}

//打开资源函数
//功能：打开资源的会话
//调用该函数时，已经对会话的有效性及会话是否支持该操作做了判断
MfiStatus RMOpen(MfiRsrc name, MfiPSession Mfi)
{
	MfiPModuleRsrcNodeInfo node;
	MfiInt32 temp=0;
	MfiStatus status;
	pthread_t pid;
	MfiPSessionInfo psession=NULL;
	
	//在模块资源链表中逐个比对资源字符串，找到要打开的模块
	for(node=RsrcManager->module_rsrc;node!=NULL;node=node->next)
	{
		if(0==strcmp(node->rsrc_name,name)){
			if(node->session!=-1){
				*Mfi=node->session;    //已分配会话，直接返回
				return MFI_SUCCESS;
			}
			else{
				//当前资源未分配会话
				if(RsrcManager->session_list[0].next==0)
					return MFI_ERROR_SESN_NUM;  //系统内已经达到最大限制的会话数量
				
				/*Init new session*/
				psession=&(RsrcManager->session_list[RsrcManager->session_list[0].next]);
				
				/*find the msg_rfifo attr & init msgfifo*/
				if((temp=FindAttr(MFI_ATTR_MSG_RFIFO_LEN, node->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1) //该处使用node来获取属性
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
				
				/*init attrbuf,用于属性机制*/
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
				/*用于机制切换的任务队列*/ //机制切换队列的初始化移动到事件使能接口中执行
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
				
				*Mfi=node->session=RsrcManager->session_list[0].next;  //设置资源的会话ID
				RsrcManager->session_list[0].next=RsrcManager->session_list[node->session].next;

				SesStatusSetUSE(psession);
				psession->rsrc=node;
				psession->rsrc_type=MODULE_RSRC_TYPE;
				psession->rsrc_attr=node->rsrc_attr;
				psession->attr_amount=node->attr_amount;
				psession->rsrc_opt=node->rsrc_opt;
				psession->new_jobid=0;
				
				//创建异步处理线程
				if(0!=pthread_create(&pid,NULL,thr_asyncread,&(RsrcManager->session_list[*Mfi].session)))   
				{		
					return MFI_ERROR_ASYNC_THR_FAIL;
				}
				else
				{
					psession->pthread_id.asyncread_thr=pid;
				}
				
				//创建异步处理线程
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
			*Mfi=RsrcManager->bus_rsrc->session;    //已分配会话，直接返回
			return MFI_SUCCESS;
		}
		else{
				if(RsrcManager->session_list[0].next==0)
					return MFI_ERROR_SESN_NUM;
				
				/*Init new session*/
				psession=&(RsrcManager->session_list[RsrcManager->session_list[0].next]);
				//查找fifo深度属性，初始化读fifo
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
				
				//初始化事件设置结构
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
				
				//创建异步处理线程
				if(0!=pthread_create(&pid,NULL,thr_asyncread,&(RsrcManager->session_list[*Mfi].session)))   
				{		
					return MFI_ERROR_ASYNC_THR_FAIL;
				}
				else
				{
					psession->pthread_id.asyncread_thr=pid;
				}
				
				//创建异步处理线程
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
	
	return MFI_ERROR_RSRC_NFOUND;   //未找到资源
}

MfiStatus RM_Attr_Init(void)
{
	MfiStatus status=MFI_SUCCESS;
	MfiString tmp=NULL;
	int i=0;
		
	if((i=FindAttr(MFI_ATTR_RM_SESSION, RsrcManager->rsrc_attr, RM_ATTR_NUM))==-1)
		return MFI_ERROR_NSUP_ATTR;
	RsrcManager->rsrc_attr[i].attr_val.attr_ui=RsrcManager->rmsession; //设置资源管理器的会话ID属性

	if((i=FindAttr(MFI_ATTR_RSRC_INFO, RsrcManager->rsrc_attr, RM_ATTR_NUM))==-1)
		return MFI_ERROR_NSUP_ATTR;
	RsrcManager->rsrc_attr[i].attr_val.attr_s=((MfiString)Module.Module_Info_p)-2; //设置模块资源信息 -2为纳入两个字节的板数信息
	
	if((i=FindAttr(MFI_ATTR_RSRC_NUM, RsrcManager->rsrc_attr, RM_ATTR_NUM))==-1)
		return MFI_ERROR_NSUP_ATTR;
	RsrcManager->rsrc_attr[i].attr_val.attr_ui=Module.number; //设置模块资源数量
	
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
	int cflags=REG_EXTENDED;  //匹配表达式如何被处理
	regmatch_t pmatch;
	MfiUInt32 nmatch=1;
	regex_t reg;
	int status,flag=0;
	
	regcomp(&reg,expr,cflags); //编译正则表达式,获得reg
	
	for(node=RsrcManager->module_rsrc;node!=NULL;node=node->next){
		status=regexec(&reg,node->rsrc_name,nmatch,&pmatch,0);  //正则匹配 
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
	
	status=regexec(&reg,RsrcManager->bus_rsrc->rsrc_name,nmatch,&pmatch,0);  //正则匹配 
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