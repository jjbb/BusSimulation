#include "bus_control_datatype.h"


int m_destination_addr = 0;//目的地址
int m_class_big = 0;//消息大类
int m_class_small = 0;//消息小类
int m_retry=0;

/*
*进行消息解析（放在前面是为了能够保证在调用之前被定义）
*static jint m_destination_addr = 0;//目的地址
*static jint m_class_big = 0;//消息大类
*static jint m_class_small = 0;//消息小类
*/
void paraseMessageType(MID_BITS struc){
	//MID_BITS temp;
	m_class_big = (int)((struc.m_class3<<3) | (struc.m_class2<<2) | (struc.m_class1<<1) | struc.m_class0);
	m_destination_addr = (int)(struc.m_dst_addr);
	m_class_small = (int)(struc.m_type);
	m_retry = (int)(struc.m_retry);
}
