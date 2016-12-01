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
}MfiMessage,*MfiPMessage;   //该结构设计参考第一版的data fifo设计，mfimessage中有指针指向具体的消息buf

typedef struct{
}Mfidata,*MfiPdata;
*/

typedef CombMsg_Head    MfiMessage;
typedef CombMsg_Head_p  MfiPMessage;
typedef CombData_Head   Mfidata;
typedef CombData_Head_p MfiPdata;

//使用get属性接口从前端获取属性时，获取到的包含属性的消息存放在该结构中
typedef struct{
	MfiPMessage                  buf;
	pthread_mutex_t              lock;                  //fifo的锁，用于同步
	pthread_cond_t               ready;                 //条件变量，用于同步	
}MfiAttrGetBuf,*MfiPAttrGetBuf;

//消息fifo，用于暂存该会话收到的消息
typedef struct{
	MfiBoolean                   empty;                 //FIFO空标志位
	MfiBoolean                   full;                  //FIFO满标志位
  MfiPMessage*                 buf;                   //缓存FIFO
	MfiUInt32                    buf_len;               //FIFO深度
	MfiUInt32                    rindex;                //当前可进行读操作的FIFO指针    
	MfiUInt32                    windex;                //当前可进行写操作的FIFO指针   
	pthread_mutex_t              lock;                  //fifo的锁，用于同步
	pthread_cond_t               ready;                 //条件变量，用于同步
}MfiMsgFifo,*MfiPMsgFifo;

//数据fifo，用于暂存该会话收到的数据
typedef struct{
	MfiBoolean                   empty;                 //FIFO空标志位
	MfiBoolean                   full;                  //FIFO满标志位
  MfiPdata*                    buf;                   //缓存FIFO
	MfiUInt32                    buf_len;               //FIFO深度
	MfiUInt32                    rindex;                //当前可进行读操作的FIFO指针    
	MfiUInt32                    windex;                //当前可进行写操作的FIFO指针   
	pthread_mutex_t              lock;                  //fifo的锁，用于同步
	pthread_cond_t               ready;                 //条件变量，用于同步
}MfiDataFifo,*MfiPDataFifo;

//存放该会话对应的四个线程的id, 当id=0时, 未分配使用该线程;当id=-1时, 等待关闭该线程;当id>0时, 正常线程的id
typedef struct{
	pthread_t callback_thr;
	pthread_t suspend_thr;
	pthread_t asyncread_thr;
	pthread_t asyncwrite_thr;
}MfiPthreadId;

typedef struct{
	MfiSession                    session;              
	void*                         rsrc;                 //指向该会话对应资源结构的指针 1 2
	MfiUInt32                     rsrc_type;            //资源类型 1 2 
	MfiPAttribute                 rsrc_attr;            //资源参数数组 1 2
	MfiUInt32                     attr_amount;          //属性数量 1 2
	MfiPOperations                rsrc_opt;             //属性操作函数集 1 2
	MfiMsgFifo                    msg_rfifo;            //消息的读FIFO 1 2
	MfiDataFifo                   data_rfifo;           //数据的读FIFO 1 2
	MfiAttrGetBuf                 attr_get_buf;         //通过get属性接口获取的硬件相关属性的消息，存放在该结构中 1 2
	MfiPEventCfgInfo              event_cfg;            //事件设置结构数组，用于配置事件的处理机制等 1 2
	MfiUInt32                     cfg_amount;           //数量 1 2
	MfiUInt32                     event_mech_en;        //该会话中第一次使能某机制时，将对应标记置位，直到会话关闭 1 2
	MfiEventPointerQueue          queue_mech;           //处理机制为队列机制的事件队列 1 2
	MfiEventPointerQueue          callback_mech;        //处理机制为回调机制的事件队列 1 2
	MfiEventPointerQueue          suspend_mech;         //处理机制为挂起机制的事件队列 1 2
	MfiMechChangeFifo             mech_fifo;            //机制切换FIFO，用于从挂起机制切换回队列机制时，保存切换任务，供挂起线程获取
	MfiAsyncQueue                 rd_queue;             //消息、数据的读异步任务队列 1 2
	MfiAsyncQueue                 wt_queue;         		//消息、数据的写异步任务队列 1 2
	MfiPthreadId                  pthread_id;           //记录4个线程的ID 1 2
	MfiJobId                      new_jobid;            //当前可分配的异步任务ID。异步任务ID的分配从0-0xFFFFFFFF。 1 2(!!!若可在回调函数中调用异步读写接口，则需要为该值添加一个锁)
	MfiUInt32                     next;                 //指向静态链表中该会话结构的后继节点 1 2
	MfiSesStatus                  is_inuse;             //用于判断静态链表中该节点是否已被分配为正在使用的会话 1 2
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
