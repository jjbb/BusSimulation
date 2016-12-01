#ifndef _BUS_CONTROL_DATATYPE_HEADER_
#define _BUS_CONTROL_DATATYPE_HEADER_

#include "mfi_module_info.h"
#include "mfi_message.h"

extern int m_destination_addr;//目的地址
extern int m_class_big;//消息大类
extern int m_class_small;//消息小类
extern int m_retry;


void paraseMessageType(MID_BITS struc);


#endif