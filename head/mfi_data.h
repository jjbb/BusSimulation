#ifndef _MFI_DATA_HEADER_
#define _MFI_DATA_HEADER_

#include "mfi_define.h"
#include "mfiapi.h"
#include "mfi_pool.h"
//#include "mfi_rsrc_manager.h"

//----------------------------------------------------------------------------
// ����
//----------------------------------------------------------------------------
/*�����ٲó� 32λ*/
typedef struct
{
	MfiUInt32 d_single     : 1;     //�жϵ�֡�������� 0.��֡ 1.����
	MfiUInt32 d_src_addr   : 5;     //Դ��ַ
	MfiUInt32 d_dst_addr   : 5;     //Ŀ�ĵ�ַ
	MfiUInt32 d_retry      : 1;     //��������Ƿ��ط�
	MfiUInt32 d_type       : 13;    //�������ͣ������߲�Э��
	MfiUInt32 d_class0     : 1;     //�������ݴ����0λ
	MfiUInt32 priority0    : 1;     //���ȼ���0λ
	MfiUInt32 d_class1     : 1;     //�������ݴ����1λ
	MfiUInt32 priority1    : 1;     //���ȼ���1λ
	MfiUInt32 d_class2     : 1;     //�������ݴ����2λ
	MfiUInt32 priority2    : 1;     //���ȼ���2λ
	MfiUInt32 d_class3     : 1;     //�������ݴ����3λ
}DID_BITS;

typedef union
{
	MfiUInt32 all;
	DID_BITS  bit;
}DID_REG;

/*���ݽṹ��*/
typedef struct __DATA
{
	DID_REG         d_id;
	MfiByte         d_char[DATA_MAX_LEN];
	struct __DATA*  next;
	struct __DATA*  last;
}DATA,*DATA_p;

/*����fifo*/
typedef struct __DATA_Fifo
{
	 pthread_mutex_t          lock;
	 pthread_cond_t           ready; 
	 DATA                     head;	                   //����fifoͷ
	 DATA_p                   tail;	                   //����fifoβ
	 MfiUInt32                Data_Fifo_Len;           //FIFO���
	 MfiUInt32                Data_Fifo_MaxLen;        //��������
	 MfiUInt16                Tmp_Fifo_MaxLen;         //�ݴ�fifo��������
	 MfiUInt16                number[SESSION_MAX_NUM]; //���ڼ�¼���͵�ÿһ��ģ������ݵĵ�ǰ��ţ����ֵΪTmp_Fifo_MaxLen-1/���ڼ�¼��ͬ����Դ�Ĵ��������ݱ��
	 MfiUInt16                tmplen[SESSION_MAX_NUM]; //���ڼ�¼���͵�ÿһ��ģ����ݴ�fifo�ĵ�ǰ���ȣ����ֵΪTmp_Fifo_MaxLen-1
	 DATA_p                   tmpHead[SESSION_MAX_NUM];//�ݴ�fifo�Ķ�ͷָ�����飬ÿһ����Ӧһ��ģ����ݴ�fifo
	 DATA_p                   tmpTail[SESSION_MAX_NUM];//�ݴ�fifo�Ķ�βָ�����飬ÿһ����Ӧһ��ģ����ݴ�fifo
}DATA_Fifo,*DATA_Fifo_p;
/*����fifo*/

typedef struct _CombData_Head
{
	struct _CombData_Head*    next;
	MfiUInt16                 memsize;   //�ڴ���С���ͷ�ʱʹ��
	MfiUInt16                 time;      //���ı���ʱ��
	DID_REG                   d_id;      //
	MfiByte                   freamnum;  //�ñ��İ���֡��
	MfiByte                   cnt;       //�ñ��Ļ��м�֡ʣ��
	MfiUInt16                 startnum;  //�ñ�����ʼ֡���
	MfiUInt16                 len;       //�ñ������ݳ���
}CombData_Head,*CombData_Head_p;

typedef struct
{
	MfiUInt16                 queue_len;
	CombData_Head             head;
}CombData_Fifo,*CombData_Fifo_p;

extern DATA_Fifo dataRxFifo;
extern DATA_Fifo dataTxFifo;
extern mem_pools DataSendPool;
extern mem_pools DataRecPool;
extern CombData_Fifo dataCombFifo;
extern pthread_mutex_t DPFreamData_lock;

MfiStatus DataFifo_Init(DATA_Fifo* Data_Fifo_p);
MfiStatus DataFifo_Delete(DATA_Fifo* Data_Fifo_p);
MfiStatus DecmposeFreamData(MfiUInt32 dst_ip, DATA_Fifo* Data_Fifo_p, MfiUInt32 priorty, MfiUInt32 dataclass, MfiUInt32 datatype, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 flag);
MfiStatus DataSendToBus(DATA_Fifo* Data_Fifo_p);
MfiStatus DataReadFromBus(DATA_Fifo* DATA_Fifo_p);
MfiStatus CombData_Fifo_Init(CombData_Fifo_p pointer);
MfiStatus CombineFreamData(MfiSession Mfi, DATA_Fifo* Data_Fifo_p, CombData_Fifo* CombData_Fifo_p);
MfiStatus MfiCombDataFree(MfiByte* buf);
MfiStatus CombDataChange(CombData_Head_p combData, MfiPUInt32 dataclass, MfiPUInt32 datatype, MfiPUInt32 datasrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt);

#endif