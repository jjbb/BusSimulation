#ifndef _MFI_MODULE_INFO_HEADER_
#define _MFI_MODULE_INFO_HEADER_

#include "mfiapi.h"

//前端模块的信息
typedef struct __Module_Info
{
	char mod_ip;      //动态分配地址
	char mod_id[4];   //板子固有信息:module id
	char manf_id[2];  //板子固有信息:manf id
	char mod_resv1;   //保留
	char mod_resv2;   //保留
}_Module_Info_t,*_Module_Info_p;

typedef struct __Module
{
	_Module_Info_p     Module_Info_p;
	MfiUInt32          Available_flag; //通信链路通畅的模块标记
	MfiUInt32          number;   //模块数量
}Module_t,*Module_p;

extern Module_t Module;

void Module_Useful_check(void);
MfiUInt32 Module_IP_Find(MfiUInt32 mod_mac);

#endif
