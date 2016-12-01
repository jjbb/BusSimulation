#include "mfi_data.h"
#include "mfiapi.h"
#include "mfi_pool.h"
#include "mfi_message.h"
#include "mfi_rsrc_manager.h"
#include "mfi_test_define.h"
#include "mfi_system_command.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

mem_pools DataSendPool;
mem_pools DataRecPool;

DATA_Fifo dataRxFifo;
DATA_Fifo dataTxFifo;
CombData_Fifo dataCombFifo;

/*����FIFO��ʼ������*/
MfiStatus DataFifo_Init(DATA_Fifo* Data_Fifo_p)
{
	memset(Data_Fifo_p,0,sizeof(DATA_Fifo));
	
	if(pthread_mutex_init(&(Data_Fifo_p->lock),MFI_NULL)!=0)
		return MFI_ERROR_SYSTEM_ERROR;
	
	if(pthread_cond_init(&(Data_Fifo_p->ready),MFI_NULL)!=0){
		pthread_mutex_destroy(&(Data_Fifo_p->lock));
		return MFI_ERROR_SYSTEM_ERROR;
	}
	
	Data_Fifo_p->head.next=&(Data_Fifo_p->head);
	Data_Fifo_p->head.last=&(Data_Fifo_p->head);
	Data_Fifo_p->tail=&(Data_Fifo_p->head);
		
	Data_Fifo_p->Data_Fifo_MaxLen  = DATA_FIFO_DEF_LEN;
	Data_Fifo_p->Tmp_Fifo_MaxLen   = DATA_TMP_FIFO_DEF_LEN;
	
	return MFI_SUCCESS;
}

MfiStatus DataFifo_Delete(DATA_Fifo* Data_Fifo_p)
{
	int i=0;
	pool_head_p pool=NULL;
	DATA_p data;
	
	pool_create(&DataSendPool, sizeof(DATA), &pool);
	pthread_mutex_lock(&(Data_Fifo_p->lock));
	for(;i<Data_Fifo_p->Data_Fifo_Len;++i){
		Data_Fifo_p->tail=Data_Fifo_p->tail->last;
		pool_free(pool, Data_Fifo_p->tail->next);
	}
	for(i=0;i<SESSION_MAX_NUM;i++)
	{
		if(Data_Fifo_p->tmpHead[i]!=NULL){
			while(Data_Fifo_p->tmpHead[i]!=Data_Fifo_p->tmpTail[i]){
				data=Data_Fifo_p->tmpHead[i];
				Data_Fifo_p->tmpHead[i]=data->next;
				pool_free(pool, data);
			}
			pool_free(pool, Data_Fifo_p->tmpHead[i]);
		}
	}
	
	pthread_cond_destroy(&(Data_Fifo_p->ready));
	pthread_mutex_destroy(&(Data_Fifo_p->lock));
	
	memset(Data_Fifo_p,0,sizeof(DATA_Fifo));
	
	return MFI_SUCCESS;
}

pthread_mutex_t DPFreamData_lock=PTHREAD_MUTEX_INITIALIZER;

