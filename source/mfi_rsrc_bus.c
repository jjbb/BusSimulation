#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mfi_rsrc_bus.h"
#include "mfi_attribute.h"
#include "mfi_event.h"
#include "mfi_rsrc_manager.h"
#include "mfi_trigger.h"
#include "mfi_module_info.h"

//总线资源的操作函数集
//功能：初始化总线资源时，用于配置总线资源所具有的操作
MfiOperations BusRsrcOperations={
	NULL,NULL,NULL,NULL,NULL,RsrcSetAttribute,RsrcGetAttribute,NULL,NULL,
	EnableEvent,DisableEvent,DiscardEvents,WaitOnEvent,InstallHandler,
	UninstallHandler,RsrcSessionReadMsg,ReadMsgAsync,RsrcSessionReadData,ReadDataAsync,
	RsrcSessionWriteMsg,WriteMsgAsync,RsrcSessionWriteData,WriteDataAsync,
	SysReadMsg,SysReadData,SysWriteMsg,SysWriteData,SysReadMsgAsync,SysWriteMsgAsync,
	SysReadDataAsync,SysWriteDataAsync,ConfigTrigger,AssertTrigger,DeleteTrigger
};

//总线资源事件配置结构数组
//功能：用于初始化总线资源对应会话的事件配置结构，同时也包含了该会话所支持的事件类型信息
MfiEventCfgInfo BusRsrcEvent[BUS_RSRC_EVENT_NUM]={
	{MFI_EVENT_EXCEPTION,NOMECH,NULL,NULL},
	{MFI_EVENT_IO_COMPLETION,NOMECH,NULL,NULL}
};

//总线资源初始化函数
//功能：创建总线资源
MfiStatus Rsrc_Bus_Init(MfiPBusRsrcNodeInfo* p)
{
	MfiPBusRsrcNodeInfo bus_rsrc;
	MfiStatus status=MFI_SUCCESS;
	MfiString tmp=NULL;
	int i=0;
	
	bus_rsrc=(MfiPBusRsrcNodeInfo)malloc(sizeof(MfiBusRsrcNodeInfo)); //创建总线资源
	
	if(bus_rsrc==NULL){
		return MFI_ERROR_ALLOC; 
	}
	
	memset(bus_rsrc,0,sizeof(MfiBusRsrcNodeInfo));
	memcpy(bus_rsrc->rsrc_attr,BusRsrcAttr,sizeof(BusRsrcAttr)); //初始化总线属性
	bus_rsrc->attr_amount=BUS_RSRC_ATTR_NUM;
	AttrInit(bus_rsrc->rsrc_attr, BUS_RSRC_ATTR_NUM);   //初始化属性默认值
	if((status=Rsrc_Bus_Attr_Init(bus_rsrc))!=MFI_SUCCESS){
		free(bus_rsrc);
		return status;
	}
	
	bus_rsrc->session=-1;
	bus_rsrc->ip=1;
	bus_rsrc->rsrc_opt=BusRsrcOperations;            //初始化操作函数集
	for(;i<TRIG_LINE_NUM;i++)
		bus_rsrc->trig_line[i].info_list.next=NULL;//由于把bus资源节点的空间统一置为了0，所以该步骤可以省略
	
	*p=bus_rsrc;
	
	return MFI_SUCCESS;
}

