#ifndef _MFI_RSRC_BUS_HEADER_
#define _MFI_RSRC_BUS_HEADER_

#include "mfi_define.h"
#include "mfiapi.h"
#include "mfi_rsrc_module.h"
#include "mfi_event.h"

typedef struct _MfiTrigLineUseInfo{
	MfiSession                    session;            //�ô�����ʹ����Ϣ�ṹ��Ӧ�ĻỰ
	MfiUInt32                     is_inuse;           //ɾ��ĳ�Ựռ�ô�����ʱ����ɾ���ýṹ��ָʾ���Ϊ��ʹ�á�
	MfiUInt32                     mode;               //��ǰ��ģ��ռ�ô����ߵķ�ʽ�������򱻶�
	MfiUInt32                     condition;   				//ǰ��ģ������/�����������ߵ�����
//	struct _MfiTrigLineUseInfo*   last;
	struct _MfiTrigLineUseInfo*   next;
}MfiTrigLineUseInfo,*MfiPTrigLineUseInfo;

typedef struct{
//	MfiByte                   be_driver;          //��¼�ô������Ƿ��ѱ�������������ǰ��ģ��(5.23:���ڿ����ж����������ģ�飬�ñ���û������)
	MfiUInt32                 rsrc_amount;        //ռ���˸ô����ߵ�ģ�������
	MfiTrigLineUseInfo        info_list;          //�Ự�Դ����ߵ�ʹ�����
}MfiTrigLineManager;

typedef struct{
	MfiRsrc                   rsrc_name;
	MfiSession                session;                      //�ỰID
	MfiUInt32                 ip;
	MfiAttribute              rsrc_attr[BUS_RSRC_ATTR_NUM]; //ָ��ģ����Դ����������
	MfiUInt32                 attr_amount;                  //��������������Եĸ���
	MfiOperations             rsrc_opt;                     //ģ����Դ�Ĳ���������	
	MfiTrigLineManager        trig_line[TRIG_LINE_NUM];     //�����ߵķ������ṹ
//	MfiUInt32                 trigevent_en[TRIG_LINE_NUM];  //���ߴ����¼���ʹ��״��
//	MfiPSubWinNodeInfo        subwin_manager[SUBWIN_NUM];   //�̶�ʱ�䴰��Ϣ�ṹ��ָ������,ָ��������ģ����Դ��Ӧ�ĻỰ�ر�ʱ�ͷ�
	MfiUInt32                 subwin_manager_msg;
	MfiUInt32                 subwin_manager_data;

}MfiBusRsrcNodeInfo,*MfiPBusRsrcNodeInfo;

extern MfiEventCfgInfo BusRsrcEvent[BUS_RSRC_EVENT_NUM];
extern MfiOperations BusRsrcOperations;

MfiStatus Rsrc_Bus_Init(MfiPBusRsrcNodeInfo* p);
MfiStatus Rsrc_Bus_Delete(MfiPBusRsrcNodeInfo* p);
MfiStatus Rsrc_Bus_Free(MfiPBusRsrcNodeInfo* p);
MfiStatus Rsrc_Bus_Attr_Init(MfiPBusRsrcNodeInfo bus_rsrc);

#endif
