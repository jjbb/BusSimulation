#ifndef _MFI_IO_HEAD_
#define _MFI_IO_HEAD_

#include "mfi_define.h"
#include "mfiapi.h"

typedef struct _MfiAsyncTaskInfo{
	MfiJobId                     job_id;
	MfiUInt32                    job_type;
	MfiPBuf                      job_buf;
	MfiUInt32                    buf_size;
	MfiUInt32                    priorty;
	MfiUInt32                    mdclass;
	MfiUInt32                    type;
	MfiUInt32                    addr;
	struct _MfiAsyncTaskInfo*    next;
	struct _MfiAsyncTaskInfo*    last;
}MfiAsyncTaskInfo,*MfiPAsyncTaskInfo;

typedef struct{
	MfiUInt32                    queue_len;
	MfiAsyncTaskInfo             queue_head;
	MfiPAsyncTaskInfo            queue_tail;
}MfiAsyncTaskQueue;

typedef struct{
	MfiAsyncTaskQueue            msg_task_queue;
	MfiAsyncTaskQueue            data_task_queue;
	pthread_cond_t               ready;
	pthread_mutex_t              lock;                         //¶ÓÁÐËø
}MfiAsyncQueue,*MfiPAsyncQueue;

MfiStatus AsyncQueueInit(MfiPAsyncQueue queue);
MfiStatus AsyncQueueDelete(MfiPAsyncQueue queue);
MfiStatus ForCloseAsyncWtThr(MfiSession Mfi);
MfiStatus ForCloseAsyncRdThr(MfiSession Mfi);

MfiStatus RsrcSessionReadMsg(MfiSession Mfi, MfiPUInt32 msgtype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiUInt32 timeout);
MfiStatus RsrcSessionReadData(MfiSession Mfi, MfiPUInt32 datatype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiUInt32 timeout);
MfiStatus RsrcSessionWriteMsg(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgtype, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 timeout, MfiUInt32 sendtype);
MfiStatus RsrcSessionWriteData(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 datatype, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 timeout, MfiUInt32 sendtype);
MfiStatus ReadMsgAsync(MfiSession Mfi, MfiPUInt32 msgtype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId);
MfiStatus WriteMsgAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgtype, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId);
MfiStatus WriteDataAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 datatype, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId);
MfiStatus ReadDataAsync(MfiSession Mfi, MfiPUInt32 datatype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId);
void* thr_asyncread(void* arg);
void* thr_asyncwrite(void* arg);
MfiStatus SysReadMsg(MfiSession Mfi, MfiPUInt32 msgclass, MfiPUInt32 msgtype, MfiPUInt32 msgsrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiUInt32 timeout);
MfiStatus SysReadData(MfiSession Mfi, MfiPUInt32 dataclass, MfiPUInt32 datatype, MfiPUInt32 datasrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiUInt32 timeout);
MfiStatus SysWriteMsg(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgclass, MfiUInt32 msgtype, MfiUInt32 msgdstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 timeout, MfiUInt32 sendtype);
MfiStatus SysWriteData(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 dataclass, MfiUInt32 datatype, MfiUInt32 datadstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 timeout, MfiUInt32 sendtype);

MfiStatus SysReadMsgAsync(MfiSession Mfi, MfiPUInt32 msgclass, MfiPUInt32 msgtype, MfiPUInt32 msgsrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId);
MfiStatus SysWriteMsgAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgclass, MfiUInt32 msgtype, MfiUInt32 msgdstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId);
MfiStatus SysReadDataAsync(MfiSession Mfi, MfiPUInt32 dataclass, MfiPUInt32 datatype, MfiPUInt32 datasrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId);
MfiStatus SysWriteDataAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 dataclass, MfiUInt32 datatype, MfiUInt32 datadstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId);

#endif 