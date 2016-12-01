#ifndef _MFI_RSRC_MODULE_HEADER_
#define _MFI_RSRC_MODULE_HEADER_

#include "mfi_define.h"
#include "mfiapi.h"
#include "mfi_attribute.h"
#include "mfi_operations.h"
#include "mfi_event.h"
//#include "mfi_rsrc_manager.h"

typedef struct{
	MfiByte                   line_id;                            //ÿһλ����һ�������ߣ���Ϊ1ʱ����ʾ��ģ��ռ�øô�����
	MfiByte                   mode;                               //ÿһλ����һ�������ߣ���Ϊ1ʱ����ʾ��ģ��ռ�ô����ߵ�ģʽΪ��������
	MfiUInt32                 condition[TRIG_LINE_NUM];    				//ǰ��ģ�������������ߵ�����(���������ͱ������������Ĵ洢���飬�ɸ���Ϊһ���������ڴ�ռ��)
}MfiRsrcTrigInfo,*MfiPRsrcTrigInfo;

typedef struct _MfiSubWinNodeInfo{
	MfiUInt16                        num;                   //�̶�ʱ���Ӵ��ı��
	MfiUInt16                        is_local;              //��Ǹ��Ӵ��Ƿ����ǰ��ģ�黹������ģ��
	MfiSession                       session;               
	MfiUInt32                        start_time;            //�Ӵ�����ֹʱ��
	MfiUInt32                        end_time;
	struct _MfiSubWinNodeInfo*       next;
}MfiSubWinNodeInfo,*MfiPSubWinNodeInfo;

typedef struct _MfiModuleRsrcNodeInfo{
	MfiRsrc                          rsrc_name;
	MfiSession                       session;
	MfiUInt32                        ip;
	MfiAttribute                     rsrc_attr[MODULE_RSRC_ATTR_NUM];   //ָ��ģ����Դ����������
	MfiUInt32                        attr_amount;                       //��������������Եĸ���
	MfiOperations                    rsrc_opt;                          //ģ����Դ�Ĳ���������	
	MfiRsrcTrigInfo                  trig_info;                         //��Դռ�ô����ߵ�������Ϣ�ṹ
	MfiPSubWinNodeInfo               subwin_list;                       //��Դ����Ĺ̶�ʱ�䴰����Ϣ
//	struct _MfiModuleRsrcNodeInfo*   last;  
	struct _MfiModuleRsrcNodeInfo*   next; 
}MfiModuleRsrcNodeInfo,*MfiPModuleRsrcNodeInfo;

extern MfiOperations ModuleRsrcOperations;
extern MfiEventCfgInfo ModuleRsrcEvent[MODULE_RSRC_EVENT_NUM];

MfiStatus Rsrc_Module_Init(MfiPModuleRsrcNodeInfo* p,MfiUInt32 num);
MfiStatus Rsrc_Module_Delete(MfiPModuleRsrcNodeInfo* p);
MfiStatus Rsrc_Module_Attr_Init(MfiPModuleRsrcNodeInfo module_rsrc,MfiUInt32 num);
MfiStatus Rsrc_Module_Free(MfiPModuleRsrcNodeInfo* p);

#endif
