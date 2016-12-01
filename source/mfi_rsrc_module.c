#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mfi_rsrc_module.h"
#include "mfi_event.h"
#include "mfi_session.h"
#include "mfi_io.h"
#include "mfi_rsrc_manager.h"
#include "mfi_trigger.h"
#include "mfi_module_info.h"

//模块资源的操作函数集
//功能：初始化模块资源时，用于配置模块资源所具有的操作
MfiOperations ModuleRsrcOperations={
	NULL,NULL,NULL,NULL,RsrcSessionClose,RsrcSetAttribute,RsrcGetAttribute,NULL,NULL,
	EnableEvent,DisableEvent,DiscardEvents,WaitOnEvent,InstallHandler,
	UninstallHandler,RsrcSessionReadMsg,ReadMsgAsync,RsrcSessionReadData,ReadDataAsync,RsrcSessionWriteMsg,
	WriteMsgAsync,RsrcSessionWriteData,WriteDataAsync,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	ConfigTrigger,AssertTrigger,DeleteTrigger,NULL
};

//模块资源事件配置结构数组
//功能：用于初始化模块资源对应会话的事件配置结构，同时也包含了该会话所支持的事件类型信息
MfiEventCfgInfo ModuleRsrcEvent[MODULE_RSRC_EVENT_NUM]={
	{MFI_EVENT_IO_COMPLETION,NOMECH,NULL,NULL},
	{MFI_EVENT_TRIG0,NOMECH,NULL,NULL},
	{MFI_EVENT_TRIG1,NOMECH,NULL,NULL},
	{MFI_EVENT_TRIG2,NOMECH,NULL,NULL},
	{MFI_EVENT_TRIG3,NOMECH,NULL,NULL},
	{MFI_EVENT_TRIG4,NOMECH,NULL,NULL},
	{MFI_EVENT_TRIG5,NOMECH,NULL,NULL},
	{MFI_EVENT_TRIG6,NOMECH,NULL,NULL},
	{MFI_EVENT_TRIG7,NOMECH,NULL,NULL},
};

//模块资源初始化函数
//功能：创建模块资源
//形参中可能添加硬件信息结构的指针，用于函数体内初始化硬件信息相关的属性！！！
MfiStatus Rsrc_Module_Init(MfiPModuleRsrcNodeInfo* p, MfiUInt32 num)
{
	MfiPModuleRsrcNodeInfo module_rsrc;
	MfiStatus status=MFI_SUCCESS;
	int i=0;
	
	module_rsrc=(MfiPModuleRsrcNodeInfo)malloc(sizeof(MfiModuleRsrcNodeInfo)); //创建模块资源
	
	if(module_rsrc==NULL){
		return MFI_ERROR_ALLOC;
	}
	
	memset(module_rsrc,0,sizeof(MfiModuleRsrcNodeInfo));
	memcpy(module_rsrc->rsrc_attr,ModuleRsrcAttr,sizeof(ModuleRsrcAttr)); //初始化属性
	module_rsrc->attr_amount=MODULE_RSRC_ATTR_NUM;
	AttrInit(module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM);   //初始化属性默认值
	
	//获取硬件信息，并初始化模块资源名(获取模块ID等属性拼接)
	if((status=Rsrc_Module_Attr_Init(module_rsrc,num))!=MFI_SUCCESS){
		free(module_rsrc);
		return status;
	}
	
	module_rsrc->session=-1;
	module_rsrc->rsrc_opt=ModuleRsrcOperations;            //初始化操作函数集
	module_rsrc->next=NULL;
	
	*p=module_rsrc;
	
	return MFI_SUCCESS;
}

