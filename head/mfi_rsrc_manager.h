#ifndef _MFI_RSRC_MANAGER_HEADER_
#define _MFI_RSRC_MANAGER_HEADER_

#include "mfi_define.h"
#include "mfiapi.h"
#include "mfi_attribute.h"
#include "mfi_operations.h"
#include "mfi_rsrc_module.h"
#include "mfi_rsrc_bus.h"
#include "mfi_session.h"
#include "mfi_event.h"

typedef struct _MfiRsrcFindListNode{
	MfiFindList                    findlist_id;                      //资源查找链表ID
	MfiUInt32                      rsrc_amount;                      //当前资源查找链表中的符合条件资源个数
	MfiRsrc                        rsrc_name[FINDRSRC_MAX_NUM];      //符合查找条件的资源的资源字符串数组
	MfiUInt32                      rsrc_index;                       //待返回的资源索引
	struct _MfiRsrcFindListNode*   last;
	struct _MfiRsrcFindListNode*   next;
}MfiRsrcFindListNode,*MfiPRsrcFindListNode;

//由于徐定科软件的调度机制是一个串行的机制，无需对资源链表加锁
typedef struct{
	MfiUInt32               queue_len;                        //资源查找链表队列的长度
	MfiRsrcFindListNode     queue_head;                       //队列头
//	MfiPRsrcFindListNode    queue_tail;                       //队列尾
	MfiFindList             min_id;                           //当前最小资源链表ID，用于给查找资源链表分配ID
}MfiRsrcFindListQueue;

typedef struct{
	MfiSession              rmsession;                        //资源管理器的会话ID
	MfiAttribute            rsrc_attr[RM_ATTR_NUM];           //资源管理器的属性数组
	MfiUInt32               attr_amount;                      //属性数组的中属性的个数
	MfiOperations           rsrc_opt;                         //资源管理器的操作函数集
	MfiSessionInfo          session_list[SESSION_MAX_NUM];    //会话分配链表
//pthread_mutex_t         session_list_lock;                //会话列表锁(由于java层面软件调度机制是单线程的，其对会话的打开与关闭操作不存在同步问题，暂不考虑加锁)
	MfiRsrcFindListQueue    findlist_queue;                   //查找资源链表结构，可同时保存多个查找资源链表
	MfiPModuleRsrcNodeInfo  module_rsrc;                      //模块资源链表
	MfiPBusRsrcNodeInfo     bus_rsrc;                         //总线资源链表
	MfiEventQueue           event_closing;                    //事件信息队列，系统中产生的所有事件汇集在资源管理器中管理
	MfiPEventEnInfo         event_en;                         //记录所有会话，所有事件的使能情况，用于事件产生时的判断
	MfiUInt32               event_amount;
}MfiRsrcManager,*MfiPRsrcManager;

extern MfiPRsrcManager RsrcManager;

extern MfiOperations RMOperations;

MfiStatus MfiOpenDefaultRM (MfiPSession Mfi);
MfiStatus RMOpen(MfiRsrc name, MfiPSession Mfi);
MfiStatus RMClose(MfiObject Mfi);
MfiStatus RM_Attr_Init(void);

MfiStatus MfiFindListQueueInit(MfiRsrcFindListQueue* queue);
MfiStatus MfiFindListQueueDelete(MfiRsrcFindListQueue* queue);
MfiStatus RMFindRsrc(MfiString expr, MfiPFindList Mfi, MfiPUInt32 retCnt, MfiChar desc[]);
MfiStatus RMFindNext(MfiFindList Mfi, MfiChar desc[]);
MfiStatus FindListClose(MfiFindList Mfi);

#endif
