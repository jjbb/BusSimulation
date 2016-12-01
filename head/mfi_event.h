#ifndef _MFI_EVENT_HEAD_
#define _MFI_EVENT_HEAD_

#include <pthread.h>
#include "mfi_define.h"
#include "mfi_attribute.h"

//�����л�fifo
//���ܣ���һ���¼��Ĵ������ƴӹ�������л������л���ʱ����Ҫ���л���������FIFO��
//      �ȴ������߳�ȡ��������
typedef struct{
	MfiBoolean                   empty;                 //FIFO�ձ�־λ
	MfiBoolean                   full;                  //FIFO����־λ
  MfiEventType                 buf[MECHCHANGEFIFOLEN];//����FIFO
	MfiUInt32                    buf_len;               //FIFO���
	MfiUInt32                    rindex;                //��ǰ�ɽ��ж�������FIFOָ��    
	MfiUInt32                    windex;                //��ǰ�ɽ���д������FIFOָ��   
	pthread_mutex_t              lock;                  //fifo����������ͬ��
	pthread_cond_t               ready;                 //��������������ͬ��
}MfiMechChangeFifo,*MfiPMechChangeFifo;

typedef union{
	MfiAttribute               trig_event[TRIG_EVENT_ATTR_NUM];          //�����¼�������
	MfiAttribute               async_io_event[ASYNC_EVENT_ATTR_NUM];     //�첽IO�¼�������
	MfiAttribute               fault_detect_event[FAULT_EVENT_ATTR_NUM]; //�����¼�������
}MfiEventAttribute;

//�¼���Ϣ�ṹ
//���ܣ���¼�¼�����ϸ��Ϣ��ϵͳ�ڶ��в������¼�����ϸ��Ϣ������������Դ���������¼���Ϣ�ṹ�����У�
//      �Ự���¼������н�����ָ���¼���Ϣ�ṹ��ָ��
typedef struct _MfiEventInfo{
	MfiEvent                    event_id;                   //�¼���Ϣ�ṹ��λ����Դ��������
	MfiUInt32                   ref_count;                  //ָʾ���¼��������Ự��ռ��
	MfiEventType                event_type;                 //�¼�����
	MfiEventAttribute           event_attr;                 //�¼�������
	struct _MfiEventInfo*       next;
	struct _MfiEventInfo*       last;                       //֮��ɽ�MfiEventQueue�ĳ�ɢ�б������е��¼���Ϣ�ṹ�ȴ����ɢ�б��У������һ������
}MfiEventInfo,*MfiPEventInfo;

typedef struct{
	pthread_rwlock_t           queue_lock;                   //������
	MfiUInt32                  queue_len;
	MfiEventInfo               queue_head;                   //�¼���Ϣ�ṹ����Ϊ���е�ͷ��㣬�����κ�������Ϣ��head��lastָ����β����β��nextָ��head
	MfiPEventInfo              insert_point;                 //�¼������еĲ����
}MfiEventQueue,*MfiPEventQueue;

//�¼�ָ��ṹ
//���ܣ������¼���Ϣ�ṹ��ָ��
typedef struct _MfiEventPointer{
	MfiPEventInfo                event_info;                 //ָ��������¼�����Ϣ�ṹ
	struct _MfiEventPointer*     next;
	struct _MfiEventPointer*     last;
}MfiEventPointer,*MfiPEventPointer;

typedef struct{
	MfiUInt32                    queue_len;
	MfiEventPointer              queue_head;                   //ѭ��˫�������Ķ��ף�lastΪtail
	MfiPEventPointer             queue_tail;                   //ѭ��˫�������Ķ�β��nextΪhead
	MfiEventType                 wait_type;                    //����û��ȴ����¼����ͣ����ڶ��л���(waitonevent���Ҷ����е�Ҫ���¼���û��ʱ���øó�Ա�����������������¼���ʱ������)
	pthread_mutex_t              lock;                         //������
	pthread_cond_t               ready;                        //
}MfiEventPointerQueue,*MfiPEventPointerQueue;

//5.9����������ʱ�������ýṹ����ҲҪ�������ص��̻߳��ѯ�ýṹ�����߳̿����޸ĸýṹ���¼������߳�Ҳ��Ҫ��ѯ�ýṹ�Ի�ȡ�¼���ʹ�ܻ���
typedef struct{
	MfiEventType                 event_type;
	MfiUInt32                    is_mechanism_en;     //��ʹ��3�ֻ����е�һ��
	MfiHndlr                     handler;             //�ص����ƵĻص�����
	MfiAddr                      userhandle;          //�ص������Ĳ���
}MfiEventCfgInfo,*MfiPEventCfgInfo; 

typedef struct{
	MfiEventType                 event_type;
	MfiUInt32                    event_en;
}MfiEventEnInfo,*MfiPEventEnInfo;

extern MfiEventEnInfo all_event_en[10];

MfiPEventInfo Event_Find(MfiPEventQueue event_queue,MfiEvent event_id);
//MfiStatus EventClose(MfiObject Mfi);
MfiStatus EventPointerQueueInit(MfiPEventPointerQueue queue);
MfiStatus EventPointerQueueDelete(MfiPEventPointerQueue queue);
MfiStatus MfiMechChangeFifoInit(MfiPMechChangeFifo pfifo);
void MfiMechChangeFifoDelete(MfiPMechChangeFifo pfifo);
MfiBoolean MechChangeFifoWrite(MfiPMechChangeFifo pfifo,MfiEventType eventtype);
MfiBoolean MechChangeFifoRead(MfiPMechChangeFifo pfifo,MfiEventType* peventtype);
MfiStatus EventQueueDelete(void);
MfiStatus InsertEventInfo(MfiPEventInfo eventInfo);
MfiStatus InsertEventPointer(MfiSession Mfi, MfiPEventInfo eventInfo);
MfiStatus ForCloseCallBackThr(MfiSession Mfi);
MfiStatus ForCloseSuspendThr(MfiSession Mfi);

MfiStatus EventClose(MfiPEventInfo event_info);
MfiStatus EnableEvent(MfiSession Mfi, MfiEventType eventType, MfiUInt16 mechanism);
MfiStatus DisableEvent(MfiSession Mfi, MfiEventType eventType);
MfiStatus WaitOnEvent(MfiSession Mfi, MfiEventType inEventType, MfiUInt32 timeout,MfiPEventType outEventType, MfiPEvent outContext);
MfiStatus DiscardEvents(MfiSession Mfi, MfiEventType eventType, MfiUInt16 mechanism);
MfiStatus InstallHandler(MfiSession Mfi, MfiEventType eventType, MfiHndlr handler, MfiAddr userHandle);
MfiStatus UninstallHandler(MfiSession Mfi, MfiEventType eventType, MfiHndlr handler, MfiAddr userHandle);


void* thr_event_callback(void* arg);
void* thr_event_suspend(void* arg);

#endif