//��֡����
//����֡ģ����øú��������ش�����ʱ�����ݷ����߳�ȡ���������жϵ����ش�����ʱ��
//���ݴ�fifoȡ�����ش����ݣ��������ش�λ
//ע�⣡�������ɿ����ڷ�֡�����жԺ���pool_create(&DataSendPool, sizeof(DATA), &pool)ֻ����һ�Σ���pool����Ϊ��̬����
MfiStatus DecmposeFreamData(MfiUInt32 dst_ip, DATA_Fifo* Data_Fifo_p, MfiUInt32 priorty, MfiUInt32 dataclass, MfiUInt32 datatype, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 flag)
{
	DID_BITS dataid={0};
	DATA_p data=NULL;
	pool_head_p pool=NULL;
	int i=1;
	MfiUInt32 real_len=0,fream_num=0;
	
	dataid.d_src_addr=HOST_IP;
	dataid.d_dst_addr=dst_ip;
	dataid.d_type=datatype;
	dataid.priority2=priorty>>2;
	dataid.priority1=priorty>>1;
	dataid.priority0=priorty;
	dataid.d_class3=dataclass>>3;
	dataid.d_class2=dataclass>>2;
	dataid.d_class1=dataclass>>1;
	dataid.d_class0=dataclass;
	
	//����<=DATA_MAX_LEN-4,��֡
	if(retCnt<=DATA_MAX_LEN-4){
		dataid.d_single=0;
		
		//�����ڴ�
		pool_create(&DataSendPool, sizeof(DATA), &pool);
		data=(DATA_p)pool_alloc(&DataSendPool, pool);
		//�����ݴ����¿ռ�
		data->d_id.bit=dataid;
		((MfiPUInt16)data->d_char)[0]=retCnt;
		if(flag==0){
			((MfiPUInt16)data->d_char)[1]=Data_Fifo_p->number[dst_ip]++;
			Data_Fifo_p->number[dst_ip]%=(Data_Fifo_p->Tmp_Fifo_MaxLen-1);
		}
		else if(flag==1)
			((MfiPUInt16)data->d_char)[1]=DATA_TMP_FIFO_DEF_LEN-1;  //�̶�ʱ�䴰���ݣ������
		else if(flag==2)
			((MfiPUInt16)data->d_char)[1]=DATA_TMP_FIFO_DEF_LEN; 
			
		memcpy(&(data->d_char[4]),buf,retCnt);
		
		pthread_mutex_lock(&(Data_Fifo_p->lock));
		while(Data_Fifo_p->Data_Fifo_Len>=Data_Fifo_p->Data_Fifo_MaxLen)
			pthread_cond_wait(&(Data_Fifo_p->ready),&(Data_Fifo_p->lock));
		//���
		data->next=Data_Fifo_p->tail->next;
		data->last=Data_Fifo_p->tail;
		data->next->last=data;
		data->last->next=data;
		Data_Fifo_p->tail=data;
		Data_Fifo_p->Data_Fifo_Len++;
		pthread_mutex_unlock(&(Data_Fifo_p->lock));
		
		return MFI_SUCCESS;
	}
	
	real_len=DATA_MAX_LEN-8;//��������֡ʱ��һ֡��ʵ�ʳ���
	fream_num=(retCnt+real_len-1)/real_len;
	
	i=1;
	dataid.d_single=1;
	pool_create(&DataSendPool, sizeof(DATA), &pool);
	while(retCnt>0){
		//�����ڴ�	
		data=(DATA_p)pool_alloc(&DataSendPool, pool);
		//�����ݴ����¿ռ�
		data->d_id.bit=dataid;
		((MfiPUInt16)data->d_char)[0]=(retCnt>real_len?real_len:retCnt);
			
		if(flag==0){
			((MfiPUInt16)data->d_char)[1]=Data_Fifo_p->number[dst_ip]++;
			Data_Fifo_p->number[dst_ip]%=(Data_Fifo_p->Tmp_Fifo_MaxLen-1);
		}
		else if(flag==1)
			((MfiPUInt16)data->d_char)[1]=DATA_TMP_FIFO_DEF_LEN-1;  //�̶�ʱ�䴰���ݣ������
		else if(flag==2)
			((MfiPUInt16)data->d_char)[1]=DATA_TMP_FIFO_DEF_LEN;  //�����
			
		((MfiPUInt16)data->d_char)[2]=i;
		((MfiPUInt16)data->d_char)[3]=fream_num;
		memcpy(&(data->d_char[8]),buf,((MfiPUInt16)data->d_char)[0]);
		buf+=((MfiPUInt16)data->d_char)[0];
		
		pthread_mutex_lock(&(Data_Fifo_p->lock));
		while(Data_Fifo_p->Data_Fifo_Len>=Data_Fifo_p->Data_Fifo_MaxLen)
			pthread_cond_wait(&(Data_Fifo_p->ready),&(Data_Fifo_p->lock));
		//���
		data->next=Data_Fifo_p->tail->next;
		data->last=Data_Fifo_p->tail;
		data->next->last=data;
		data->last->next=data;
		Data_Fifo_p->tail=data;
		Data_Fifo_p->Data_Fifo_Len++;
		pthread_mutex_unlock(&(Data_Fifo_p->lock));
		
		retCnt-=((MfiPUInt16)data->d_char)[0];
		++i;
	}
	
	return MFI_SUCCESS;
}

