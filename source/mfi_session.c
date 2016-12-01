#include "mfi_session.h"
#include "mfi_rsrc_manager.h"
#include <pthread.h>
#include <stdlib.h>

/*��ϢFIFO��ʼ���������Ự��ʼ��ʱʹ��*/
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
	//�����޸ģ���ȡ����Ȼ���ж�fifo���Ƿ�����Ϣ�������ͷŵ�ÿһ����Ϣ�����ͷ�buf��������rd/wt/empty/full
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

/*����FIFO��ʼ������*/
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
	//�����޸ģ���ȡ����Ȼ���ж�fifo���Ƿ�����Ϣ�������ͷŵ�ÿһ����Ϣ�����ͷ�buf��������rd/wt/empty/full
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

//�رջỰ����
//���ܣ��ر��ѷ���Ự���ͷ�ͨ����Դ��
MfiStatus RsrcSessionClose(MfiObject Mfi)
{
	MfiPSessionInfo psession=NULL;
	
	psession=&(RsrcManager->session_list[Mfi]);
	SesStatusSetNOUSE(psession);                  //�������нӿ��Լ��ײ����ݷ����̡߳��ص��̡߳������߳��޷��ٶԸûỰ����
	if(psession->rsrc_type==MODULE_RSRC_TYPE)
		((MfiPModuleRsrcNodeInfo)psession->rsrc)->session=-1;
	else if(psession->rsrc_type==BUS_RSRC_TYPE)
		((MfiPBusRsrcNodeInfo)psession->rsrc)->session=-1;
	
	DisableEvent(Mfi, MFI_ALL_ENABLED_EVENTS);//�رջỰʱ��һ��Ҫʧ�������¼�������Դ���������¼�ʹ�ܽṹ��ɾ����¼
	
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
	EventPointerQueueDelete(&(psession->queue_mech));    //ɾ��3���¼�������������δ�����¼�
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
	
	free(psession->event_cfg);               //�ͷ��¼����ýṹ����
	psession->cfg_amount=0;
	psession->event_mech_en=NOMECH;

	
	//����ģ����Դ�Ự��Ӧ����Դ�ṹ�е��Ӵ�����ṹ��������������Դ�е�ָ������Ķ�ӦֵΪ��
	
	//���ģ����Դ�Ự��Ӧ����Դ�ṹ�еĴ����߷���ṹ��Ϣ������������������Դ�еĴ�������Ϣ�ṹ
	if(psession->rsrc_type != BUS_RSRC_TYPE)
		DeleteTrigger(Mfi,MFI_TRIG_ALL);
	
	psession->rsrc=NULL;
	psession->rsrc_type=MFI_NULL;
	psession->rsrc_attr=NULL;
	psession->attr_amount=0;
//	psession->rsrc_opt=NULL;
	psession->new_jobid=0;
	psession->next=RsrcManager->session_list[0].next;  //���Ự���뵽������Ự������
	RsrcManager->session_list[0].next=Mfi;
	
	return MFI_SUCCESS;
}

