#include <stdio.h>
#include <stdlib.h>
#include "mfi_trigger.h"
#include "mfi_rsrc_bus.h"
#include "mfi_session.h"
#include "mfi_rsrc_manager.h"
#include "mfi_system_command.h"

//�����߷��亯��
//������->�ж��Ƿ��Ѿ������ù�->���������á�����������->�����޸�������Դ�жԴ����ߵĹ�����Ϣ->����������Ϣ��ǰ��
//condition����ʹ��TRIGCONDTION�꣬����ģʽʱ����ʹ�øú���л����㣬һ�������ö��������������
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
	//������
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
	
	//����ͬ��д�ӿڷ���������Ϣ��ǰ��
	buf[0]=trigline;
	buf[1]=mode;
	((MfiPUInt32)buf)[1]=condition;
	Mfi_to_Ip(Mfi,&dstaddr);
	SysWriteMsg(Mfi, MSG_SET_DEF_PORITY, MSG_SET_CLASS, SET_TRIGGER_ENABLE, dstaddr, (MfiPBuf)buf, 8, MFI_TMO_INFINITE,0);
	
	if(oldmode!=NULL) *oldmode=omode;
	if(oldcondition!=NULL) *oldcondition=ocondition;
	return status;
}

//ɾ��������������Ϣ����
//������->��鴥�����Ƿ�����ʹ��->���������Ϣ->����ɾ��������Դ�еĹ�����Ϣ->����������Ϣ��ǰ��
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
	//������		
	if(trigline>=MFI_TRIG_TTL0 && trigline<=MFI_TRIG_TTL7){
		line=line<<trigline;
		if(line & prsrc->trig_info.line_id ==0)
			return MFI_SUCCESS_TRIG_DELE;   //������δ������ʹ��
  	
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
		
		//����ͬ��д�ӿڷ���������Ϣ��ǰ��
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
		
		//����ͬ��д�ӿڷ���������Ϣ��ǰ��
		buf=8;
		Mfi_to_Ip(Mfi,&dstaddr);
		SysWriteMsg(Mfi, MSG_SET_DEF_PORITY, MSG_SET_CLASS, SET_TRIGGER_DISABLE, dstaddr, (MfiPBuf)(&buf), 2, MFI_TMO_INFINITE,0);
	}
	else
		return MFI_ERROR_INV_SETUP;	
	
	return MFI_SUCCESS;
}

//�����������������ߺ���
//������->ֻ��Mfi��Ӧ��ģ�鱻����Ϊ��������ģʽʱ�����ܹ��ø�mfi����������
MfiStatus AssertTrigger (MfiSession Mfi, MfiInt16 trigline)
{
	MfiPSessionInfo psession=NULL;
	MfiPModuleRsrcNodeInfo prsrc=NULL;
	MfiByte line=1;
	
	psession=&(RsrcManager->session_list[Mfi]);
	prsrc=(MfiPModuleRsrcNodeInfo)psession->rsrc;
	//������
	if(trigline<MFI_TRIG_TTL0 || trigline>MFI_TRIG_TTL7)
		return MFI_ERROR_INV_SETUP;		
		
	line=line<<trigline;
	if(line & prsrc->trig_info.line_id ==0)
		return MFI_ERROR_INV_SETUP;   //������δ������ʹ��
	if(prsrc->trig_info.mode & line == 1) //��ģ�鱻�����˴����ߣ����Ǳ�����Ϊ������������ʱ�������ܴ���������
		return MFI_ERROR_INV_SETUP;
	
	//����FPGA�ӿڣ�����FPGA�Ĵ�������������
	
	return MFI_SUCCESS;
}