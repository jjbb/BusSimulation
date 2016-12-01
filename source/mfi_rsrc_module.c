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

//ģ����Դ�Ĳ���������
//���ܣ���ʼ��ģ����Դʱ����������ģ����Դ�����еĲ���
MfiOperations ModuleRsrcOperations={
	NULL,NULL,NULL,NULL,RsrcSessionClose,RsrcSetAttribute,RsrcGetAttribute,NULL,NULL,
	EnableEvent,DisableEvent,DiscardEvents,WaitOnEvent,InstallHandler,
	UninstallHandler,RsrcSessionReadMsg,ReadMsgAsync,RsrcSessionReadData,ReadDataAsync,RsrcSessionWriteMsg,
	WriteMsgAsync,RsrcSessionWriteData,WriteDataAsync,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	ConfigTrigger,AssertTrigger,DeleteTrigger,NULL
};

//ģ����Դ�¼����ýṹ����
//���ܣ����ڳ�ʼ��ģ����Դ��Ӧ�Ự���¼����ýṹ��ͬʱҲ�����˸ûỰ��֧�ֵ��¼�������Ϣ
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

//ģ����Դ��ʼ������
//���ܣ�����ģ����Դ
//�β��п������Ӳ����Ϣ�ṹ��ָ�룬���ں������ڳ�ʼ��Ӳ����Ϣ��ص����ԣ�����
MfiStatus Rsrc_Module_Init(MfiPModuleRsrcNodeInfo* p, MfiUInt32 num)
{
	MfiPModuleRsrcNodeInfo module_rsrc;
	MfiStatus status=MFI_SUCCESS;
	int i=0;
	
	module_rsrc=(MfiPModuleRsrcNodeInfo)malloc(sizeof(MfiModuleRsrcNodeInfo)); //����ģ����Դ
	
	if(module_rsrc==NULL){
		return MFI_ERROR_ALLOC;
	}
	
	memset(module_rsrc,0,sizeof(MfiModuleRsrcNodeInfo));
	memcpy(module_rsrc->rsrc_attr,ModuleRsrcAttr,sizeof(ModuleRsrcAttr)); //��ʼ������
	module_rsrc->attr_amount=MODULE_RSRC_ATTR_NUM;
	AttrInit(module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM);   //��ʼ������Ĭ��ֵ
	
	//��ȡӲ����Ϣ������ʼ��ģ����Դ��(��ȡģ��ID������ƴ��)
	if((status=Rsrc_Module_Attr_Init(module_rsrc,num))!=MFI_SUCCESS){
		free(module_rsrc);
		return status;
	}
	
	module_rsrc->session=-1;
	module_rsrc->rsrc_opt=ModuleRsrcOperations;            //��ʼ������������
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
	
	//������ע��˴���Ȼ��Ҫ����CAN�����޸�����޸�
	memcpy(&manfId, Module.Module_Info_p[num].manf_id, 2);
	memcpy(&modId, Module.Module_Info_p[num].mod_id, 4);

	//��ʼ����Դ��(��ȡģ��ID������ƴ��)
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
	module_rsrc->rsrc_attr[i].attr_val.attr_ui=RsrcManager->rmsession; //������Դ�������ĻỰID����

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
	module_rsrc->rsrc_attr[i].attr_val.attr_s=tmptype; //������Դ����
	
	if((i=FindAttr(MFI_ATTR_MANF_ID, module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	module_rsrc->rsrc_attr[i].attr_val.attr_ui=manfId; //����ģ�鳧��id
	
/*	if((i=FindAttr(MFI_ATTR_MANF_NAME, module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	module_rsrc->rsrc_attr[i].attr_val.attr_s=manfId; //����ģ�鳧����*/
	
	if((i=FindAttr(MFI_ATTR_MODEL_CODE, module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	module_rsrc->rsrc_attr[i].attr_val.attr_ui=modId; //����ģ��id
	
	if((i=FindAttr(MFI_ATTR_MODEL_LA, module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	module_rsrc->rsrc_attr[i].attr_val.attr_ui=Module.Module_Info_p[num].mod_ip; //����ģ��ip
	module_rsrc->ip=Module.Module_Info_p[num].mod_ip;

	if((i=FindAttr(MFI_ATTR_MSG_ACR0_FILTER, module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	module_rsrc->rsrc_attr[i].attr_val.attr_ui=(module_rsrc->ip)<<6; //�����˲��� 6ΪĿ�ĵ�ַ��32λ֡ͷ��ƫ����
	
	if((i=FindAttr(MFI_ATTR_DATA_ACR0_FILTER, module_rsrc->rsrc_attr, MODULE_RSRC_ATTR_NUM))==-1){
		status = MFI_ERROR_NSUP_ATTR;
		goto error3;
	}
	module_rsrc->rsrc_attr[i].attr_val.attr_ui=(module_rsrc->ip)<<6; //�����˲���
		
	return MFI_SUCCESS;
	
	error3:
		free(tmptype);
	error2:
		free(module_rsrc->rsrc_name);
	error1:
		return status;
}

//ɾ��ģ����Դ����
//���ܣ�������Դ�������ر�ʱ�����ģ����Դ��ռ�õ�ϵͳ��Դ
//���ڻỰ�ر�ʱ���ͷż��������Դ�̶�ʱ�䴰����ȴ󲿷���Ϣ�����иú�������������
/*
MfiStatus Rsrc_Module_Delete(MfiPModuleRsrcNodeInfo* p)
{
	MfiPModuleRsrcNodeInfo module_rsrc=*p;
	
	free(module_rsrc->rsrc_name);
	module_rsrc->rsrc_name=NULL;
	AttrFree(module_rsrc->rsrc_attr, module_rsrc->attr_amount); //�ͷ��ַ����ԵĿռ�
	free(module_rsrc);
	*p=NULL;
		
	return MFI_SUCCESS;
}
*/

MfiStatus Rsrc_Module_Delete(MfiPModuleRsrcNodeInfo* p)
{
	MfiPModuleRsrcNodeInfo module_rsrc=*p;
	
	//AttrFree(module_rsrc->rsrc_attr, module_rsrc->attr_amount); //�ͷ��ַ����ԵĿռ�  9.19��ע��
	//AttrInit(module_rsrc->rsrc_attr, module_rsrc->attr_amount);   //��ʼ������Ĭ��ֵ  9.19��ע��
		
	return MFI_SUCCESS;
}

//�ú��������״γ�ʼ����Դ������ʧ��ʱ������ģ����Դ
MfiStatus Rsrc_Module_Free(MfiPModuleRsrcNodeInfo* p)
{
	MfiPModuleRsrcNodeInfo module_rsrc=*p;
	
	module_rsrc->rsrc_name=NULL;
	AttrFree(module_rsrc->rsrc_attr, module_rsrc->attr_amount); //�ͷ��ַ����ԵĿռ�
	
	free(module_rsrc);
	*p=NULL;
		
	return MFI_SUCCESS;
}