MfiStatus DataSendToBus(DATA_Fifo* Data_Fifo_p)
{
	MfiUInt32 dst_ip=0,num=0,endnum=0;;
	MfiUInt32 dataclass=0;
	DATA_p data,tmpdata;
	pool_head_p pool=NULL;
	
	#ifdef ALL_TEST
	printf("DataSendToBus!\n");
	#endif
	
	pthread_mutex_lock(&(Data_Fifo_p->lock));
	if(Data_Fifo_p->Data_Fifo_Len==0){
		pthread_mutex_unlock(&(Data_Fifo_p->lock));
		#ifdef ALL_TEST
		printf("DataSendToBus: fifo empty!\n");
		#endif
		return MFI_SUCCESS;
	}
	//���ݷ��Ͷ�����ȡ��һ֡����
	data=Data_Fifo_p->head.next;
	Data_Fifo_p->head.next=data->next;
	data->next->last=data->last;
	if(data==Data_Fifo_p->tail)
		Data_Fifo_p->tail=data->last;
	Data_Fifo_p->Data_Fifo_Len--;
	
	pthread_cond_signal(&(Data_Fifo_p->ready));
	pthread_mutex_unlock(&(Data_Fifo_p->lock));
	
	dst_ip=data->d_id.bit.d_dst_addr;
	if(((MfiPUInt16)data->d_char)[1] < DATA_TMP_FIFO_DEF_LEN-1){
		//������ţ��跢�͵�����������ݴ�fifo
		if(Data_Fifo_p->tmplen[dst_ip]>=Data_Fifo_p->Tmp_Fifo_MaxLen){
			//�ݴ�fifo��
			tmpdata=Data_Fifo_p->tmpHead[dst_ip];
			Data_Fifo_p->tmpHead[dst_ip]=tmpdata->next;
			Data_Fifo_p->tmpTail[dst_ip]->next=data;
			Data_Fifo_p->tmpTail[dst_ip]=data;
			data->next=Data_Fifo_p->tmpHead[dst_ip];
  	
			//�ͷ�tmpdata
			pool_create(&DataSendPool, sizeof(DATA), &pool);
			pool_free(pool, tmpdata);
		}
		else if(Data_Fifo_p->tmplen[dst_ip]>0){
			//fifo�������ҷ���
			Data_Fifo_p->tmpTail[dst_ip]->next=data;
			Data_Fifo_p->tmpTail[dst_ip]=data;
			data->next=Data_Fifo_p->tmpHead[dst_ip];
			Data_Fifo_p->tmplen[dst_ip]++;
		}
		else{
			//�ݴ�fifoΪ��
			Data_Fifo_p->tmpHead[dst_ip]=data;
			Data_Fifo_p->tmpTail[dst_ip]=data;
			data->next=Data_Fifo_p->tmpHead[dst_ip];
			Data_Fifo_p->tmplen[dst_ip]=1;
		}
		
	}
	else{
		//��������ݣ���������֡ģ�鷢�������ط�֪ͨ���̶�ʱ�䴰���ݣ��㲥����
		dataclass=data->d_id.bit.d_class0|data->d_id.bit.d_class1<<1|data->d_id.bit.d_class2<<2|data->d_id.bit.d_class3<<3;
		//�ж��Ƿ�Ϊ��֡ģ�鷢�������ط�֪ͨ ������ע�⣺������Ϊ��ȷ��class��type
		if(((MfiPUInt16)data->d_char)[1]==DATA_TMP_FIFO_DEF_LEN && dataclass==URGENT_ORDER_CLASS && data->d_id.bit.d_type==RETRANS_APPLY_IN)
		{
			//����Ϣ�ĵ�һ����Ϣλ�ô���˴��ط���ţ����´�������Ϣ�ı��
			num=((MfiPUInt16)data->d_char)[2];
			endnum=((MfiPUInt16)data->d_char)[3];
			//�ͷ�data
			pool_create(&DataSendPool, sizeof(DATA), &pool);
			pool_free(pool, data);
			//�ݴ�����в���num
			data=Data_Fifo_p->tmpHead[dst_ip];
			while(data!=Data_Fifo_p->tmpTail[dst_ip] && ((MfiPUInt16)data->d_char)[1]!=num){
				data=data->next;
			}
			//δ�ҵ������ط��ı�ŵ�����
			if(((MfiPUInt16)data->d_char)[1]!=num)
				return MFI_SUCCESS;
			
			while(num!=endnum){
				data->d_id.bit.d_retry=1; //����ط�
				num=(num+1)%(DATA_TMP_FIFO_DEF_LEN-1);
				
				//�������������ӿڣ������ݷ��͵�FPGA
				
				
				data=data->next;
			}
			
			return MFI_SUCCESS;
		}
		else if(((MfiPUInt16)data->d_char)[1]==DATA_TMP_FIFO_DEF_LEN-1){
			//�������������ӿڣ������ݷ��͵�FPGA�й̶�ʱ�䴰��ؼĴ���
		
			pool_create(&DataSendPool, sizeof(DATA), &pool);
			pool_free(pool, data);
			return MFI_SUCCESS;
		}

	}

	//�������������ӿڣ�����Ϣ���͵�FPGA
	
	return MFI_SUCCESS;
}

