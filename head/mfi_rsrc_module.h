#ifndef _MFI_RSRC_MODULE_HEADER_
#define _MFI_RSRC_MODULE_HEADER_

#include "mfi_define.h"
#include "mfiapi.h"
#include "mfi_attribute.h"
#include "mfi_operations.h"
#include "mfi_event.h"
//#include "mfi_rsrc_manager.h"

typedef struct{
	MfiByte                   line_id;                            //每一位代表一根触发线，当为1时，表示该模块占用该触发线
	MfiByte                   mode;                               //每一位代表一根触发线，当为1时，表示该模块占用触发线的模式为主动触发
	MfiUInt32                 condition[TRIG_LINE_NUM];    				//前端模块主动触发总线的条件(主动触发和被动触发条件的存储数组，可更改为一个，减少内存占用)
}MfiRsrcTrigInfo,*MfiPRsrcTrigInfo;

typedef struct _MfiSubWinNodeInfo{
	MfiUInt16                        num;                   //固定时间子窗的编号
	MfiUInt16                        is_local;              //标记该子窗是分陪给前端模块还是主机模块
	MfiSession                       session;               
	MfiUInt32                        start_time;            //子窗的起止时间
	MfiUInt32                        end_time;
	struct _MfiSubWinNodeInfo*       next;
}MfiSubWinNodeInfo,*MfiPSubWinNodeInfo;

typedef struct _MfiModuleRsrcNodeInfo{
	MfiRsrc                          rsrc_name;
	MfiSession                       session;
	MfiUInt32                        ip;
	MfiAttribute                     rsrc_attr[MODULE_RSRC_ATTR_NUM];   //指向模块资源的属性数组
	MfiUInt32                        attr_amount;                       //属性数组的中属性的个数
	MfiOperations                    rsrc_opt;                          //模块资源的操作函数集	
	MfiRsrcTrigInfo                  trig_info;                         //资源占用触发线的配置信息结构
	MfiPSubWinNodeInfo               subwin_list;                       //资源分配的固定时间窗的信息
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
