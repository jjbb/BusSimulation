#ifndef _BUS_CONTROL_DATATYPE_HEADER_
#define _BUS_CONTROL_DATATYPE_HEADER_

#include "mfi_module_info.h"
#include "mfi_message.h"

extern int m_destination_addr;//Ŀ�ĵ�ַ
extern int m_class_big;//��Ϣ����
extern int m_class_small;//��ϢС��
extern int m_retry;


void paraseMessageType(MID_BITS struc);


#endif