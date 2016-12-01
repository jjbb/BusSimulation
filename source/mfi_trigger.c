#include <stdio.h>
#include <stdlib.h>
#include "mfi_trigger.h"
#include "mfi_rsrc_bus.h"
#include "mfi_session.h"
#include "mfi_rsrc_manager.h"
#include "mfi_system_command.h"

//触发线分配函数
//检查参数->判断是否已经被配置过->保留就配置、设置新配置->查找修改总线资源中对触发线的管理信息->发送配置信息到前端
//condition参数使用TRIGCONDTION宏，主动模式时，可使用该宏进行或运算，一次性配置多个主动触发条件
MfiStatus ConfigTrigger(MfiSession Mfi, MfiInt16 trigline, MfiUInt16 mode, MfiUInt32 condition ,MfiPUInt16 oldmode, MfiPUInt32 oldcondition)
{
	MfiPSessionInfo psession=NULL;
	MfiPModuleRsrcNodeInfo prsrc=NULL;
	MfiPBusRsrcNodeInfo pbusrsrc=NULL;
	MfiPTrigLineUseInfo triglineinfo=NULL,temp=NULL;
	MfiByte line=1;
	MfiUInt16 omode=0;
	MfiUInt32 ocondition=0;
	MfiStatus status;
	MfiUInt16 buf[4];
	MfiUInt32 dstaddr;
	
	psession=&(RsrcManager->session_list[Mfi]);
	prsrc=(MfiPModuleRsrcNodeInfo)psession->rsrc;
	//检查参数
	if(trigline<MFI_TRIG_TTL0 || trigline>MFI_TRIG_TTL7 || mode<0 || mode>1)
		return MFI_ERROR_INV_SETUP;
	
	line=line<<trigline;
	if(line & prsrc->trig_info.line_id !=0)
		status=MFI_SUCCESS_TRIG_SET;
	else
		status=MFI_SUCCESS;
		
	prsrc->trig_info.line_id|=line;
	omode=(prsrc->trig_info.mode & line)>>trigline;
	mode ? (prsrc->trig_info.mode|=line) : (prsrc->trig_info.mode&=(~line));
	ocondition=prsrc->trig_info.condition[trigline];
	prsrc->trig_info.condition[trigline]=condition;
	
	pbusrsrc=RsrcManager->bus_rsrc;
	triglineinfo=pbusrsrc->trig_line[trigline].info_list.next;
	while(triglineinfo!=NULL && triglineinfo->session!=Mfi)
		triglineinfo=triglineinfo->next;
	if(triglineinfo!=NULL){
		triglineinfo->mode=mode;
		triglineinfo->condition=condition;
		if(triglineinfo->is_inuse==0){
			triglineinfo->is_inuse=1;
			(pbusrsrc->trig_line[trigline].rsrc_amount)+=1;
		}
	}
	else{
		temp=(MfiPTrigLineUseInfo)malloc(sizeof(MfiTrigLineUseInfo));
		if(temp==NULL){
			prsrc->trig_info.condition[trigline]=ocondition;
			omode ? (prsrc->trig_info.mode|=line) : (prsrc->trig_info.mode&=(~line));
			return MFI_ERROR_ALLOC;
		}
		
		temp->session=Mfi;
		temp->mode=mode;
		temp->condition=condition;
		temp->is_inuse=1;
		temp->next=pbusrsrc->trig_line[trigline].info_list.next;
		pbusrsrc->trig_line[trigline].info_list.next=temp;
		(pbusrsrc->trig_line[trigline].rsrc_amount)+=1;
	}
	
	//调用同步写接口发送配置消息到前端
	buf[0]=trigline;
	buf[1]=mode;
	((MfiPUInt32)buf)[1]=condition;
	Mfi_to_Ip(Mfi,&dstaddr);
	SysWriteMsg(Mfi, MSG_SET_DEF_PORITY, MSG_SET_CLASS, SET_TRIGGER_ENABLE, dstaddr, (MfiPBuf)buf, 8, MFI_TMO_INFINITE,0);
	
	if(oldmode!=NULL) *oldmode=omode;
	if(oldcondition!=NULL) *oldcondition=ocondition;
	return status;
}

