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
	MfiFindList                    findlist_id;                      //��Դ��������ID
	MfiUInt32                      rsrc_amount;                      //��ǰ��Դ���������еķ���������Դ����
	MfiRsrc                        rsrc_name[FINDRSRC_MAX_NUM];      //���ϲ�����������Դ����Դ�ַ�������
	MfiUInt32                      rsrc_index;                       //�����ص���Դ����
	struct _MfiRsrcFindListNode*   last;
	struct _MfiRsrcFindListNode*   next;
}MfiRsrcFindListNode,*MfiPRsrcFindListNode;

//�����춨������ĵ��Ȼ�����һ�����еĻ��ƣ��������Դ�������
typedef struct{
	MfiUInt32               queue_len;                        //��Դ����������еĳ���
	MfiRsrcFindListNode     queue_head;                       //����ͷ
//	MfiPRsrcFindListNode    queue_tail;                       //����β
	MfiFindList             min_id;                           //��ǰ��С��Դ����ID�����ڸ�������Դ�������ID
}MfiRsrcFindListQueue;

typedef struct{
	MfiSession              rmsession;                        //��Դ�������ĻỰID
	MfiAttribute            rsrc_attr[RM_ATTR_NUM];           //��Դ����������������
	MfiUInt32               attr_amount;                      //��������������Եĸ���
	MfiOperations           rsrc_opt;                         //��Դ�������Ĳ���������
	MfiSessionInfo          session_list[SESSION_MAX_NUM];    //�Ự��������
//pthread_mutex_t         session_list_lock;                //�Ự�б���(����java����������Ȼ����ǵ��̵߳ģ���ԻỰ�Ĵ���رղ���������ͬ�����⣬�ݲ����Ǽ���)
	MfiRsrcFindListQueue    findlist_queue;                   //������Դ����ṹ����ͬʱ������������Դ����
	MfiPModuleRsrcNodeInfo  module_rsrc;                      //ģ����Դ����
	MfiPBusRsrcNodeInfo     bus_rsrc;                         //������Դ����
	MfiEventQueue           event_closing;                    //�¼���Ϣ���У�ϵͳ�в����������¼��㼯����Դ�������й���
	MfiPEventEnInfo         event_en;                         //��¼���лỰ�������¼���ʹ������������¼�����ʱ���ж�
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
