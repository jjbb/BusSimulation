#include "bus_control_datatype.h"


int m_destination_addr = 0;//Ŀ�ĵ�ַ
int m_class_big = 0;//��Ϣ����
int m_class_small = 0;//��ϢС��
int m_retry=0;

/*
*������Ϣ����������ǰ����Ϊ���ܹ���֤�ڵ���֮ǰ�����壩
*static jint m_destination_addr = 0;//Ŀ�ĵ�ַ
*static jint m_class_big = 0;//��Ϣ����
*static jint m_class_small = 0;//��ϢС��
*/
void paraseMessageType(MID_BITS struc){
	//MID_BITS temp;
	m_class_big = (int)((struc.m_class3<<3) | (struc.m_class2<<2) | (struc.m_class1<<1) | struc.m_class0);
	m_destination_addr = (int)(struc.m_dst_addr);
	m_class_small = (int)(struc.m_type);
	m_retry = (int)(struc.m_retry);
}
