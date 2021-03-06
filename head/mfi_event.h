#ifndef _MFI_EVENT_HEAD_
#define _MFI_EVENT_HEAD_

#include <pthread.h>
#include "mfi_define.h"
#include "mfi_attribute.h"

//机制切换fifo
//功能：当一个事件的处理机制从挂起机制切换到队列机制时，需要将切换任务加入该FIFO，
//      等待挂起线程取出任务处理
typedef struct{
	MfiBoolean                   empty;                 //FIFO空标志位
	MfiBoolean                   full;                  //FIFO满标志位
  MfiEventType                 buf[MECHCHANGEFIFOLEN];//缓存FIFO
	MfiUInt32                    buf_len;               //FIFO深度
	MfiUInt32                    rindex;                //当前可进行读操作的FIFO指针    
	MfiUInt32                    windex;                //当前可进行写操作的FIFO指针   
	pthread_mutex_t              lock;                  //fifo的锁，用于同步
	pthread_cond_t               ready;                 //条件变量，用于同步
}MfiMechChangeFifo,*MfiPMechChangeFifo;

typedef union{
	MfiAttribute               trig_event[TRIG_EVENT_ATTR_NUM];          //触发事件的属性
	MfiAttribute               async_io_event[ASYNC_EVENT_ATTR_NUM];     //异步IO事件的属性
	MfiAttribute               fault_detect_event[FAULT_EVENT_ATTR_NUM]; //错误事件的属性
}MfiEventAttribute;

//事件信息结构
//功能：记录事件的详细信息，系统内多有产生的事件的详细信息，均保存在资源管理器的事件信息结构队列中；
//      会话的事件队列中仅保存指向事件信息结构的指针
typedef struct _MfiEventInfo{
	MfiEvent                    event_id;                   //事件信息结构，位于资源管理器中
	MfiUInt32                   ref_count;                  //指示该事件被几个会话所占有
	MfiEventType                event_type;                 //事件类型
	MfiEventAttribute           event_attr;                 //事件的属性
	struct _MfiEventInfo*       next;
	struct _MfiEventInfo*       last;                       //之后可将MfiEventQueue改成散列表，所有的事件信息结构既存放在散列表中，又组成一个链表
}MfiEventInfo,*MfiPEventInfo;

typedef struct{
	pthread_rwlock_t           queue_lock;                   //队列锁
	MfiUInt32                  queue_len;
	MfiEventInfo               queue_head;                   //事件信息结构，作为队列的头结点，不含任何有用信息，head的last指向链尾，链尾的next指向head
	MfiPEventInfo              insert_point;                 //事件队列中的插入点
}MfiEventQueue,*MfiPEventQueue;

//事件指针结构
//功能：保存事件信息结构的指针
typedef struct _MfiEventPointer{
	MfiPEventInfo                event_info;                 //指向产生的事件的信息结构
	struct _MfiEventPointer*     next;
	struct _MfiEventPointer*     last;
}MfiEventPointer,*MfiPEventPointer;

typedef struct{
	MfiUInt32                    queue_len;
	MfiEventPointer              queue_head;                   //循环双向链表的队首，last为tail
	MfiPEventPointer             queue_tail;                   //循环双向链表的队尾，next为head
	MfiEventType                 wait_type;                    //存放用户等待的事件类型，用于队列机制(waitonevent查找队列中的要求事件，没有时设置该成员，并阻塞，当所需事件来时，唤醒)
	pthread_mutex_t              lock;                         //队列锁
	pthread_cond_t               ready;                        //
}MfiEventPointerQueue,*MfiPEventPointerQueue;

//5.9：后续更改时，该配置结构可能也要加锁，回调线程会查询该结构，主线程可能修改该结构，事件产生线程也需要查询该结构以获取事件的使能机制
typedef struct{
	MfiEventType                 event_type;
	MfiUInt32                    is_mechanism_en;     //可使能3种机制中的一种
	MfiHndlr                     handler;             //回调机制的回调函数
	MfiAddr                      userhandle;          //回调函数的参数
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