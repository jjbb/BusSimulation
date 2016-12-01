#ifndef _MFI_MODULE_INFO_HEADER_
#define _MFI_MODULE_INFO_HEADER_

#include "mfiapi.h"

//ǰ��ģ������Ϣ
typedef struct __Module_Info
{
	char mod_ip;      //��̬������ַ
	char mod_id[4];   //���ӹ�����Ϣ:module id
	char manf_id[2];  //���ӹ�����Ϣ:manf id
	char mod_resv1;   //����
	char mod_resv2;   //����
}_Module_Info_t,*_Module_Info_p;

typedef struct __Module
{
	_Module_Info_p     Module_Info_p;
	MfiUInt32          Available_flag; //ͨ����·ͨ����ģ������
	MfiUInt32          number;   //ģ������
}Module_t,*Module_p;

extern Module_t Module;

// void Module_Useful_check(void);
void initModuelInfo();
MfiUInt32 Module_IP_Find(MfiUInt32 mod_mac);

#endif