MfiStatus Rsrc_Bus_Attr_Init(MfiPBusRsrcNodeInfo bus_rsrc)
{
	MfiStatus status=MFI_SUCCESS;
	MfiString tmp=NULL;
	int i=0;
	
	//初始化总线资源名(获取模块ID等属性拼接)
	if((status=RsrcNameCreate(BUS_RSRC_TYPE_S,BUS_MANF_ID,0,&(bus_rsrc->rsrc_name))) != MFI_SUCCESS)
		goto error1;
	
	if((i=FindAttr(MFI_ATTR_RSRC_NAME, bus_rsrc->rsrc_attr, BUS_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error2;
	}
	bus_rsrc->rsrc_attr[i].attr_val.attr_s=bus_rsrc->rsrc_name; 
	
	if((i=FindAttr(MFI_ATTR_RM_SESSION, bus_rsrc->rsrc_attr, BUS_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error2;
	}
	bus_rsrc->rsrc_attr[i].attr_val.attr_ui=RsrcManager->rmsession; //设置资源管理器的会话ID属性
	
	tmp=(MfiString)malloc(sizeof(BUS_RSRC_TYPE_S));
	if(tmp==NULL){
		status = MFI_ERROR_ALLOC;
		goto error2;
	}
	strcpy(tmp,BUS_RSRC_TYPE_S);
	if((i=FindAttr(MFI_ATTR_RSRC_CLASS, bus_rsrc->rsrc_attr, BUS_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	bus_rsrc->rsrc_attr[i].attr_val.attr_s=tmp; //设置资源类型
	
	if((i=FindAttr(MFI_ATTR_RSRC_INFO, bus_rsrc->rsrc_attr, BUS_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	bus_rsrc->rsrc_attr[i].attr_val.attr_s=((MfiString)Module.Module_Info_p)-2; //设置模块资源信息 -2为纳入两个字节的板数信息
	
	if((i=FindAttr(MFI_ATTR_RSRC_NUM, bus_rsrc->rsrc_attr, BUS_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	bus_rsrc->rsrc_attr[i].attr_val.attr_ui=Module.number; //设置模块资源数量
	
	return MFI_SUCCESS;
	
	error3:
		free(tmp);
	error2:
		free(bus_rsrc->rsrc_name);
	error1:

		return status;
}

//删除总线资源函数
//功能：用于资源管理器关闭时，清除总线资源所占用的系统资源
//由于会话关闭时，释放及清除了总线资源中的大部分信息，所有该函数工作量不大
/*
MfiStatus Rsrc_Bus_Delete(MfiPBusRsrcNodeInfo* p)
{
	int i=0;
	MfiPTrigLineUseInfo temp=NULL;
	MfiPBusRsrcNodeInfo bus_rsrc=*p;
	
	free(bus_rsrc->rsrc_name);
	bus_rsrc->rsrc_name=NULL;
	AttrFree(bus_rsrc->rsrc_attr, bus_rsrc->attr_amount); //释放字符属性的空间
	//删除所有的触发线信息结构
	for(;i<TRIG_LINE_NUM;i++)
	{
		while(bus_rsrc->trig_line[i].info_list.next!=NULL)
		{
			temp=bus_rsrc->trig_line[i].info_list.next;
			bus_rsrc->trig_line[i].info_list.next=temp->next;
			free(temp);
		}
	}
	free(bus_rsrc);
	*p=NULL;
		
	return MFI_SUCCESS;
}*/

MfiStatus Rsrc_Bus_Delete(MfiPBusRsrcNodeInfo* p)
{
	int i=0;
	MfiPTrigLineUseInfo temp=NULL;
	MfiPBusRsrcNodeInfo bus_rsrc=*p;
	
	//AttrFree(bus_rsrc->rsrc_attr, bus_rsrc->attr_amount); //释放字符属性的空间  9.19：注释
	//AttrInit(bus_rsrc->rsrc_attr, bus_rsrc->attr_amount);   //初始化属性默认值  9.19：注释
	//删除所有的触发线信息结构
	for(;i<TRIG_LINE_NUM;i++)
	{
		while(bus_rsrc->trig_line[i].info_list.next!=NULL)
		{
			temp=bus_rsrc->trig_line[i].info_list.next;
			bus_rsrc->trig_line[i].info_list.next=temp->next;
			free(temp);
		}
	}

	return MFI_SUCCESS;
}

//该函数用于首次初始化资源管理器失败时，清理总线资源
//初始化资源管理器时，未对触发线等进行分配
MfiStatus Rsrc_Bus_Free(MfiPBusRsrcNodeInfo* p)
{
	int i=0;
	MfiPTrigLineUseInfo temp=NULL;
	MfiPBusRsrcNodeInfo bus_rsrc=*p;
	
	bus_rsrc->rsrc_name=NULL;
	AttrFree(bus_rsrc->rsrc_attr, bus_rsrc->attr_amount); //释放字符属性的空间
	
	free(bus_rsrc);
	*p=NULL;
		
	return MFI_SUCCESS;
}