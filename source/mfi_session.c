#include "mfi_session.h"
#include "mfi_rsrc_manager.h"
#include <pthread.h>
#include <stdlib.h>

/*消息FIFO初始化函数，会话初始化时使用*/
MfiStatus MfiMsgFifoInit(MfiPMsgFifo pfifo, MfiUInt32 len)
{
	pfifo->buf = (MfiPMessage*)malloc(sizeof(MfiPMessage)*len);
	if(pfifo->buf==NULL)
		return MFI_ERROR_ALLOC;
	
	pfifo->empty  = MFI_TRUE;
	pfifo->full   = MFI_FALSE;
	pfifo->buf_len = len;
	pfifo->rindex = 0;
	pfifo->windex = 0;
	
	if(pthread_mutex_init(&(pfifo->lock),MFI_NULL)!=0){
		free(pfifo->buf);
		return MFI_ERROR_SYSTEM_ERROR;
	}
	
	if(pthread_cond_init(&(pfifo->ready),MFI_NULL)!=0){
		free(pfifo->buf);
		pthread_mutex_destroy(&(pfifo->lock));
		return MFI_ERROR_SYSTEM_ERROR;
	}
	
	return MFI_SUCCESS;
}

void MfiMsgFifoDelete(MfiPMsgFifo pfifo)
{
	MfiPMessage temp=NULL;
	//后续修改：获取锁，然后判断fifo中是否有消息，有则释放掉每一个消息，再释放buf，再设置rd/wt/empty/full
	pthread_mutex_lock(&(pfifo->lock));
	while(pfifo->empty==MFI_FALSE){
		temp=pfifo->buf[pfifo->rindex];
		MfiCombMsgFree((MfiByte*)(temp+1));
		
		if(pfifo->rindex < (pfifo->buf_len-1))
			pfifo->rindex++;
		else
			pfifo->rindex=0;
	
		if(pfifo->rindex == pfifo->windex)
			pfifo->empty=MFI_TRUE;
	}
	pthread_mutex_unlock(&(pfifo->lock));
	free(pfifo->buf);
	pthread_mutex_destroy(&(pfifo->lock));
	pthread_cond_destroy(&(pfifo->ready));
	
	return;
}

//#ifdef TEST_ASYNC
MfiUInt32 discardnum=0,keepnum=0;
//#endif
MfiStatus SesMsgFifoWrite(MfiPMsgFifo pfifo, MfiPMessage msg)
{
	pthread_mutex_lock(&(pfifo->lock));
	
	if(pfifo->full==MFI_TRUE){
		pthread_mutex_unlock(&(pfifo->lock));
		
		//#ifdef TEST_ASYNC
		discardnum++;
		//#endif
		
		return MFI_ERROR_BUF_FULL;
	}
	
	pfifo->buf[pfifo->windex]=msg;
	
	if (pfifo->windex < (pfifo->buf_len-1))
		pfifo->windex++;
	else
		pfifo->windex=0;
	
	if (pfifo->windex == pfifo->rindex)
		pfifo->full=MFI_TRUE;
	
	pfifo->empty=MFI_FALSE;

	keepnum++;
	pthread_cond_signal(&(pfifo->ready));
	pthread_mutex_unlock(&(pfifo->lock));
	
	return MFI_SUCCESS;
}

MfiStatus SesDataFifoWrite(MfiPDataFifo pfifo, MfiPdata data)
{
	pthread_mutex_lock(&(pfifo->lock));
	
	if(pfifo->full==MFI_TRUE){
		pthread_mutex_unlock(&(pfifo->lock));
		return MFI_ERROR_BUF_FULL;
	}
	
	pfifo->buf[pfifo->windex]=data;
	
	if (pfifo->windex < (pfifo->buf_len-1))
		pfifo->windex++;
	else
		pfifo->windex=0;
	
	if (pfifo->windex == pfifo->rindex)
		pfifo->full=MFI_TRUE;
	
	pfifo->empty=MFI_FALSE;

	pthread_cond_signal(&(pfifo->ready));
	pthread_mutex_unlock(&(pfifo->lock));
	
	return MFI_SUCCESS;
}

/*数据FIFO初始化函数*/
MfiStatus MfiDataFifoInit(MfiPDataFifo pfifo, MfiUInt32 len)
{
	pfifo->buf = (MfiPdata*)malloc(sizeof(MfiPdata)*len);
	if(pfifo->buf==NULL)
		return MFI_ERROR_ALLOC;
	
	pfifo->empty  = MFI_TRUE;
	pfifo->full   = MFI_FALSE;
	pfifo->buf_len = len;
	pfifo->rindex = 0;
	pfifo->windex = 0;
	
	if(pthread_mutex_init(&(pfifo->lock),MFI_NULL)!=0){
		free(pfifo->buf);
		return MFI_ERROR_SYSTEM_ERROR;
	}
	
	if(pthread_cond_init(&(pfifo->ready),MFI_NULL)!=0){
		free(pfifo->buf);
		pthread_mutex_destroy(&(pfifo->lock));
		return MFI_ERROR_SYSTEM_ERROR;
	}
	
	return MFI_SUCCESS;
}