//删除触发线配置信息函数
//检查参数->检查触发线是否被配置使用->清除配置信息->懒惰删除总线资源中的管理信息->发送配置信息到前端
MfiStatus DeleteTrigger(MfiSession Mfi, MfiInt16 trigline)
{
	MfiPSessionInfo psession=NULL;
	MfiPModuleRsrcNodeInfo prsrc=NULL;
	MfiPBusRsrcNodeInfo pbusrsrc=NULL;
	MfiPTrigLineUseInfo triglineinfo=NULL,temp=NULL;	
	MfiByte line=1;
	int i=0;
	MfiUInt16 buf;
	MfiUInt32 dstaddr;
	
	psession=&(RsrcManager->session_list[Mfi]);
	prsrc=(MfiPModuleRsrcNodeInfo)psession->rsrc;
	pbusrsrc=RsrcManager->bus_rsrc;
	//检查参数		
	if(trigline>=MFI_TRIG_TTL0 && trigline<=MFI_TRIG_TTL7){
		line=line<<trigline;
		if(line & prsrc->trig_info.line_id ==0)
			return MFI_SUCCESS_TRIG_DELE;   //触发线未被配置使用
  	
		prsrc->trig_info.line_id &= (~line);
		prsrc->trig_info.mode &= (~line);
		prsrc->trig_info.condition[trigline]=0;
		
		triglineinfo=pbusrsrc->trig_line[trigline].info_list.next;
		while(triglineinfo!=NULL && triglineinfo->session!=Mfi)
			triglineinfo=triglineinfo->next;
		if(triglineinfo!=NULL){
			triglineinfo->mode=0;
			triglineinfo->condition=0;
			triglineinfo->is_inuse=0;
			(pbusrsrc->trig_line[trigline].rsrc_amount)-=1;
		}
		
		//调用同步写接口发送配置消息到前端
		buf=trigline;
		Mfi_to_Ip(Mfi,&dstaddr);
		SysWriteMsg(Mfi, MSG_SET_DEF_PORITY, MSG_SET_CLASS, SET_TRIGGER_DISABLE, dstaddr, (MfiPBuf)(&buf), 2, MFI_TMO_INFINITE,0);

	}
	else if(trigline==MFI_TRIG_ALL)
	{
		if(prsrc->trig_info.line_id==0)
			return MFI_SUCCESS_TRIG_DELE;
		for(i=0;i<=7;i++)
		{
			if(line&prsrc->trig_info.line_id==0){
				line=line<<1;
				continue;
			}
			
			prsrc->trig_info.line_id &= (~line);
			prsrc->trig_info.mode &= (~line);
			prsrc->trig_info.condition[i]=0;
			
			triglineinfo=pbusrsrc->trig_line[i].info_list.next;
			while(triglineinfo!=NULL && triglineinfo->session!=Mfi)
				triglineinfo=triglineinfo->next;
			if(triglineinfo!=NULL){
				triglineinfo->mode=0;
				triglineinfo->condition=0;
				triglineinfo->is_inuse=0;
				(pbusrsrc->trig_line[i].rsrc_amount)-=1;
			}			
			
			line=line<<1;
		}
		
		//调用同步写接口发送配置消息到前端
		buf=8;
		Mfi_to_Ip(Mfi,&dstaddr);
		SysWriteMsg(Mfi, MSG_SET_DEF_PORITY, MSG_SET_CLASS, SET_TRIGGER_DISABLE, dstaddr, (MfiPBuf)(&buf), 2, MFI_TMO_INFINITE,0);
	}
	else
		return MFI_ERROR_INV_SETUP;	
	
	return MFI_SUCCESS;
}

//主机主动触发触发线函数
//检查参数->只有Mfi对应的模块被配置为被动触发模式时，才能够用该mfi来触发总线
MfiStatus AssertTrigger (MfiSession Mfi, MfiInt16 trigline)
{
	MfiPSessionInfo psession=NULL;
	MfiPModuleRsrcNodeInfo prsrc=NULL;
	MfiByte line=1;
	
	psession=&(RsrcManager->session_list[Mfi]);
	prsrc=(MfiPModuleRsrcNodeInfo)psession->rsrc;
	//检查参数
	if(trigline<MFI_TRIG_TTL0 || trigline>MFI_TRIG_TTL7)
		return MFI_ERROR_INV_SETUP;		
		
	line=line<<trigline;
	if(line & prsrc->trig_info.line_id ==0)
		return MFI_ERROR_INV_SETUP;   //触发线未被配置使用
	if(prsrc->trig_info.mode & line == 1) //该模块被分配了触发线，但是被配置为主动触发，此时主机不能触发触发线
		return MFI_ERROR_INV_SETUP;
	
	//调用FPGA接口，设置FPGA寄存器，触发总线
	
	return MFI_SUCCESS;
}