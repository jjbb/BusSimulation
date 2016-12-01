#ifndef _MFI_MESSAGE_HEADER_
#define _MFI_MESSAGE_HEADER_

#include "mfi_define.h"
#include "mfiapi.h"
#include "mfi_pool.h"
//#include "mfi_rsrc_manager.h"

//----------------------------------------------------------------------------
// 消息
//----------------------------------------------------------------------------
/*消息仲裁场 32位*/
typedef struct
{
	MfiUInt32 m_single     : 1;     //判断单帧或连续： 0.单帧 1.连续
	MfiUInt32 m_src_addr   : 5;     //源地址
	MfiUInt32 m_dst_addr   : 5;     //目的地址
	MfiUInt32 m_retry      : 1;     //标记消息是否重发
	MfiUInt32 m_type       : 13;    //消息类型，留给高层协议
	MfiUInt32 m_class0     : 1;     //仪器消息大类第0位
	MfiUInt32 priority0    : 1;     //优先级第0位
	MfiUInt32 m_class1     : 1;     //仪器消息大类第1位
	MfiUInt32 priority1    : 1;     //优先级第1位
	MfiUInt32 m_class2     : 1;     //仪器消息大类第2位
	MfiUInt32 priority2    : 1;     //优先级第2位
	MfiUInt32 m_class3     : 1;     //仪器消息大类第3位
}MID_BITS;

#define Head_Len 4

typedef union
{
	MfiUInt32 all;
	MID_BITS  bit;
}MID_REG;

/*消息内容 96位*/
typedef union
{
	MfiByte   m_char[4];
	MfiUInt16 m_short[2];
	MfiUInt32 m_int;
}MTXT_REG;

/*消息结构体*/
typedef struct
{
	MID_REG  m_id;
	MTXT_REG m_txt1;
	MTXT_REG m_txt2;
	MTXT_REG m_txt3;
}MSG,*MSG_p;

/*消息缓存区*/
//发送/接收fifo
typedef struct __MSG_Fifo
{
	 MfiBoolean               empty;						  	  //FIFO空标志位
	 MfiBoolean               full;							    	//FIFO满标志位
	//  pthread_mutex_t          lock;
	//  pthread_cond_t           ready;
	 MSG_p                    buf;	                  //缓存FIFO
	 MfiUInt32                Msg_Fifo_Len;           //FIFO深度
	 MfiUInt32                rIndex;					        //当前可进行读操作的FIFO指针
	 MfiUInt32                wIndex;					        //当前可进行写操作的FIFO指针
	 MfiByte                  number[SESSION_MAX_NUM];//用于记录发送到每一个模块的消息的当前编号/用于记录不同发送源的待接收消息编号
	 MSG_p                    tmpbuf[SESSION_MAX_NUM];//用于暂存发送到每一个模块的消息；根据模块的数量分配暂存空间
}MSG_Fifo,*MSG_Fifo_p;
/*消息缓存区*/

typedef struct _CombMsg_Head
{
	struct _CombMsg_Head*     next;
	MfiUInt16                 memsize;   //内存块大小，释放时使用
	MfiUInt16                 time;      //报文保存时间
	MID_REG                   m_id;      //
	MfiByte                   freamnum;  //该报文包含帧数
	MfiByte                   startnum;  //该报文起始帧编号
	MfiByte                   cnt;       //支持重发：该报文还有几帧剩余；不支持重发：下一帧的帧位
	MfiByte                   len;       //该报文消息内容长度
}CombMsg_Head,*CombMsg_Head_p;

//用于组帧的暂存fifo
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