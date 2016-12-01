#ifndef _MFI_SESSION_HEAD_
#define _MFI_SESSION_HEAD_

#include "mfi_define.h"
#include "mfiapi.h"
#include "mfi_event.h"
#include "mfi_io.h"
#include "mfi_operations.h"
#include "mfi_message.h"
#include "mfi_data.h"

/*
typedef struct{
}MfiMessage,*MfiPMessage;   //�ýṹ��Ʋο���һ���data fifo��ƣ�mfimessage����ָ��ָ��������Ϣbuf

typedef struct{
}Mfidata,*MfiPdata;
*/

typedef CombMsg_Head    MfiMessage;
typedef CombMsg_Head_p  MfiPMessage;
typedef CombData_Head   Mfidata;
typedef CombData_Head_p MfiPdata;

//ʹ��get���Խӿڴ�ǰ�˻�ȡ����ʱ����ȡ���İ������Ե���Ϣ����ڸýṹ��
typedef struct{
	MfiPMessage                  buf;
	pthread_mutex_t              lock;                  //fifo����������ͬ��
	pthread_cond_t               ready;                 //��������������ͬ��	
}MfiAttrGetBuf,*MfiPAttrGetBuf;

//��Ϣfifo�������ݴ�ûỰ�յ�����Ϣ
typedef struct{
	MfiBoolean                   empty;                 //FIFO�ձ�־λ
	MfiBoolean                   full;                  //FIFO����־λ
  MfiPMessage*                 buf;                   //����FIFO
	MfiUInt32                    buf_len;               //FIFO���
	MfiUInt32                    rindex;                //��ǰ�ɽ��ж�������FIFOָ��    
	MfiUInt32                    windex;                //��ǰ�ɽ���д������FIFOָ��   
	pthread_mutex_t              lock;                  //fifo����������ͬ��
	pthread_cond_t               ready;                 //��������������ͬ��
}MfiMsgFifo,*MfiPMsgFifo;

//����fifo�������ݴ�ûỰ�յ�������
typedef struct{
	MfiBoolean                   empty;                 //FIFO�ձ�־λ
	MfiBoolean                   full;                  //FIFO����־λ
  MfiPdata*                    buf;                   //����FIFO
	MfiUInt32                    buf_len;               //FIFO���
	MfiUInt32                    rindex;                //��ǰ�ɽ��ж�������FIFOָ��    
	MfiUInt32                    windex;                //��ǰ�ɽ���д������FIFOָ��   
	pthread_mutex_t              lock;                  //fifo����������ͬ��
	pthread_cond_t               ready;                 //��������������ͬ��
}MfiDataFifo,*MfiPDataFifo;

//��ŸûỰ��Ӧ���ĸ��̵߳�id, ��id=0ʱ, δ����ʹ�ø��߳�;��id=-1ʱ, �ȴ��رո��߳�;��id>0ʱ, �����̵߳�id
typedef struct{
	pthread_t callback_thr;
	pthread_t suspend_thr;
	pthread_t asyncread_thr;
	pthread_t asyncwrite_thr;
}MfiPthreadId;

typedef struct{
	MfiSession                    session;              
	void*                         rsrc;                 //ָ��ûỰ��Ӧ��Դ�ṹ��ָ�� 1 2
	MfiUInt32                     rsrc_type;            //��Դ���� 1 2 
	MfiPAttribute                 rsrc_attr;            //��Դ�������� 1 2
	MfiUInt32                     attr_amount;          //�������� 1 2
	MfiPOperations                rsrc_opt;             //���Բ��������� 1 2
	MfiMsgFifo                    msg_rfifo;            //��Ϣ�Ķ�FIFO 1 2
	MfiDataFifo                   data_rfifo;           //���ݵĶ�FIFO 1 2
	MfiAttrGetBuf                 attr_get_buf;         //ͨ��get���Խӿڻ�ȡ��Ӳ��������Ե���Ϣ������ڸýṹ�� 1 2
	MfiPEventCfgInfo              event_cfg;            //�¼����ýṹ���飬���������¼��Ĵ�����Ƶ� 1 2
	MfiUInt32                     cfg_amount;           //���� 1 2
	MfiUInt32                     event_mech_en;        //�ûỰ�е�һ��ʹ��ĳ����ʱ������Ӧ�����λ��ֱ���Ự�ر� 1 2
	MfiEventPointerQueue          queue_mech;           //�������Ϊ���л��Ƶ��¼����� 1 2
	MfiEventPointerQueue          callback_mech;        //�������Ϊ�ص����Ƶ��¼����� 1 2
	MfiEventPointerQueue          suspend_mech;         //�������Ϊ������Ƶ��¼����� 1 2
	MfiMechChangeFifo             mech_fifo;            //�����л�FIFO�����ڴӹ�������л��ض��л���ʱ�������л����񣬹������̻߳�ȡ
	MfiAsyncQueue                 rd_queue;             //��Ϣ�����ݵĶ��첽������� 1 2
	MfiAsyncQueue                 wt_queue;         		//��Ϣ�����ݵ�д�첽������� 1 2
	MfiPthreadId                  pthread_id;           //��¼4���̵߳�ID 1 2
	MfiJobId                      new_jobid;            //��ǰ�ɷ�����첽����ID���첽����ID�ķ����0-0xFFFFFFFF�� 1 2(!!!�����ڻص������е����첽��д�ӿڣ�����ҪΪ��ֵ���һ����)
	MfiUInt32                     next;                 //ָ��̬�����иûỰ�ṹ�ĺ�̽ڵ� 1 2
	MfiSesStatus                  is_inuse;             //�����жϾ�̬�����иýڵ��Ƿ��ѱ�����Ϊ����ʹ�õĻỰ 1 2
}MfiSessionInfo,*MfiPSessionInfo;

MfiStatus MfiMsgFifoInit(MfiPMsgFifo pfifo, MfiUInt32 len);
MfiStatus MfiDataFifoInit(MfiPDataFifo pfifo, MfiUInt32 len);
MfiStatus MfiAttrGetBufInit(MfiPAttrGetBuf buf);
void MfiMsgFifoDelete(MfiPMsgFifo pfifo);
void MfiDataFifoDelete(MfiPDataFifo pfifo);
void MfiAttrGetBufDelete(MfiPAttrGetBuf buf);
MfiStatus SesMsgFifoWrite(MfiPMsgFifo pfifo, MfiPMessage msg);
MfiStatus SesDataFifoWrite(MfiPDataFifo pfifo, MfiPdata data);

MfiStatus RsrcSessionClose(MfiObject Mfi);

#endif
