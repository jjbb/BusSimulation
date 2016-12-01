#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mfi_rsrc_bus.h"
#include "mfi_attribute.h"
#include "mfi_event.h"
#include "mfi_rsrc_manager.h"
#include "mfi_trigger.h"
#include "mfi_module_info.h"

//������Դ�Ĳ���������
//���ܣ���ʼ��������Դʱ����������������Դ�����еĲ���
MfiOperations BusRsrcOperations={
	NULL,NULL,NULL,NULL,NULL,RsrcSetAttribute,RsrcGetAttribute,NULL,NULL,
	EnableEvent,DisableEvent,DiscardEvents,WaitOnEvent,InstallHandler,
	UninstallHandler,RsrcSessionReadMsg,ReadMsgAsync,RsrcSessionReadData,ReadDataAsync,
	RsrcSessionWriteMsg,WriteMsgAsync,RsrcSessionWriteData,WriteDataAsync,
	SysReadMsg,SysReadData,SysWriteMsg,SysWriteData,SysReadMsgAsync,SysWriteMsgAsync,
	SysReadDataAsync,SysWriteDataAsync,ConfigTrigger,AssertTrigger,DeleteTrigger
};

//������Դ�¼����ýṹ����
//���ܣ����ڳ�ʼ��������Դ��Ӧ�Ự���¼����ýṹ��ͬʱҲ�����˸ûỰ��֧�ֵ��¼�������Ϣ
MfiEventCfgInfo BusRsrcEvent[BUS_RSRC_EVENT_NUM]={
	{MFI_EVENT_EXCEPTION,NOMECH,NULL,NULL},
	{MFI_EVENT_IO_COMPLETION,NOMECH,NULL,NULL}
};

//������Դ��ʼ������
//���ܣ�����������Դ
MfiStatus Rsrc_Bus_Init(MfiPBusRsrcNodeInfo* p)
{
	MfiPBusRsrcNodeInfo bus_rsrc;
	MfiStatus status=MFI_SUCCESS;
	MfiString tmp=NULL;
	int i=0;
	
	bus_rsrc=(MfiPBusRsrcNodeInfo)malloc(sizeof(MfiBusRsrcNodeInfo)); //����������Դ
	
	if(bus_rsrc==NULL){
		return MFI_ERROR_ALLOC; 
	}
	
	memset(bus_rsrc,0,sizeof(MfiBusRsrcNodeInfo));
	memcpy(bus_rsrc->rsrc_attr,BusRsrcAttr,sizeof(BusRsrcAttr)); //��ʼ����������
	bus_rsrc->attr_amount=BUS_RSRC_ATTR_NUM;
	AttrInit(bus_rsrc->rsrc_attr, BUS_RSRC_ATTR_NUM);   //��ʼ������Ĭ��ֵ
	if((status=Rsrc_Bus_Attr_Init(bus_rsrc))!=MFI_SUCCESS){
		free(bus_rsrc);
		return status;
	}
	
	bus_rsrc->session=-1;
	bus_rsrc->ip=1;
	bus_rsrc->rsrc_opt=BusRsrcOperations;            //��ʼ������������
	for(;i<TRIG_LINE_NUM;i++)
		bus_rsrc->trig_line[i].info_list.next=NULL;//���ڰ�bus��Դ�ڵ�Ŀռ�ͳһ��Ϊ��0�����Ըò������ʡ��
	
	*p=bus_rsrc;
	
	return MFI_SUCCESS;
}

MfiStatus Rsrc_Bus_Attr_Init(MfiPBusRsrcNodeInfo bus_rsrc)
{
	MfiStatus status=MFI_SUCCESS;
	MfiString tmp=NULL;
	int i=0;
	
	//��ʼ��������Դ��(��ȡģ��ID������ƴ��)
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
	bus_rsrc->rsrc_attr[i].attr_val.attr_ui=RsrcManager->rmsession; //������Դ�������ĻỰID����
	
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
	bus_rsrc->rsrc_attr[i].attr_val.attr_s=tmp; //������Դ����
	
	if((i=FindAttr(MFI_ATTR_RSRC_INFO, bus_rsrc->rsrc_attr, BUS_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	bus_rsrc->rsrc_attr[i].attr_val.attr_s=((MfiString)Module.Module_Info_p)-2; //����ģ����Դ��Ϣ -2Ϊ���������ֽڵİ�����Ϣ
	
	if((i=FindAttr(MFI_ATTR_RSRC_NUM, bus_rsrc->rsrc_attr, BUS_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	bus_rsrc->rsrc_attr[i].attr_val.attr_ui=Module.number; //����ģ����Դ����
	
	return MFI_SUCCESS;
	
	error3:
		free(tmp);
	error2:
		free(bus_rsrc->rsrc_name);
	error1:

		return status;
}

//ɾ��������Դ����
//���ܣ�������Դ�������ر�ʱ�����������Դ��ռ�õ�ϵͳ��Դ
//���ڻỰ�ر�ʱ���ͷż������������Դ�еĴ󲿷���Ϣ�����иú�������������
/*
MfiStatus Rsrc_Bus_Delete(MfiPBusRsrcNodeInfo* p)
{
	int i=0;
	MfiPTrigLineUseInfo temp=NULL;
	MfiPBusRsrcNodeInfo bus_rsrc=*p;
	
	free(bus_rsrc->rsrc_name);
	bus_rsrc->rsrc_name=NULL;
	AttrFree(bus_rsrc->rsrc_attr, bus_rsrc->attr_amount); //�ͷ��ַ����ԵĿռ�
	//ɾ�����еĴ�������Ϣ�ṹ
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
	
	//AttrFree(bus_rsrc->rsrc_attr, bus_rsrc->attr_amount); //�ͷ��ַ����ԵĿռ�  9.19��ע��
	//AttrInit(bus_rsrc->rsrc_attr, bus_rsrc->attr_amount);   //��ʼ������Ĭ��ֵ  9.19��ע��
	//ɾ�����еĴ�������Ϣ�ṹ
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

//�ú��������״γ�ʼ����Դ������ʧ��ʱ������������Դ
//��ʼ����Դ������ʱ��δ�Դ����ߵȽ��з���
MfiStatus Rsrc_Bus_Free(MfiPBusRsrcNodeInfo* p)
{
	int i=0;
	MfiPTrigLineUseInfo temp=NULL;
	MfiPBusRsrcNodeInfo bus_rsrc=*p;
	
	bus_rsrc->rsrc_name=NULL;
	AttrFree(bus_rsrc->rsrc_attr, bus_rsrc->attr_amount); //�ͷ��ַ����ԵĿռ�
	
	free(bus_rsrc);
	*p=NULL;
		
	return MFI_SUCCESS;
}