MfiStatus DataReadFromBus(DATA_Fifo* Data_Fifo_p)
{
	DATA_p data;
	pool_head_p pool=NULL;
	
	pthread_mutex_lock(&(Data_Fifo_p->lock));
	
	if(Data_Fifo_p->Data_Fifo_Len>=Data_Fifo_p->Data_Fifo_MaxLen){
		//����FIFO������ֱ�ӽ���������������
		pthread_mutex_unlock(&(Data_Fifo_p->lock));
		//�����ǣ�������֡����
		
		return MFI_ERROR_BUF_FULL;
	}
	pthread_mutex_unlock(&(Data_Fifo_p->lock));
	
	pool_create(&DataRecPool, sizeof(DATA), &pool);
	data=(DATA_p)pool_alloc(&DataRecPool, pool);
	
	//�������������ӿڣ���FPGA�������ݣ�����data
	//������ҪCRC��
	
	pthread_mutex_lock(&(Data_Fifo_p->lock));
	//���
	data->next=Data_Fifo_p->tail->next;
	data->last=Data_Fifo_p->tail;
	data->next->last=data;
	data->last->next=data;
	Data_Fifo_p->tail=data;
	Data_Fifo_p->Data_Fifo_Len++;
	pthread_mutex_unlock(&(Data_Fifo_p->lock));
	
	return MFI_SUCCESS;
}

/*������֡FIFO��ʼ������*/
MfiStatus CombData_Fifo_Init(CombData_Fifo_p pointer)
{
	memset(pointer,0,sizeof(CombData_Fifo));
	
	pointer->queue_len=0;
	pointer->head.next=NULL;
	
	return MFI_SUCCESS;
}