void MfiDataFifoDelete(MfiPDataFifo pfifo)
{
	MfiPdata temp=NULL;
	//后续修改：获取锁，然后判断fifo中是否有消息，有则释放掉每一个消息，再释放buf，再设置rd/wt/empty/full
	pthread_mutex_lock(&(pfifo->lock));
	while(pfifo->empty==MFI_FALSE){
		temp=pfifo->buf[pfifo->rindex];
		MfiCombDataFree((MfiByte*)(temp+1));
		
		if(pfifo->rindex < (pfifo->buf_len-1))
			pfifo->rindex++;
		else
			pfifo->rindex=0;
	
		if(pfifo->rindex == pfifo->windex)
			pfifo->empty=MFI_TRUE;
	}
	pthread_mutex_unlock(&(pfifo->lock));
	free(pfifo->buf);
	pthread_mutex_destroy(&(pfifo->lock));
	pthread_cond_destroy(&(pfifo->ready));
	
	return;
}

MfiStatus MfiAttrGetBufInit(MfiPAttrGetBuf buf)
{
	if(pthread_mutex_init(&(buf->lock),MFI_NULL)!=0){
		return MFI_ERROR_SYSTEM_ERROR;
	}
	
	if(pthread_cond_init(&(buf->ready),MFI_NULL)!=0){
		pthread_mutex_destroy(&(buf->lock));
		return MFI_ERROR_SYSTEM_ERROR;
	}
	
	buf->buf=NULL;
	return MFI_SUCCESS;
}

void MfiAttrGetBufDelete(MfiPAttrGetBuf buf)
{
	pthread_mutex_lock(&(buf->lock));
	if(buf->buf!=NULL){
		MfiCombMsgFree((MfiByte*)(buf->buf+1));
		buf->buf=NULL;
	}
	pthread_mutex_unlock(&(buf->lock));
	pthread_mutex_destroy(&(buf->lock));
	pthread_cond_destroy(&(buf->ready));
	
	return;
}

//关闭会话函数
//功能：关闭已分配会话，释放通信资源等
MfiStatus RsrcSessionClose(MfiObject Mfi)
{
	MfiPSessionInfo psession=NULL;
	
	psession=&(RsrcManager->session_list[Mfi]);
	SesStatusSetNOUSE(psession);                  //其他所有接口以及底层数据分配线程、回调线程、挂起线程无法再对该会话操作
	if(psession->rsrc_type==MODULE_RSRC_TYPE)
		((MfiPModuleRsrcNodeInfo)psession->rsrc)->session=-1;
	else if(psession->rsrc_type==BUS_RSRC_TYPE)
		((MfiPBusRsrcNodeInfo)psession->rsrc)->session=-1;
	
	DisableEvent(Mfi, MFI_ALL_ENABLED_EVENTS);//关闭会话时，一定要失能所有事件，在资源管理器的事件使能结构中删除记录
	
	/*wait the four threads exit*/
	if(psession->pthread_id.asyncread_thr!=0){
		psession->pthread_id.asyncread_thr=-1;
		ForCloseAsyncRdThr(Mfi);
		pthread_join(psession->pthread_id.asyncread_thr,NULL); 
		psession->pthread_id.asyncread_thr=0;
	}
	if(psession->pthread_id.asyncwrite_thr!=0){
		psession->pthread_id.asyncwrite_thr=-1;
		ForCloseAsyncWtThr(Mfi);
		pthread_join(psession->pthread_id.asyncwrite_thr,NULL);
		psession->pthread_id.asyncwrite_thr=0;
	}
	if(psession->pthread_id.callback_thr!=0){
		psession->pthread_id.callback_thr=-1;
		ForCloseCallBackThr(Mfi);
		pthread_join(psession->pthread_id.callback_thr,NULL);
		psession->pthread_id.callback_thr=0;
	}
	if(psession->pthread_id.suspend_thr!=0){
		psession->pthread_id.suspend_thr=-1;
		ForCloseSuspendThr(Mfi);
		pthread_join(psession->pthread_id.suspend_thr,NULL);
		psession->pthread_id.suspend_thr=0;
	}
	
	/*delete the event pointer queue & delete the event info struct*/
	EventPointerQueueDelete(&(psession->queue_mech));    //删除3个事件队列中遗留的未处理事件
	EventPointerQueueDelete(&(psession->callback_mech));
	EventPointerQueueDelete(&(psession->suspend_mech));
	
	/*delete the async io task queue*/
	AsyncQueueDelete(&(psession->rd_queue));
	AsyncQueueDelete(&(psession->wt_queue));
	
	MfiMechChangeFifoDelete(&(psession->mech_fifo));
	
	/*delete the msg and data rfifo*/
	MfiMsgFifoDelete(&(psession->msg_rfifo));
	MfiDataFifoDelete(&(psession->data_rfifo));
	MfiAttrGetBufDelete(&(psession->attr_get_buf));
	
	free(psession->event_cfg);               //释放事件设置结构数组
	psession->cfg_amount=0;
	psession->event_mech_en=NOMECH;

	
	//销毁模块资源会话对应的资源结构中的子窗分配结构，并设置总线资源中的指针数组的对应值为空
	
	//清空模块资源会话对应的资源结构中的触发线分配结构信息，并懒惰销毁总线资源中的触发线信息结构
	if(psession->rsrc_type != BUS_RSRC_TYPE)
		DeleteTrigger(Mfi,MFI_TRIG_ALL);
	
	psession->rsrc=NULL;
	psession->rsrc_type=MFI_NULL;
	psession->rsrc_attr=NULL;
	psession->attr_amount=0;
//	psession->rsrc_opt=NULL;
	psession->new_jobid=0;
	psession->next=RsrcManager->session_list[0].next;  //将会话加入到待分配会话链表中
	RsrcManager->session_list[0].next=Mfi;
	
	return MFI_SUCCESS;
}

