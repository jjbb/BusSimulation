#ifndef _MFI_RSRC_BUS_HEADER_
#define _MFI_RSRC_BUS_HEADER_

#include "mfi_define.h"
#include "mfiapi.h"
#include "mfi_rsrc_module.h"
#include "mfi_event.h"

typedef struct _MfiTrigLineUseInfo{
	MfiSession                    session;            //该触发线使用信息结构对应的会话
	MfiUInt32                     is_inuse;           //删除某会话占用触发线时，不删除该结构，指示标记为不使用。
	MfiUInt32                     mode;               //该前端模块占用触发线的方式：主动或被动
	MfiUInt32                     condition;   				//前端模块主动/被动触发总线的条件
//	struct _MfiTrigLineUseInfo*   last;
	struct _MfiTrigLineUseInfo*   next;
}MfiTrigLineUseInfo,*MfiPTrigLineUseInfo;

typedef struct{
//	MfiByte                   be_driver;          //记录该触发线是否已被分配主动触发前端模块(5.23:由于可以有多个主动触发模块，该变量没有意义)
	MfiUInt32                 rsrc_amount;        //占用了该触发线的模块的数量
	MfiTrigLineUseInfo        info_list;          //会话对触发线的使用情况
}MfiTrigLineManager;

typedef struct{
	MfiRsrc                   rsrc_name;
	MfiSession                session;                      //会话ID
	MfiUInt32                 ip;
	MfiAttribute              rsrc_attr[BUS_RSRC_ATTR_NUM]; //指向模块资源的属性数组
	MfiUInt32                 attr_amount;                  //属性数组的中属性的个数
	MfiOperations             rsrc_opt;                     //模块资源的操作函数集	
	MfiTrigLineManager        trig_line[TRIG_LINE_NUM];     //触发线的分配管理结构
//	MfiUInt32                 trigevent_en[TRIG_LINE_NUM];  //总线触发事件的使能状况
//	MfiPSubWinNodeInfo        subwin_manager[SUBWIN_NUM];   //固定时间窗信息结构的指针数组,指针内容由模块资源对应的会话关闭时释放
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
