#include "mfiapi.h"
#include "mfi_attribute.h"
#include "mfi_rsrc_manager.h"
#include "mfi_system_command.h"
#include "mfi_event.h"
#include "mfi_attr_deal.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//静态属性表
//功能：存储各属性的静态特性，如属性的类型、是否与硬件相关、是否实时更新，属性的默认值、上限、下限等，
//			与前端相关的属性的设置和获取指令、特定属性的设置获取函数
MfiStaticAttrTable attr_table[]={
	{MFI_ATTR_RSRC_CLASS,        READONLY|NHDWARE<<1|NREALTIME<<2|STRINGTYPE<<3,  .def_val.attr_s=NO_VAL,              .max_val.attr_s=NO_VAL,                .min_val.attr_s=NO_VAL,              {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_RSRC_NAME,         READONLY|NHDWARE<<1|NREALTIME<<2|STRINGTYPE<<3,  .def_val.attr_s=NO_VAL,              .max_val.attr_s=NO_VAL,                .min_val.attr_s=NO_VAL,              {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_RSRC_IMPL_VERSION, READWRITE|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,   .def_val.attr_ui=0,                  .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_RSRC_MANF_NAME,    READWRITE|NHDWARE<<1|NREALTIME<<2|STRINGTYPE<<3, .def_val.attr_s=NO_VAL,              .max_val.attr_s=NO_VAL,                .min_val.attr_s=NO_VAL,              {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_RSRC_MANF_ID,      READWRITE|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,   .def_val.attr_ui=0,                  .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_RSRC_SPEC_VERSION, READONLY|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=MFI_SPEC_VERSION,   .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_RM_SESSION,        READONLY|NHDWARE<<1|NREALTIME<<2|INTTYPE<<3,     .def_val.attr_i=RM_MIN_SESSION_ID,   .max_val.attr_i=RM_MAX_SESSION_ID,     .min_val.attr_i=RM_MIN_SESSION_ID,   {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_RSRC_INFO,         READONLY|NHDWARE<<1|NREALTIME<<2|STRINGTYPE<<3,  .def_val.attr_s=NO_VAL,              .max_val.attr_s=NO_VAL,                .min_val.attr_s=NO_VAL,              {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_RSRC_NUM,          READONLY|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=0,                  .max_val.attr_ui=SESSION_MAX_NUM,      .min_val.attr_ui=0,                  {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_MANF_ID,           READONLY|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,     .def_val.attr_ui=0,                  .max_val.attr_ui=0x0000FFFF,           .min_val.attr_ui=0,                  {NO_VAL},  GET_MANF_ID_CMD,           NO_VAL},
	{MFI_ATTR_MANF_NAME,         READONLY|HDWARE<<1|NREALTIME<<2|STRINGTYPE<<3,   .def_val.attr_s=NO_VAL,              .max_val.attr_s=NO_VAL,                .min_val.attr_s=NO_VAL,              {NO_VAL},  GET_MANF_NAME_CMD,         NO_VAL},
	{MFI_ATTR_MODEL_CODE,        READONLY|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,     .def_val.attr_ui=0,                  .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  GET_MODEL_CODE_CMD,        NO_VAL},
	{MFI_ATTR_MODEL_NAME,        READONLY|HDWARE<<1|NREALTIME<<2|STRINGTYPE<<3,   .def_val.attr_s=NO_VAL,              .max_val.attr_s=NO_VAL,                .min_val.attr_s=NO_VAL,              {NO_VAL},  GET_MODEL_NAME_CMD,        NO_VAL},
	{MFI_ATTR_MODEL_LA,          READONLY|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=1,                  .max_val.attr_ui=RSRC_MAX_SESSION_ID,  .min_val.attr_ui=1,                  {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_MAX_QUEUE_LENGTH,  READWRITE|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,   .def_val.attr_ui=DEF_QUEUE_LEN,      .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=1,                  {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_TMO_VALUE,         READWRITE|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,   .def_val.attr_ui=DEF_TWO_VALUE,      .max_val.attr_ui=MFI_TMO_INFINITE,     .min_val.attr_ui=MFI_TMO_IMMEDIATE,  {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_USER_DATA,         READWRITE|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,   .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_JOB_ID,            READONLY|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=0,                  .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_EVENT_TYPE,        READONLY|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=0,                  .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_RECV_TRIG_ID,      READONLY|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=MFI_TRIG_TTL7,        .min_val.attr_ui=MFI_TRIG_TTL0,      {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_STATUS,            READONLY|NHDWARE<<1|NREALTIME<<2|INTTYPE<<3,     .def_val.attr_i=NO_VAL,              .max_val.attr_i=NO_VAL,                .min_val.attr_i=NO_VAL,              {NO_VAL},  NO_VAL,                    NO_VAL},	
	{MFI_ATTR_RET_COUNT,         READONLY|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  NO_VAL,                    NO_VAL},
	//注意：MFI_ATTR_BUFFER的属性值为UINTTYPE类型是为了避免被自动释放而出现bug
	{MFI_ATTR_BUFFER,            READONLY|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=NO_VAL,               .min_val.attr_ui=NO_VAL,             {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_OPER_NAME,         READONLY|NHDWARE<<1|NREALTIME<<2|STRINGTYPE<<3,  .def_val.attr_s=NO_VAL,              .max_val.attr_s=NO_VAL,                .min_val.attr_s=NO_VAL,              {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_EXCEP_ID,          READONLY|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_EXCEP_BUFFER,      READONLY|NHDWARE<<1|NREALTIME<<2|STRINGTYPE<<3,  .def_val.attr_s=NO_VAL,              .max_val.attr_s=NO_VAL,                .min_val.attr_s=NO_VAL,              {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_EXCEP_BUFLEN,      READONLY|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_MSG_BASIC_TLEN,    READWRITE|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=DEF_MSG_BASIC_TLEN, .max_val.attr_ui=MAX_MSG_BASIC_TLEN,   .min_val.attr_ui=MIN_MSG_BASIC_TLEN, {NO_VAL},  GET_MSG_TLEN_CMD,          SET_MSG_TLEN_CMD,       SetAttr_Msg_Tlen},
	{MFI_ATTR_MSG_FIX_TLEN,      READWRITE|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=DEF_MSG_FIX_TLEN,   .max_val.attr_ui=MAX_MSG_FIX_TLEN,     .min_val.attr_ui=MIN_MSG_FIX_TLEN,   {NO_VAL},  GET_MSG_TLEN_CMD,          SET_MSG_TLEN_CMD,       SetAttr_Msg_Tlen},
	{MFI_ATTR_DATA_BASIC_TLEN,   READWRITE|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=DEF_DATA_BASIC_TLEN,.max_val.attr_ui=MAX_DATA_BASIC_TLEN,  .min_val.attr_ui=MIN_DATA_BASIC_TLEN,{NO_VAL},  GET_DATA_TLEN_CMD,         SET_DATA_TLEN_CMD,      SetAttr_Data_Tlen},
	{MFI_ATTR_DATA_FIX_TLEN,     READWRITE|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=DEF_DATA_FIX_TLEN,  .max_val.attr_ui=MAX_DATA_FIX_TLEN,    .min_val.attr_ui=MIN_DATA_FIX_TLEN,  {NO_VAL},  GET_DATA_TLEN_CMD,         SET_DATA_TLEN_CMD,      SetAttr_Data_Tlen},
	{MFI_ATTR_MSG_ACR0_FILTER,   READONLY|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,     .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  GET_MSG_FILTER0_CMD,       NO_VAL,                 NO_VAL,              GetAttr_Msg_FILTER0},
	{MFI_ATTR_MSG_ACR1_FILTER,   READONLY|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,     .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  GET_MSG_FILTER0_CMD,       NO_VAL,                 NO_VAL,              GetAttr_Msg_FILTER0},
	{MFI_ATTR_MSG_AMR0_FILTER,   READONLY|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,     .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  GET_MSG_FILTER0_CMD,       NO_VAL,                 NO_VAL,              GetAttr_Msg_FILTER0},
	{MFI_ATTR_MSG_ACR2_FILTER,   READWRITE|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  GET_MSG_FILTER1_CMD,       SET_MSG_FILTER1_CMD,    SetAttr_Msg_FILTER,  GetAttr_Msg_FILTER1},
	{MFI_ATTR_MSG_ACR3_FILTER,   READWRITE|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  GET_MSG_FILTER1_CMD,       SET_MSG_FILTER1_CMD,    SetAttr_Msg_FILTER,  GetAttr_Msg_FILTER1},
	{MFI_ATTR_MSG_AMR1_FILTER,   READWRITE|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  GET_MSG_FILTER1_CMD,       SET_MSG_FILTER1_CMD,    SetAttr_Msg_FILTER,  GetAttr_Msg_FILTER1},
	{MFI_ATTR_DATA_ACR0_FILTER,  READONLY|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,     .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  GET_DATA_FILTER0_CMD,      NO_VAL},
	{MFI_ATTR_DATA_ACR1_FILTER,  READONLY|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,     .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  GET_DATA_FILTER0_CMD,      NO_VAL},
	{MFI_ATTR_DATA_AMR0_FILTER,  READONLY|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,     .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  GET_DATA_FILTER0_CMD,      NO_VAL},
	{MFI_ATTR_DATA_ACR2_FILTER,  READWRITE|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  GET_DATA_FILTER1_CMD,      SET_DATA_FILTER1_CMD},
	{MFI_ATTR_DATA_ACR3_FILTER,  READWRITE|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  GET_DATA_FILTER1_CMD,      SET_DATA_FILTER1_CMD},
	{MFI_ATTR_DATA_AMR1_FILTER,  READWRITE|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  GET_DATA_FILTER1_CMD,      SET_DATA_FILTER1_CMD},
	{MFI_ATTR_MSG_RFIFO_LEN,     READWRITE|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,   .def_val.attr_ui=DEF_MSG_RFIFO_LEN,  .max_val.attr_ui=MAX_MSG_RFIFO_LEN,    .min_val.attr_ui=MIN_MSG_RFIFO_LEN,  {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_DATA_RFIFO_LEN,    READWRITE|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,   .def_val.attr_ui=DEF_DATA_RFIFO_LEN, .max_val.attr_ui=MAX_DATA_RFIFO_LEN,   .min_val.attr_ui=MIN_DATA_RFIFO_LEN, {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_DATA_TYPE,         READONLY|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_DATA_CLASS,        READONLY|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_DATA_ADDR,         READONLY|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,    .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  NO_VAL,                    NO_VAL},	
	{MFI_ATTR_MODEL_BIRTHDAY,    READONLY|HDWARE<<1|NREALTIME<<2|STRINGTYPE<<3,   .def_val.attr_s=NO_VAL,              .max_val.attr_s=NO_VAL,                .min_val.attr_s=NO_VAL,              {NO_VAL},  GET_MODEL_BIRTHDAY_CMD,    NO_VAL},	//10.23：新增
	{MFI_ATTR_MODE_MAX_CURRENT,  READONLY|HDWARE<<1|NREALTIME<<2|UINTTYPE<<3,     .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0xFFFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  NO_VAL,                    NO_VAL},
	{MFI_ATTR_MSG_FIX_TWIN_CFG,  READWRITE|HDWARE<<1|REALTIME<<2|UINTTYPE<<3,     .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0x7FFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  NO_VAL,                    SET_MSG_FIX_TWIN_CMD,   SetAttr_Msg_Twin_cfg},
	{MFI_ATTR_DATA_FIX_TWIN_CFG, READWRITE|HDWARE<<1|REALTIME<<2|UINTTYPE<<3,     .def_val.attr_ui=NO_VAL,             .max_val.attr_ui=0x7FFFFFFF,           .min_val.attr_ui=0,                  {NO_VAL},  NO_VAL,                    SET_DATA_FIX_TWIN_CMD,  SetAttr_Msg_Twin_cfg},
	{MFI_ATTR_ASYNC_TYPE,		 READONLY|NHDWARE<<1|NREALTIME<<2|UINTTYPE<<3,	  .def_val.attr_i=NO_VAL,			   .max_val.attr_i=NO_VAL,				  .min_val.attr_i=NO_VAL,			   {NO_VAL},  NO_VAL,					 NO_VAL},
};                                                                                                                                                                                              

//资源管理器的属性数组
//功能：资源管理器初始化时，该数组用于配置其所具有的属性及特定的值，如资源字符串、
//      资源厂家名、资源厂家ID、资源版本、资源类型等不同类型资源值不同但是初始确定的，可在此处写死
//(9.19:不能写死，资源字符串等字符串类型若写死，非动态分配，释放时出现问题；资源厂家名等根据资源驱
//动各不相同，由驱动调用VISA接口设置，也不能写死)
MfiAttribute RMAttr[RM_ATTR_NUM]={
	{MFI_ATTR_RSRC_NAME,NO_VAL},{MFI_ATTR_RSRC_MANF_NAME,NO_VAL},{MFI_ATTR_RSRC_MANF_ID,MFI_MANF_ID},
	{MFI_ATTR_RSRC_CLASS,NO_VAL},{MFI_ATTR_RSRC_IMPL_VERSION,NO_VAL},{MFI_ATTR_RSRC_SPEC_VERSION,MFI_SPEC_VERSION},
	{MFI_ATTR_RM_SESSION,NO_VAL},{MFI_ATTR_RSRC_INFO,NO_VAL},{MFI_ATTR_RSRC_NUM,NO_VAL},
};

//模块资源的资源类型的属性数组
//功能：模块资源初始化时，该数组用于配置其所具有的属性及特定的值
MfiAttribute ModuleRsrcAttr[MODULE_RSRC_ATTR_NUM]={
	{MFI_ATTR_RSRC_NAME,NO_VAL},{MFI_ATTR_RSRC_MANF_NAME,NO_VAL},{MFI_ATTR_RSRC_MANF_ID,NO_VAL},
	{MFI_ATTR_RSRC_CLASS,NO_VAL},{MFI_ATTR_RSRC_IMPL_VERSION,NO_VAL},{MFI_ATTR_RSRC_SPEC_VERSION,MFI_SPEC_VERSION},
	{MFI_ATTR_RM_SESSION,NO_VAL},{MFI_ATTR_MANF_ID,NO_VAL},{MFI_ATTR_MANF_NAME,NO_VAL},
	{MFI_ATTR_MODEL_CODE,NO_VAL},{MFI_ATTR_MODEL_NAME,NO_VAL},{MFI_ATTR_MODEL_LA,NO_VAL},
	{MFI_ATTR_MSG_ACR0_FILTER,NO_VAL},{MFI_ATTR_MSG_ACR1_FILTER,DEF_BROADCAST_FILTER},{MFI_ATTR_MSG_AMR0_FILTER,DEF_AMR0_FILTER},
	{MFI_ATTR_MSG_ACR2_FILTER,NO_VAL},{MFI_ATTR_MSG_ACR3_FILTER,NO_VAL},{MFI_ATTR_MSG_AMR1_FILTER,CLOSE_AMR_FILTER},
	{MFI_ATTR_DATA_ACR0_FILTER,NO_VAL},{MFI_ATTR_DATA_ACR1_FILTER,DEF_BROADCAST_FILTER},{MFI_ATTR_DATA_AMR0_FILTER,DEF_AMR0_FILTER},
	{MFI_ATTR_DATA_ACR2_FILTER,NO_VAL},{MFI_ATTR_DATA_ACR3_FILTER,NO_VAL},{MFI_ATTR_DATA_AMR1_FILTER,CLOSE_AMR_FILTER},
	{MFI_ATTR_MSG_RFIFO_LEN,NO_VAL},{MFI_ATTR_DATA_RFIFO_LEN,NO_VAL},{MFI_ATTR_MAX_QUEUE_LENGTH,NO_VAL},
	{MFI_ATTR_TMO_VALUE,NO_VAL},{MFI_ATTR_MODEL_BIRTHDAY,NO_VAL},{MFI_ATTR_MODE_MAX_CURRENT,NO_VAL},{MFI_ATTR_MSG_FIX_TWIN_CFG,NO_VAL},
	{MFI_ATTR_DATA_FIX_TWIN_CFG,NO_VAL}
};

//总线资源的资源类型的属性数组
//功能：总线资源初始化时，该数组用于配置其所具有的属性及特定的值
MfiAttribute BusRsrcAttr[BUS_RSRC_ATTR_NUM]={
	{MFI_ATTR_RSRC_NAME,NO_VAL},{MFI_ATTR_RSRC_MANF_NAME,NO_VAL},{MFI_ATTR_RSRC_MANF_ID,BUS_MANF_ID},
	{MFI_ATTR_RSRC_CLASS,NO_VAL},{MFI_ATTR_RSRC_IMPL_VERSION,NO_VAL},{MFI_ATTR_RSRC_SPEC_VERSION,MFI_SPEC_VERSION},
	{MFI_ATTR_RM_SESSION,NO_VAL},{MFI_ATTR_RSRC_INFO,NO_VAL},{MFI_ATTR_RSRC_NUM,NO_VAL},
	{MFI_ATTR_MSG_BASIC_TLEN,NO_VAL},{MFI_ATTR_MSG_FIX_TLEN,NO_VAL},
	{MFI_ATTR_DATA_BASIC_TLEN,NO_VAL},{MFI_ATTR_DATA_FIX_TLEN,NO_VAL},
	{MFI_ATTR_MSG_ACR0_FILTER,DEF_HOST_ACR0_FILTER},{MFI_ATTR_MSG_ACR1_FILTER,DEF_HOST_ACR1_FILTER},{MFI_ATTR_MSG_AMR0_FILTER,DEF_AMR0_FILTER},
	{MFI_ATTR_MSG_ACR2_FILTER,NO_VAL},{MFI_ATTR_MSG_ACR3_FILTER,NO_VAL},{MFI_ATTR_MSG_AMR1_FILTER,CLOSE_AMR_FILTER},
	{MFI_ATTR_DATA_ACR0_FILTER,DEF_HOST_ACR0_FILTER},{MFI_ATTR_DATA_ACR1_FILTER,DEF_HOST_ACR1_FILTER},{MFI_ATTR_DATA_AMR0_FILTER,DEF_AMR0_FILTER},
	{MFI_ATTR_DATA_ACR2_FILTER,NO_VAL},{MFI_ATTR_DATA_ACR3_FILTER,NO_VAL},{MFI_ATTR_DATA_AMR1_FILTER,CLOSE_AMR_FILTER},
	{MFI_ATTR_MSG_RFIFO_LEN,NO_VAL},{MFI_ATTR_DATA_RFIFO_LEN,NO_VAL},{MFI_ATTR_MAX_QUEUE_LENGTH,NO_VAL},
	{MFI_ATTR_TMO_VALUE,NO_VAL},{MFI_ATTR_MODEL_BIRTHDAY,HOST_BIRTHDAY},{MFI_ATTR_MODE_MAX_CURRENT,HOST_MAX_CURRENT},
	{MFI_ATTR_MSG_FIX_TWIN_CFG,HOST_FIX_TWIN},{MFI_ATTR_DATA_FIX_TWIN_CFG,HOST_FIX_TWIN}
};

//异步IO事件的属性
//功能：异步IO事件产生时，该数组用于用于配置其所具有的属性及特定的值，用户可调用属性获取接口，获取事件属性
MfiAttribute AsyncIOEventAttr[ASYNC_EVENT_ATTR_NUM]={
	{MFI_ATTR_EVENT_TYPE,MFI_EVENT_IO_COMPLETION},{MFI_ATTR_JOB_ID,NO_VAL},{MFI_ATTR_BUFFER,NO_VAL},
	{MFI_ATTR_DATA_CLASS,NO_VAL},{MFI_ATTR_DATA_TYPE,NO_VAL},{MFI_ATTR_DATA_ADDR,NO_VAL},
	{MFI_ATTR_RET_COUNT,NO_VAL},{MFI_ATTR_STATUS,NO_VAL},{MFI_ATTR_ASYNC_TYPE,NO_VAL},
};

MfiAttribute TrigEventAttr[]={
	{MFI_ATTR_EVENT_TYPE,NO_VAL},{MFI_ATTR_RECV_TRIG_ID,NO_VAL},
};

MfiAttribute ExceptionEvent[]={
	{MFI_ATTR_EVENT_TYPE,MFI_EVENT_EXCEPTION},{MFI_ATTR_EXCEP_ID,NO_VAL},{MFI_ATTR_EXCEP_BUFFER,NO_VAL},
	{MFI_ATTR_EXCEP_BUFLEN,NO_VAL},
};

//属性初始化函数
//功能：用于初始化事件、资源等具有属性的对象的属性数组的默认值
void AttrInit(MfiPAttribute base, MfiUInt32 num)
{
	MfiInt32 i=0,index=0;
	
	for(;i<num;i++){
		index=base[i].attr_id-MFI_ATTR_BASE;
		if(attr_table[index].def_val.attr_ui==NO_VAL)
			continue;
		else
			base[i].attr_val=attr_table[index].def_val;
	}
	
	return;
}

//查找属性函数
//功能：用于获取事件、资源等具有属性的对象的某一属性在其属性数组中的索引，没有该属性则返回-1
MfiInt32 FindAttr(MfiAttr attr_id, MfiPAttribute base, MfiUInt32 num)
{
	MfiInt32 i=0;
	
	for(;i<num;i++){
		if(base[i].attr_id==attr_id)
			return i;
	}
	
	return -1;//MFI_ERROR_NSUP_ATTR
}

//释放属性函数
//功能：字符串属性为动态分配，在关闭事件或者资源时，用该函数对字符串属性进行释放
void AttrFree(MfiPAttribute base, MfiUInt32 num)
{
	MfiInt32 i=0,index=0;
	
	for(;i<num;i++){
		index=base[i].attr_id-MFI_ATTR_BASE;
		if(attr_table[index].attr_cfg.bit.type==STRINGTYPE){
			free(base[i].attr_val.attr_s);
			base[i].attr_val.attr_s=NULL;               //避免重复释放
		}
		else
			continue;
	}
	
	return;
}
//MfiInt32 CheckAttrVal(MfiAttr attrName, MfiAttrState attrValue)
//{
//	MfiAttrConfig cfg;
//	if()
//}


//属性设置函数
//功能：用于设置系统内所有属性的值，包括软件及硬件属性，并检查属性的合法性等
MfiStatus RsrcSetAttribute(MfiObject Mfi, MfiAttr attrName, MfiAttrState attrValue, MfiUInt16 refresh)
{
	MfiInt32 temp=0,attrindex=0;
	MfiAttrConfig cfg;
	MfiPAttribute attr=NULL;
	MfiUInt32 attr_num=0;
	
	if(Mfi==RsrcManager->rmsession){
		attr=RsrcManager->rsrc_attr;
		attr_num=RM_ATTR_NUM;
	}
	else{
		attr=RsrcManager->session_list[Mfi].rsrc_attr;
		attr_num=RsrcManager->session_list[Mfi].attr_amount;
	}
		
	if((attrindex=FindAttr(attrName, attr, attr_num))==-1)
		return MFI_ERROR_NSUP_ATTR;
		
	cfg=attr_table[attrName-MFI_ATTR_BASE].attr_cfg;
	
	//属性只读性检查
	if(cfg.bit.is_ro==READONLY)
		return MFI_ERROR_ATTR_READONLY;
	
	//属性设置值范围检查
	switch(cfg.bit.type)
	{
		case UINTTYPE:
			if((attrValue>=attr_table[attrName-MFI_ATTR_BASE].min_val.attr_ui)&&(attrValue<=attr_table[attrName-MFI_ATTR_BASE].max_val.attr_ui))
				break;
			else{
				temp=0;
				while((temp<ATTR_OTHER_VALUE)&&(attr_table[attrName-MFI_ATTR_BASE].other_val[temp++].attr_ui!=attrValue));
				if(temp>=ATTR_OTHER_VALUE)
					return MFI_ERROR_NSUP_ATTR_STATE;
				break;
			}
		case INTTYPE:
			if((attrValue>=attr_table[attrName-MFI_ATTR_BASE].min_val.attr_i)&&(attrValue<=attr_table[attrName-MFI_ATTR_BASE].max_val.attr_i))
				break;
			else{
				temp=0;
				while((temp<ATTR_OTHER_VALUE)&&(attr_table[attrName-MFI_ATTR_BASE].other_val[temp++].attr_i!=attrValue));
				if(temp>=ATTR_OTHER_VALUE)
					return MFI_ERROR_NSUP_ATTR_STATE;
				break;
			}
		case STRINGTYPE:
			if(attr[attrindex].attr_val.attr_s!=NULL)
				free(attr[attrindex].attr_val.attr_s);
			attr[attrindex].attr_val.attr_s=(MfiString)attrValue;
			return MFI_SUCCESS;
	}

	if(attr_table[attrName-MFI_ATTR_BASE].SetAttr==NULL)
		//该属性没有特定的设置操作，直接将设置值更新到会话中
		memcpy(&(attr[attrindex].attr_val),&(attrValue),sizeof(MfiAttrState));
	else
		attr_table[attrName-MFI_ATTR_BASE].SetAttr(Mfi,attrName,attrValue,attr,attrindex,attr_num,refresh);

/*	
	//检查是否需要将属性值更新到前端硬件
	if(cfg.bit.is_hdw==HDWARE)
	{
		if((refresh!=0)||(cfg.bit.is_rt==REALTIME))
		{
			//调用发送函数,将属性值更新到硬件
		}
	}
	
	memcpy(&(attr[attrindex].attr_val),&(attrValue),sizeof(MfiAttrState));
*/

	return MFI_SUCCESS;
}

//属性获取函数
//功能：用于获取系统内所有属性的值，包括软件及硬件属性
MfiStatus RsrcGetAttribute(MfiObject Mfi, MfiAttr attrName, void * attrValue, MfiUInt16 refresh)
{
	MfiInt32 temp=0,attrindex=0;
	MfiAttrConfig cfg;
	MfiPAttribute attr=NULL;
	MfiUInt32 attr_num=0;
	MfiPEventInfo event_info;
	MfiUInt32 msgdstaddr;
	MfiPBuf buf;
	MfiUInt32 retCnt;
	
	if(Mfi==RsrcManager->rmsession){
		attr=RsrcManager->rsrc_attr;
		attr_num=RM_ATTR_NUM;
	}
	else if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		attr=RsrcManager->session_list[Mfi].rsrc_attr;
		attr_num=RsrcManager->session_list[Mfi].attr_amount;
	}
	else{
		goto EVENT_DEAL;
	}
	
	if((attrindex=FindAttr(attrName, attr, attr_num))==-1)
		return MFI_ERROR_NSUP_ATTR;
		
	cfg=attr_table[attrName-MFI_ATTR_BASE].attr_cfg;
	
	if(cfg.bit.is_hdw==HDWARE)
	{
		if((refresh!=0)||(cfg.bit.is_rt==REALTIME))
		{
			if(RsrcManager->session_list[Mfi].rsrc_type==MODULE_RSRC_TYPE){
				//调用发送函数,将属性值获取消息发送到前端
				Mfi_to_Ip(Mfi,&msgdstaddr);
				SysWriteMsg(Mfi, MSG_GET_DEF_PORITY, MSG_GET_CLASS, attr_table[attrName-MFI_ATTR_BASE].get_command, msgdstaddr, NULL, 0, MFI_TMO_INFINITE,0);			
				//阻塞等待
				pthread_mutex_lock(&(RsrcManager->session_list[Mfi].attr_get_buf.lock));   
		  	//等待条件变量,也可用限时等待,当组帧模块组合到属性消息时,存入属性get buf,并发送条件满足信号
				pthread_cond_wait(&(RsrcManager->session_list[Mfi].attr_get_buf.ready),&(RsrcManager->session_list[Mfi].attr_get_buf.lock));
			
				//调用静态属性表中的专用函数获取更新属性到软件，若没有特殊处理需求(如关联属性)，该函数为空指针，直接更新
				CombMsgChange(RsrcManager->session_list[Mfi].attr_get_buf.buf, NULL, NULL, NULL, &buf, &retCnt);
				attr_table[attrName-MFI_ATTR_BASE].GetAttr(Mfi,attr,attr_num,buf,retCnt);
				pthread_mutex_unlock(&(RsrcManager->session_list[Mfi].attr_get_buf.lock));
				MfiCombMsgFree(buf);
			}
			else if(RsrcManager->session_list[Mfi].rsrc_type==BUS_RSRC_TYPE){
				attr_table[attrName-MFI_ATTR_BASE].GetAttr(Mfi,attr,attr_num,NULL,0);
			}
		}
	}
	
	memcpy(attrValue,&(attr[attrindex].attr_val),sizeof(MfiAttrState));
	
	return MFI_SUCCESS;
		
	EVENT_DEAL:
		event_info=Event_Find(&(RsrcManager->event_closing),Mfi);
		if(event_info==NULL)
			return MFI_ERROR_INV_OBJECT;
		
		switch(event_info->event_type)
		{
			case MFI_EVENT_IO_COMPLETION:
				attr=event_info->event_attr.async_io_event;
				attr_num=ASYNC_EVENT_ATTR_NUM;
				break;
			case MFI_EVENT_TRIG0: case MFI_EVENT_TRIG1: case MFI_EVENT_TRIG2:
			case MFI_EVENT_TRIG3: case MFI_EVENT_TRIG4: case MFI_EVENT_TRIG5:
			case MFI_EVENT_TRIG6: case MFI_EVENT_TRIG7:
				attr=event_info->event_attr.trig_event;
				attr_num=TRIG_EVENT_ATTR_NUM;
				break;
			case MFI_EVENT_EXCEPTION:
				attr=event_info->event_attr.fault_detect_event;
				attr_num=FAULT_EVENT_ATTR_NUM;
				break;							
		}
		
		if((attrindex=FindAttr(attrName, attr, attr_num))==-1)
			return MFI_ERROR_NSUP_ATTR;		
		
		memcpy(attrValue,&(attr[attrindex].attr_val),sizeof(MfiAttrState));
		
		return MFI_SUCCESS;
}

//该函数根据资源类型、厂家id、模块id生成资源字符串名
MfiStatus RsrcNameCreate(MfiString rsrcType,MfiUInt16 manfId,MfiUInt32 modId,MfiString* name)
{
	MfiString nametmp=NULL;
	MfiUInt32 len;
	
	nametmp=(MfiString)malloc(RSRC_NAME_LEN);  //分配固定长度空间存放字符串名
	if(nametmp==NULL)
		return MFI_ERROR_ALLOC;
	
	len=strlen(rsrcType);
	strcpy(nametmp,rsrcType);
	if(manfId!=0){
		sprintf(nametmp+len,"::%04x",manfId);
		len+=6;
	}
	if(modId!=0){
		sprintf(nametmp+len,"::%03x",(modId>>20));   //模块id的低20位为的模块编号，前12位为模块类型
		sprintf(nametmp+len+5,"::%05x",(modId&=(0x000FFFFF)));
	}
	nametmp[len+12]='\0';
	*name=nametmp;
	return MFI_SUCCESS;
}
