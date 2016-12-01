#ifndef _MFI_MESSAGE_HEADER_
#define _MFI_MESSAGE_HEADER_

#include "mfi_define.h"
#include "mfiapi.h"
#include "mfi_pool.h"
//#include "mfi_rsrc_manager.h"

//----------------------------------------------------------------------------
// ��Ϣ
//----------------------------------------------------------------------------
/*��Ϣ�ٲó� 32λ*/
typedef struct
{
	MfiUInt32 m_single     : 1;     //�жϵ�֡�������� 0.��֡ 1.����
	MfiUInt32 m_src_addr   : 5;     //Դ��ַ
	MfiUInt32 m_dst_addr   : 5;     //Ŀ�ĵ�ַ
	MfiUInt32 m_retry      : 1;     //������Ϣ�Ƿ��ط�
	MfiUInt32 m_type       : 13;    //��Ϣ���ͣ������߲�Э��
	MfiUInt32 m_class0     : 1;     //������Ϣ������0λ
	MfiUInt32 priority0    : 1;     //���ȼ���0λ
	MfiUInt32 m_class1     : 1;     //������Ϣ������1λ
	MfiUInt32 priority1    : 1;     //���ȼ���1λ
	MfiUInt32 m_class2     : 1;     //������Ϣ������2λ
	MfiUInt32 priority2    : 1;     //���ȼ���2λ
	MfiUInt32 m_class3     : 1;     //������Ϣ������3λ
}MID_BITS;

typedef union
{
	MfiUInt32 all;
	MID_BITS  bit;
}MID_REG;

/*��Ϣ���� 96λ*/
typedef union
{
	MfiByte   m_char[4];
	MfiUInt16 m_short[2];
	MfiUInt32 m_int;
}MTXT_REG;

/*��Ϣ�ṹ��*/
typedef struct
{
	MID_REG  m_id;
	MTXT_REG m_txt1;
	MTXT_REG m_txt2;
	MTXT_REG m_txt3;
}MSG,*MSG_p;

/*��Ϣ������*/
//����/����fifo
typedef struct __MSG_Fifo
{
	 MfiBoolean               empty;						  	  //FIFO�ձ�־λ
	 MfiBoolean               full;							    	//FIFO����־λ
	//  pthread_mutex_t          lock;
	//  pthread_cond_t           ready;
	 MSG_p                    buf;	                  //����FIFO
	 MfiUInt32                Msg_Fifo_Len;           //FIFO����
	 MfiUInt32                rIndex;					        //��ǰ�ɽ��ж�������FIFOָ��
	 MfiUInt32                wIndex;					        //��ǰ�ɽ���д������FIFOָ��
	 MfiByte                  number[SESSION_MAX_NUM];//���ڼ�¼���͵�ÿһ��ģ������Ϣ�ĵ�ǰ����/���ڼ�¼��ͬ����Դ�Ĵ�������Ϣ����
	 MSG_p                    tmpbuf[SESSION_MAX_NUM];//�����ݴ淢�͵�ÿһ��ģ������Ϣ������ģ�������������ݴ��ռ�
}MSG_Fifo,*MSG_Fifo_p;
/*��Ϣ������*/

typedef struct _CombMsg_Head
{
	struct _CombMsg_Head*     next;
	MfiUInt16                 memsize;   //�ڴ�����С���ͷ�ʱʹ��
	MfiUInt16                 time;      //���ı���ʱ��
	MID_REG                   m_id;      //
	MfiByte                   freamnum;  //�ñ��İ���֡��
	MfiByte                   startnum;  //�ñ�����ʼ֡����
	MfiByte                   cnt;       //֧���ط����ñ��Ļ��м�֡ʣ�ࣻ��֧���ط�����һ֡��֡λ
	MfiByte                   len;       //�ñ�����Ϣ���ݳ���
}CombMsg_Head,*CombMsg_Head_p;

//������֡���ݴ�fifo
typedef struct
{
	MfiUInt16                 queue_len;
	CombMsg_Head              head;
}CombMsg_Fifo,*CombMsg_Fifo_p;

extern MSG_Fifo msgRxFifo;
extern MSG_Fifo msgTxFifo;
extern mem_pools MsgRecPool;
extern CombMsg_Fifo msgCombFifo;
// extern pthread_mutex_t DPFreamMsg_lock;

MfiStatus MsgFifo_Init(MSG_Fifo* Msg_Fifo_p);
MfiStatus MsgFifo_Delete(MSG_Fifo* Msg_Fifo_p);
MfiStatus DecmposeFreamMsg(MfiUInt32 dst_ip, MSG_Fifo* Msg_Fifo_p, MfiUInt32 priorty, MfiUInt32 msgclass, MfiUInt32 msgtype, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 flag);
MfiStatus MsgSendToBus(MSG_Fifo* Msg_Fifo_p);
MfiStatus MsgReadFromBus(MSG_Fifo* Msg_Fifo_p);
MfiStatus CombMsg_Fifo_Init(CombMsg_Fifo_p pointer);
MfiStatus CombineFreamMsg(MfiSession Mfi, MSG_Fifo* Msg_Fifo_p, CombMsg_Fifo* CombMsg_Fifo_p);
MfiStatus MfiCombMsgFree(MfiByte* buf);
MfiStatus CombMsgChange(CombMsg_Head_p combMsg, MfiPUInt32 msgclass, MfiPUInt32 msgtype, MfiPUInt32 msgsrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt);
MfiStatus Mfi_to_Ip(MfiSession Mfi,MfiPUInt32 ip);
MfiStatus Ip_to_Mfi(MfiUInt32 ip,MfiPSession Mfi);

#endif