//��֡����
//���ܣ��ӽ������ݶ���ȡ�����ݣ���������֡�������䵽�����Ự���߶Դ�������Ӧ����
//      ������ʹ�õ��й����ݵ�֡ͷ����Ч���ȡ�֡λ����Ϣ���ο�����֡ͷ�����Լ����ݶ���
MfiStatus CombineFreamData(MfiSession Mfi, DATA_Fifo* Data_Fifo_p, CombData_Fifo* CombData_Fifo_p)
{
	MfiUInt32 src_ip=0,num=0,cnt=0;
	MfiUInt32 dataclass=0;
	DATA_p data;
	pool_head_p pool=NULL;
	CombData_Head_p combData=NULL,temp=&(CombData_Fifo_p->head);
	CombData_Head_p target=NULL,tgtmp=NULL;
	int i=0,flag=0;
	MfiByte* src_addr=NULL,freampos=0,freamnum=0;
	MfiSession des_mfi;
	MfiUInt16 buf[2];
	MfiStatus status;
	
	//ȡ��һ֡����
	pthread_mutex_lock(&(Data_Fifo_p->lock));
	if(Data_Fifo_p->Data_Fifo_Len==0){
		pthread_mutex_unlock(&(Data_Fifo_p->lock));
		return MFI_SUCCESS;
	}
	
	data=Data_Fifo_p->head.next;
	Data_Fifo_p->head.next=data->next;
	data->next->last=data->last;
	if(data==Data_Fifo_p->tail)
		Data_Fifo_p->tail=data->last;
	Data_Fifo_p->Data_Fifo_Len--;
	
	pthread_mutex_unlock(&(Data_Fifo_p->lock));
	
	//�жϱ���Ƿ���ȷ
	src_ip=data->d_id.bit.d_src_addr;
	num=((MfiPUInt16)data->d_char)[1];
	if(num<DATA_TMP_FIFO_DEF_LEN-1 && num!=Data_Fifo_p->number[src_ip]){
		if(data->d_id.bit.d_retry!=1){
			//���ݶ�ʧ�����������ط����ݣ��ط���������ΪData_Fifo_p->number[src_ip]��num֮��ı��
			//����Դ���������һ��ר�ŵĴ���ģ�飬����һ���߳��������У�����������������ģ�ͣ�
			//��֡ģ����������ж������ָ��ɴ�����ģ���������֡ģ�飬�����ط��������Ϣ
			buf[0]=Data_Fifo_p->number[src_ip];
			buf[0]=num;
			
			pthread_mutex_lock(&DPFreamData_lock);
			DecmposeFreamData(src_ip, &dataTxFifo, URGENT_ORDER_DEF_PORITY, URGENT_ORDER_CLASS, RETRANS_APPLY_OUT, (MfiPBuf)buf, 4,0);
			pthread_mutex_unlock(&DPFreamData_lock);//�ͷ���1
			
			Data_Fifo_p->number[src_ip]=num+1;	 //renew the num
			Data_Fifo_p->number[src_ip]%=DATA_TMP_FIFO_DEF_LEN-1;//�����DATA_TMP_FIFO_DEF_LEN-2
		}
	}
	else if(num<DATA_TMP_FIFO_DEF_LEN-1){
		Data_Fifo_p->number[src_ip]++;
		Data_Fifo_p->number[src_ip]%=DATA_TMP_FIFO_DEF_LEN-1;//�����DATA_TMP_FIFO_DEF_LEN-2
	}
	
	//��֡
	if(data->d_id.bit.d_single==0){
		dataclass=data->d_id.bit.d_class0|data->d_id.bit.d_class1<<1|data->d_id.bit.d_class2<<2|data->d_id.bit.d_class3<<3;
		//�ж��Ƿ�Ϊ���Ͷ˷����������ط���Ϣ������ע�⣺������Ϊ��ȷ��class��type
		if(dataclass==URGENT_ORDER_CLASS && data->d_id.bit.d_type==RETRANS_APPLY_OUT)
		{
			//����A���������ط��ڲ�����
			pthread_mutex_lock(&DPFreamData_lock);
			DecmposeFreamData(src_ip, &dataTxFifo, URGENT_ORDER_DEF_PORITY, URGENT_ORDER_CLASS, RETRANS_APPLY_IN, &(data->d_char[4]), 4,2);
			pthread_mutex_unlock(&DPFreamData_lock);//�ͷ���1
			
			pool_create(&DataRecPool, sizeof(DATA), &pool);
			pool_free(pool, data);
			
			return MFI_SUCCESS;
		}
		
		//���˴������䣺src_ipת�Ựid des_mfi
		Ip_to_Mfi(src_ip,&des_mfi);
		status=MFI_SUCCESS;
		
		//�жϻỰ�Ƿ��Ѿ��رգ��Ƿ�������ʹ���У�
		if((RsrcManager!=NULL)&&(SesStatusCheck(&(RsrcManager->session_list[des_mfi]))==INUSE)){
			
			//�����ڴ棬ת������
			pool_create(&DataRecPool, sizeof(CombData_Head)+((MfiPUInt16)data->d_char)[0], &pool);
			combData=(CombData_Head_p)pool_alloc(&DataRecPool, pool);
			memcpy(combData+1,data->d_char+4,((MfiPUInt16)data->d_char)[0]);
			combData->memsize=pool->size;
			combData->d_id.all=data->d_id.all;
			combData->len=((MfiPUInt16)data->d_char)[0];
			
			//�����ݴ���Ự������fifo���߸������ݴ���������ߵĻỰ
			//�������ݴ�����࣬Ӧ���������������fifo���������͹���������Դ�Ự����fifo
			if(dataclass==APP_DATA_CLASS){
				status=SesDataFifoWrite(&(RsrcManager->session_list[des_mfi].data_rfifo), combData);
			}
			else{
				status=SesDataFifoWrite(&(RsrcManager->session_list[RsrcManager->bus_rsrc->session].data_rfifo), combData);
			}
			
			SesStatusFree(&(RsrcManager->session_list[des_mfi]));
		}
		
		if(status!=MFI_SUCCESS){
			pool_create(&DataRecPool, combData->memsize, &pool);
			pool_free(pool, combData);
		}
		
		pool_create(&DataRecPool, sizeof(DATA), &pool);
		pool_free(pool, data);
		 
		return status;
	}
	
	//����֡
	data->d_id.bit.d_retry=0;
	if(num < DATA_TMP_FIFO_DEF_LEN-1)
	{
		for(i=0;i<CombData_Fifo_p->queue_len;++i){
			combData=temp->next;
			if(combData->startnum>=DATA_TMP_FIFO_DEF_LEN-1 || combData->d_id.all!=data->d_id.all || ((combData->startnum+combData->freamnum)%(DATA_TMP_FIFO_DEF_LEN-1))<=num){
				combData->time--;
				//��ʱ�������������ڴ�
				if(combData->time==0){
					temp->next=combData->next;
					pool_create(&DataRecPool, combData->memsize, &pool);
					pool_free(pool, combData);
					++cnt;
				}
				else
					temp=combData;
			}
			else{
				flag=1;
				target=combData;
				tgtmp=temp;
				temp=combData;
			}
		}
	}
	else
	{
		for(i=0;i<CombData_Fifo_p->queue_len;++i){
			combData=temp->next;
			if(combData->startnum<DATA_TMP_FIFO_DEF_LEN-1 || combData->d_id.all!=data->d_id.all){
				combData->time--;
				//��ʱ�������������ڴ�
				if(combData->time==0){
					temp->next=combData->next;
					pool_create(&DataRecPool, combData->memsize, &pool);
					pool_free(pool, combData);
					++cnt;
				}
				else
					temp=combData;
			}
			else if(combData->cnt + ((MfiPUInt16)data->d_char)[2] != (combData->freamnum + 1)){
					temp->next=combData->next;
					pool_create(&DataRecPool, combData->memsize, &pool);
					pool_free(pool, combData);
					++cnt;				
			}
			else{
				flag=1;
				target=combData;
				tgtmp=temp;
				temp=combData;
			}
		}
	}
	
	CombData_Fifo_p->queue_len-=cnt;
	if(flag==1){
		freampos=((MfiPUInt16)data->d_char)[2]-1;
		src_addr=(MfiByte*)(target+1)+freampos*(DATA_MAX_LEN-8);
		memcpy(src_addr,data->d_char+8,((MfiPUInt16)data->d_char)[0]);
		target->cnt--;
		target->len+=((MfiPUInt16)data->d_char)[0];
		
		pool_create(&DataRecPool, sizeof(DATA), &pool);
		pool_free(pool, data);
		
		if(target->cnt==1){
			tgtmp->next=target->next;
			CombData_Fifo_p->queue_len--;
			
			//��src_ipת�Ựid des_mfi
			Ip_to_Mfi(src_ip,&des_mfi);
		
			//�жϻỰ�Ƿ��Ѿ��رգ��Ƿ�������ʹ���У�
			if((RsrcManager!=NULL)&&(SesStatusCheck(&(RsrcManager->session_list[des_mfi]))==INUSE))
			{
				status=MFI_SUCCESS;
				dataclass=data->d_id.bit.d_class0|data->d_id.bit.d_class1<<1|data->d_id.bit.d_class2<<2|data->d_id.bit.d_class3<<3;
				//���˴���Ҫ���䣺���ݴ���Ự������fifo���߸������ݴ���������ߵĻỰ
				//�������ݴ�����࣬Ӧ���������������fifo���������͹���������Դ�Ự����fifo
				if(dataclass==APP_DATA_CLASS){
					status=SesDataFifoWrite(&(RsrcManager->session_list[des_mfi].data_rfifo), target);
				}
				else{
					status=SesDataFifoWrite(&(RsrcManager->session_list[RsrcManager->bus_rsrc->session].data_rfifo), target);
				}
				
				SesStatusFree(&(RsrcManager->session_list[des_mfi]));
				
				if(status!=MFI_SUCCESS){
					pool_create(&DataRecPool, target->memsize, &pool);
					pool_free(pool, target);
				}
				
				return status;
			}
			
			pool_create(&DataRecPool,target->memsize, &pool);
			pool_free(pool, target);
		}

		return MFI_SUCCESS;
	}
	else{
		//�����ڴ桢�������
		freamnum=((MfiPUInt16)data->d_char)[3];
		freampos=((MfiPUInt16)data->d_char)[2]-1;
		
		if(num>=(DATA_TMP_FIFO_DEF_LEN-1) && freampos!=0)
			goto point;
			
		pool_create(&DataRecPool, sizeof(CombData_Head)+freamnum*(DATA_MAX_LEN-8), &pool);
		combData=(CombData_Head_p)pool_alloc(&DataRecPool, pool);
		src_addr=(MfiByte*)(combData+1)+freampos*(DATA_MAX_LEN-8);
		memcpy(src_addr,data->d_char+8,((MfiPUInt16)data->d_char)[0]);
		combData->memsize=pool->size;
		combData->time=COMBDATA_KEEP_TIME;
		combData->d_id.all=data->d_id.all;
		combData->freamnum=freamnum;
		combData->startnum=num-freampos;
		combData->cnt=freamnum;
		combData->len=((MfiPUInt16)data->d_char)[0];
		
		combData->next=CombData_Fifo_p->head.next;
		CombData_Fifo_p->head.next=combData;
		CombData_Fifo_p->queue_len++;
		
		point:
		pool_create(&DataRecPool, sizeof(DATA), &pool);
		pool_free(pool, data);
	}

	return MFI_SUCCESS;
}

MfiStatus MfiCombDataFree(MfiByte* buf)
{
	MfiUInt16 memsize=0;
	pool_head_p pool=NULL;
	CombData_Head_p combData=NULL;
	
	combData=(CombData_Head_p)buf-1;
	memsize=combData->memsize;
	memset(combData,0,memsize);
	pool_create(&DataRecPool, memsize, &pool);
	if(pool_free(pool, combData)==-1){
		free(combData);
	}
	return MFI_SUCCESS;
}

MfiStatus CombDataChange(CombData_Head_p combData, MfiPUInt32 dataclass, MfiPUInt32 datatype, MfiPUInt32 datasrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt)
{
	if(dataclass!=NULL)
		*dataclass=combData->d_id.bit.d_class0|combData->d_id.bit.d_class1<<1|combData->d_id.bit.d_class2<<2|combData->d_id.bit.d_class3<<3;
		
	if(datatype!=NULL)
		*datatype=combData->d_id.bit.d_type;
		
	if(datasrcaddr!=NULL)
		*datasrcaddr=combData->d_id.bit.d_src_addr;
	
	*bufp=(MfiPBuf)(combData+1);
	*retCnt=combData->len;
	
	return MFI_SUCCESS;
}