MfiStatus Rsrc_Module_Attr_Init(MfiPModuleRsrcNodeInfo module_rsrc,MfiUInt32 num)
{
	MfiStatus status=MFI_SUCCESS;
	MfiString tmptype=NULL;
	MfiUInt32 manfId=0,modId=0;
	int i=0;
	
	//！！！注意此处仍然需要根据CAN总线修改情况修改
	memcpy(&manfId, Module.Module_Info_p[num].manf_id, 2);
	memcpy(&modId, Module.Module_Info_p[num].mod_id, 4);

	//初始化资源名(获取模块ID等属性拼接)
	if((status=RsrcNameCreate(MODULE_RSRC_TYPE_S,manfId,modId,&(module_rsrc->rsrc_name))) != MFI_SUCCESS)
		goto error1;

	if((i=FindAttr(MFI_ATTR_RSRC_NAME, module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error2;
	}
	module_rsrc->rsrc_attr[i].attr_val.attr_s=module_rsrc->rsrc_name;
	 
	if((i=FindAttr(MFI_ATTR_RM_SESSION, module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error2;
	}
	module_rsrc->rsrc_attr[i].attr_val.attr_ui=RsrcManager->rmsession; //设置资源管理器的会话ID属性

	tmptype=(MfiString)malloc(sizeof(MODULE_RSRC_TYPE_S));
	if(tmptype==NULL){
		status = MFI_ERROR_ALLOC;
		goto error2;
	}
	strcpy(tmptype,MODULE_RSRC_TYPE_S);
	if((i=FindAttr(MFI_ATTR_RSRC_CLASS, module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	module_rsrc->rsrc_attr[i].attr_val.attr_s=tmptype; //设置资源类型
	
	if((i=FindAttr(MFI_ATTR_MANF_ID, module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	module_rsrc->rsrc_attr[i].attr_val.attr_ui=manfId; //设置模块厂家id
	
/*	if((i=FindAttr(MFI_ATTR_MANF_NAME, module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	module_rsrc->rsrc_attr[i].attr_val.attr_s=manfId; //设置模块厂家名*/
	
	if((i=FindAttr(MFI_ATTR_MODEL_CODE, module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	module_rsrc->rsrc_attr[i].attr_val.attr_ui=modId; //设置模块id
	
	if((i=FindAttr(MFI_ATTR_MODEL_LA, module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	module_rsrc->rsrc_attr[i].attr_val.attr_ui=Module.Module_Info_p[num].mod_ip; //设置模块ip
	module_rsrc->ip=Module.Module_Info_p[num].mod_ip;

	if((i=FindAttr(MFI_ATTR_MSG_ACR0_FILTER, module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	module_rsrc->rsrc_attr[i].attr_val.attr_ui=(module_rsrc->ip)<<6; //设置滤波器 6为目的地址在32位帧头的偏移量
	
	if((i=FindAttr(MFI_ATTR_DATA_ACR0_FILTER, module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	module_rsrc->rsrc_attr[i].attr_val.attr_ui=(module_rsrc->ip)<<6; //设置滤波器
		
	return MFI_SUCCESS;
	
	error3:
		free(tmptype);
	error2:
		free(module_rsrc->rsrc_name);
	error1:
		return status;
}

//删除模块资源函数
//功能：用于资源管理器关闭时，清除模块资源所占用的系统资源
//由于会话关闭时，释放及清除了资源固定时间窗分配等大部分信息，所有该函数工作量不大
/*
MfiStatus Rsrc_Module_Delete(MfiPModuleRsrcNodeInfo* p)
{
	MfiPModuleRsrcNodeInfo module_rsrc=*p;
	
	free(module_rsrc->rsrc_name);
	module_rsrc->rsrc_name=NULL;
	AttrFree(module_rsrc->rsrc_attr, module_rsrc->attr_amount); //释放字符属性的空间
	free(module_rsrc);
	*p=NULL;
		
	return MFI_SUCCESS;
}
*/

MfiStatus Rsrc_Module_Delete(MfiPModuleRsrcNodeInfo* p)
{
	MfiPModuleRsrcNodeInfo module_rsrc=*p;
	
	//AttrFree(module_rsrc->rsrc_attr, module_rsrc->attr_amount); //释放字符属性的空间  9.19：注释
	//AttrInit(module_rsrc->rsrc_attr, module_rsrc->attr_amount);   //初始化属性默认值  9.19：注释
		
	return MFI_SUCCESS;
}

//该函数用于首次初始化资源管理器失败时，清理模块资源
MfiStatus Rsrc_Module_Free(MfiPModuleRsrcNodeInfo* p)
{
	MfiPModuleRsrcNodeInfo module_rsrc=*p;
	
	module_rsrc->rsrc_name=NULL;
	AttrFree(module_rsrc->rsrc_attr, module_rsrc->attr_amount); //释放字符属性的空间
	
	free(module_rsrc);
	*p=NULL;
		
	return MFI_SUCCESS;
}