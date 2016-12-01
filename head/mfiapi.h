#ifndef _MFI_API_HEADER_
#define _MFI_API_HEADER_

#if !defined(_MFITYPE_HEADER_)
#include "mfitype.h"
#endif

#define MFI_SPEC_VERSION     (0x00100000UL)

#if defined(__cplusplus) || defined(__cplusplus__)
   extern "C" {
#endif

/*- MFI Types --------------------------------------------------------------*/

typedef MfiObject             MfiEvent;    //事件ID
typedef MfiEvent *            MfiPEvent;
typedef MfiObject             MfiFindList;  //资源查找链表ID
typedef MfiFindList *         MfiPFindList;

typedef MfiUInt32             MfiAttrState; //属性值

typedef MfiUInt32             MfiEventType; //事件类型
typedef MfiEventType *        MfiPEventType;
typedef MfiEventType *        MfiAEventType;
typedef void         *        MfiPAttrState; //属性值指针
typedef MfiAttr *             MfiPAttr;      //属性ID
typedef MfiAttr *             MfiAAttr;

//typedef MfiString             MfiKeyId;
//typedef MfiPString            MfiPKeyId;
typedef MfiUInt32             MfiJobId;     //异步任务ID
typedef MfiJobId *            MfiPJobId;
//typedef MfiUInt32             MfiAccessMode;
//typedef MfiAccessMode *       MfiPAccessMode;
//typedef MfiBusAddress *       MfiPBusAddress;
typedef MfiUInt32             MfiEventFilter;

//typedef va_list              MfiVAList;

typedef MfiStatus (* MfiHndlr)
   (MfiSession Mfi, MfiEventType eventType, MfiEvent event, MfiAddr userHandle); //定义事件回调函数原型
   
/*- Resource Manager Functions and Operations -------------------------------*/

MfiStatus   MfiOpenDefaultRM (MfiPSession Mfi);   //打开资源管理器
MfiStatus   MfiFindRsrc      (MfiSession sesn, MfiString expr, MfiPFindList Mfi,
                                    MfiPUInt32 retCnt, MfiChar desc[]);  //查找资源
MfiStatus   MfiFindNext      (MfiFindList Mfi, MfiChar desc[]); //返回符合条件的下一个资源
MfiStatus   MfiParseRsrc     (MfiSession rmSesn, MfiRsrc rsrcName, //解析资源获取接口类型和接口编号
                                    MfiPUInt16 intfType, MfiPUInt16 intfNum);
//MfiStatus   MfiParseRsrcEx   (MfiSession rmSesn, MfiRsrc rsrcName, MfiPUInt16 intfType,
//                                    MfiPUInt16 intfNum, MfiChar rsrcClass[],
//                                    MfiChar expandedUnaliasedName[],
//                                    MfiChar aliasIfExists[]);
MfiStatus   MfiOpen          (MfiSession sesn, MfiRsrc name, MfiPSession Mfi); //打开资源，创建会话

/*- Resource Template Operations --------------------------------------------*/

MfiStatus   MfiClose         (MfiObject Mfi); //关闭会话、事件、资源管理器
MfiStatus   MfiSetAttribute  (MfiObject Mfi, MfiAttr attrName, MfiAttrState attrValue, MfiUInt16 refresh);//设置属性
MfiStatus   MfiGetAttribute  (MfiObject Mfi, MfiAttr attrName, void * attrValue, MfiUInt16 refresh);//获取属性
MfiStatus   MfiStatusDesc    (MfiObject Mfi, MfiStatus status, MfiChar desc[]);//状态码转换为可读字符串
MfiStatus   MfiTerminate     (MfiObject Mfi, MfiUInt16 degree, MfiJobId jobId);//中止异步任务

/*
MfiStatus   MfiLock          (MfiSession Mfi, MfiAccessMode lockType, MfiUInt32 timeout,
                                    MfiKeyId requestedKey, MfiChar  accessKey[]);
MfiStatus   MfiUnlock        (MfiSession Mfi);
*/
MfiStatus   MfiEnableEvent   (MfiSession Mfi, MfiEventType eventType, MfiUInt16 mechanism); //事件使能，设置处理机制
MfiStatus   MfiDisableEvent  (MfiSession Mfi, MfiEventType eventType); //失能事件
MfiStatus   MfiDiscardEvents (MfiSession Mfi, MfiEventType eventType, MfiUInt16 mechanism); //丢弃已发生事件
MfiStatus   MfiWaitOnEvent   (MfiSession Mfi, MfiEventType inEventType, MfiUInt32 timeout,
                                    MfiPEventType outEventType, MfiPEvent outContext); //等待某个事件发生
MfiStatus   MfiInstallHandler(MfiSession Mfi, MfiEventType eventType, MfiHndlr handler,
                                    MfiAddr userHandle); //安装事件回调处理函数
MfiStatus   MfiUninstallHandler(MfiSession Mfi, MfiEventType eventType, MfiHndlr handler,
                                      MfiAddr userHandle); //注销回调处理函数

/*- Basic I/O Operations ----------------------------------------------------*/

MfiStatus   MfiReadMsg       (MfiSession Mfi, MfiPUInt32 msgtype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiUInt32 timeout); //同步读
MfiStatus   MfiReadMsgAsync  (MfiSession Mfi, MfiPUInt32 msgtype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId  jobId); //异步读
MfiStatus   MfiReadData      (MfiSession Mfi, MfiPUInt32 datatype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiUInt32 timeout); //同步读
MfiStatus   MfiReadDataAsync (MfiSession Mfi, MfiPUInt32 datatype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId  jobId); //异步读
//MfiStatus   MfiReadToFile    (MfiSession Mfi, MfiConstString filename, MfiUInt32 cnt, 
 //                                   MfiPUInt32 retCnt); //读到文件中
MfiStatus   MfiWriteMsg      (MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgtype, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 timeout, MfiUInt32 sendtype); //同步写
MfiStatus   MfiWriteMsgAsync (MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgtype, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId); //异步写
MfiStatus 	MfiWriteDataAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 datatype, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId);
MfiStatus   MfiWriteData		 (MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 datatype, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 timeout, MfiUInt32 sendtype);
//MfiStatus   MfiWriteFromFile (MfiSession Mfi, MfiConstString filename, MfiUInt32 cnt,
//                                    MfiPUInt32 retCnt); //从文件中读出
MfiStatus   MfiAssertTrigger (MfiSession Mfi, MfiInt16 trigline); //主机主动触发触发线
MfiStatus   MfiConfigTrigger (MfiSession Mfi, MfiInt16 trigline, MfiUInt16 mode, MfiUInt32 condition ,MfiPUInt16 oldmode, MfiPUInt32 oldcondition);
MfiStatus   MfiDeleteTrigger (MfiSession Mfi, MfiInt16 trigline);
//MfiStatus   MfiReadSTB       (MfiSession Mfi, MfiPUInt16 status);
MfiStatus   MfiClear         (MfiSession Mfi); //清空前端设备的总线驱动buffer
MfiStatus   MfiSysReadMsg(MfiSession Mfi, MfiPUInt32 msgclass, MfiPUInt32 msgtype, MfiPUInt32 msgsrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiUInt32 timeout);
MfiStatus   MfiSysReadData(MfiSession Mfi, MfiPUInt32 dataclass, MfiPUInt32 datatype, MfiPUInt32 datasrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiUInt32 timeout);
MfiStatus   MfiSysWriteMsg(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgclass, MfiUInt32 msgtype, MfiUInt32 msgdstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 timeout, MfiUInt32 sendtype);
MfiStatus   MfiSysWriteData(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 dataclass, MfiUInt32 datatype, MfiUInt32 datadstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 timeout, MfiUInt32 sendtype);

MfiStatus   MfiSysReadMsgAsync(MfiSession Mfi, MfiPUInt32 msgclass, MfiPUInt32 msgtype, MfiPUInt32 msgsrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId);
MfiStatus   MfiSysWriteMsgAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgclass, MfiUInt32 msgtype, MfiUInt32 msgdstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId);
MfiStatus   MfiSysReadDataAsync(MfiSession Mfi, MfiPUInt32 dataclass, MfiPUInt32 datatype, MfiPUInt32 datasrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId);
MfiStatus   MfiSysWriteDataAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 dataclass, MfiUInt32 datatype, MfiUInt32 datadstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId);


/*- Attributes (platform independent size) ----------------------------------*/
#define MFI_ATTR_BASE                (0x3FFF0001UL)
#define MFI_ATTR_RSRC_CLASS          (0x3FFF0001UL)        //资源类型
#define MFI_ATTR_RSRC_NAME           (0x3FFF0002UL)        //资源字符串
#define MFI_ATTR_RSRC_IMPL_VERSION   (0x3FFF0003UL)        //资源版本(仪器驱动版本)
#define MFI_ATTR_RSRC_MANF_NAME      (0x3FFF0004UL)        //资源驱动的厂家名
#define MFI_ATTR_RSRC_MANF_ID        (0x3FFF0005UL)        //资源驱动的厂家ID
#define MFI_ATTR_RSRC_SPEC_VERSION   (0x3FFF0006UL)        //VISA版本
#define MFI_ATTR_RM_SESSION          (0x3FFF0007UL)        //资源管理器会话ID
#define MFI_ATTR_RSRC_INFO           (0x3FFF0008UL)        //前端仪器模块身份识别信息缓存
#define MFI_ATTR_RSRC_NUM            (0x3FFF0009UL)        //资源数量
#define MFI_ATTR_MANF_ID             (0x3FFF000AUL)        //硬件模块厂家ID
#define MFI_ATTR_MANF_NAME           (0x3FFF000BUL)        //硬件模块厂家名
#define MFI_ATTR_MODEL_CODE          (0x3FFF000CUL)        //硬件模块ID
#define MFI_ATTR_MODEL_NAME          (0x3FFF000DUL)        //硬件模块名
#define MFI_ATTR_MODEL_LA            (0x3FFF000EUL)        //模块的逻辑地址
#define MFI_ATTR_MAX_QUEUE_LENGTH    (0x3FFF000FUL)        //最大队列长度
#define MFI_ATTR_TMO_VALUE           (0x3FFF0010UL)        //超时值
#define MFI_ATTR_USER_DATA           (0x3FFF0011UL)        //用户私有数据(32位)

#define MFI_ATTR_JOB_ID              (0x3FFF0012UL)        //异步任务ID
#define MFI_ATTR_EVENT_TYPE          (0x3FFF0013UL)        //事件类型
#define MFI_ATTR_RECV_TRIG_ID        (0x3FFF0014UL)        //触发线ID
#define MFI_ATTR_STATUS              (0x3FFF0015UL)        //产生事件的操作的返回状态
#define MFI_ATTR_RET_COUNT           (0x3FFF0016UL)        //异步IO实际传输的字节数
#define MFI_ATTR_BUFFER              (0x3FFF0017UL)        //异步IO缓冲区
#define MFI_ATTR_OPER_NAME           (0x3FFF0018UL)        //产生事件的操作名
#define MFI_ATTR_EXCEP_ID            (0x3FFF0019UL)        //异常ID
#define MFI_ATTR_EXCEP_BUFFER        (0x3FFF001AUL)        //异常信息缓冲区
#define MFI_ATTR_EXCEP_BUFLEN        (0x3FFF001BUL)        //异常信息缓冲区长度
 
#define MFI_ATTR_MSG_BASIC_TLEN      (0x3FFF001CUL)        //消息基本周期
#define MFI_ATTR_MSG_FIX_TLEN        (0x3FFF001DUL)        //固定时间窗长度
#define MFI_ATTR_DATA_BASIC_TLEN     (0x3FFF001EUL)        //数据基本周期
#define MFI_ATTR_DATA_FIX_TLEN       (0x3FFF001FUL)        //固定时间窗长度
#define MFI_ATTR_MSG_ACR0_FILTER     (0x3FFF0020UL)        //消息滤波器
#define MFI_ATTR_MSG_ACR1_FILTER     (0x3FFF0021UL)
#define MFI_ATTR_MSG_AMR0_FILTER     (0x3FFF0022UL)
#define MFI_ATTR_MSG_ACR2_FILTER     (0x3FFF0023UL)
#define MFI_ATTR_MSG_ACR3_FILTER     (0x3FFF0024UL)
#define MFI_ATTR_MSG_AMR1_FILTER     (0x3FFF0025UL)
#define MFI_ATTR_DATA_ACR0_FILTER    (0x3FFF0026UL)          //数据滤波器
#define MFI_ATTR_DATA_ACR1_FILTER    (0x3FFF0027UL)
#define MFI_ATTR_DATA_AMR0_FILTER    (0x3FFF0028UL)
#define MFI_ATTR_DATA_ACR2_FILTER    (0x3FFF0029UL)
#define MFI_ATTR_DATA_ACR3_FILTER    (0x3FFF002AUL)
#define MFI_ATTR_DATA_AMR1_FILTER    (0x3FFF002BUL)
#define MFI_ATTR_MSG_RFIFO_LEN       (0x3FFF002CUL)         //会话中的消息读FIFO长度
#define MFI_ATTR_DATA_RFIFO_LEN      (0x3FFF002DUL)         //会话中的数据读FIFO长度
#define MFI_ATTR_DATA_TYPE           (0x3FFF002EUL)         //异步IO消息/数据类型
#define MFI_ATTR_DATA_CLASS          (0x3FFF002FUL)         //异步IO消息/数据大类
#define MFI_ATTR_DATA_ADDR           (0x3FFF0030UL)         //异步IO消息/数据地址
#define MFI_ATTR_MODEL_BIRTHDAY      (0x3FFF0031UL)         //前端模块生产日期
#define MFI_ATTR_MODE_MAX_CURRENT    (0x3FFF0032UL)         //前端模块的最大电流
#define MFI_ATTR_MSG_FIX_TWIN_CFG    (0x3FFF0033UL)         //消息固定时间窗选择配置
#define MFI_ATTR_DATA_FIX_TWIN_CFG   (0x3FFF0034UL)         //数据固定时间窗选择配置
#define MFI_ATTR_ASYNC_TYPE          (0x3FFF0035UL)         //异步IO完成事件的类型

/*- Event Types -------------------------------------------------------------*/

#define MFI_EVENT_IO_COMPLETION      (0x3FFF2000UL)
#define MFI_EVENT_TRIG0              (0x3FFF2001UL)
#define MFI_EVENT_TRIG1              (0x3FFF2002UL)
#define MFI_EVENT_TRIG2              (0x3FFF2003UL)
#define MFI_EVENT_TRIG3              (0x3FFF2004UL)
#define MFI_EVENT_TRIG4              (0x3FFF2005UL)
#define MFI_EVENT_TRIG5              (0x3FFF2006UL)
#define MFI_EVENT_TRIG6              (0x3FFF2007UL)
#define MFI_EVENT_TRIG7              (0x3FFF2008UL)
#define MFI_EVENT_EXCEPTION          (0x3FFF2009UL)

#define MFI_ALL_ENABLED_EVENTS       (0x3FFF7FFFUL)

/*- Completion and Error Codes ----------------------------------------------*/

#define MFI_SUCCESS_EVENT_EN                   (0x3FFF0002L) /* 3FFF0002,  1073676290 */
#define MFI_SUCCESS_EVENT_DIS                  (0x3FFF0003L) /* 3FFF0003,  1073676291 */
#define MFI_SUCCESS_QUEUE_EMPTY                (0x3FFF0004L) /* 3FFF0004,  1073676292 */
#define MFI_SUCCESS_TERM_CHAR                  (0x3FFF0005L) /* 3FFF0005,  1073676293 */
#define MFI_SUCCESS_MAX_CNT                    (0x3FFF0006L) /* 3FFF0006,  1073676294 */
#define MFI_SUCCESS_DEV_NPRESENT               (0x3FFF007DL) /* 3FFF007D,  1073676413 */
#define MFI_SUCCESS_TRIG_MAPPED                (0x3FFF007EL) /* 3FFF007E,  1073676414 */
#define MFI_SUCCESS_QUEUE_NEMPTY               (0x3FFF0080L) /* 3FFF0080,  1073676416 */
#define MFI_SUCCESS_NCHAIN                     (0x3FFF0098L) /* 3FFF0098,  1073676440 */
#define MFI_SUCCESS_NESTED_SHARED              (0x3FFF0099L) /* 3FFF0099,  1073676441 */
#define MFI_SUCCESS_NESTED_EXCLUSIVE           (0x3FFF009AL) /* 3FFF009A,  1073676442 */
#define MFI_SUCCESS_SYNC                       (0x3FFF009BL) /* 3FFF009B,  1073676443 */
#define MFI_SUCCESS_HNDLR_REP                  (0x3FFF009CL) /* 3FFF009C,  1073676443 */
#define MFI_SUCCESS_TRIG_DELE                  (0x3FFF009DL) /* 3FFF009D, -1073807191 */
#define MFI_SUCCESS_TRIG_SET                   (0x3FFF009EL) /* 3FFF009E, -1073807191 */

#define MFI_WARN_QUEUE_OVERFLOW                (0x3FFF000CL) /* 3FFF000C,  1073676300 */
#define MFI_WARN_CONFIG_NLOADED                (0x3FFF0077L) /* 3FFF0077,  1073676407 */
#define MFI_WARN_NULL_OBJECT                   (0x3FFF0082L) /* 3FFF0082,  1073676418 */
#define MFI_WARN_NSUP_ATTR_STATE               (0x3FFF0084L) /* 3FFF0084,  1073676420 */
#define MFI_WARN_UNKNOWN_STATUS                (0x3FFF0085L) /* 3FFF0085,  1073676421 */
#define MFI_WARN_NSUP_BUF                      (0x3FFF0088L) /* 3FFF0088,  1073676424 */
#define MFI_WARN_EXT_FUNC_NIMPL                (0x3FFF00A9L) /* 3FFF00A9,  1073676457 */

#define MFI_ERROR_SYSTEM_ERROR       (_MFI_ERROR+0x3FFF0000L) /* BFFF0000, -1073807360 */
#define MFI_ERROR_INV_OBJECT         (_MFI_ERROR+0x3FFF000EL) /* BFFF000E, -1073807346 */
#define MFI_ERROR_RSRC_LOCKED        (_MFI_ERROR+0x3FFF000FL) /* BFFF000F, -1073807345 */
#define MFI_ERROR_INV_EXPR           (_MFI_ERROR+0x3FFF0010L) /* BFFF0010, -1073807344 */
#define MFI_ERROR_RSRC_NFOUND        (_MFI_ERROR+0x3FFF0011L) /* BFFF0011, -1073807343 */
#define MFI_ERROR_INV_RSRC_NAME      (_MFI_ERROR+0x3FFF0012L) /* BFFF0012, -1073807342 */
#define MFI_ERROR_INV_ACC_MODE       (_MFI_ERROR+0x3FFF0013L) /* BFFF0013, -1073807341 */
#define MFI_ERROR_TMO                (_MFI_ERROR+0x3FFF0015L) /* BFFF0015, -1073807339 */
#define MFI_ERROR_CLOSING_FAILED     (_MFI_ERROR+0x3FFF0016L) /* BFFF0016, -1073807338 */
#define MFI_ERROR_INV_DEGREE         (_MFI_ERROR+0x3FFF001BL) /* BFFF001B, -1073807333 */
#define MFI_ERROR_INV_JOB_ID         (_MFI_ERROR+0x3FFF001CL) /* BFFF001C, -1073807332 */
#define MFI_ERROR_NSUP_ATTR          (_MFI_ERROR+0x3FFF001DL) /* BFFF001D, -1073807331 */
#define MFI_ERROR_NSUP_ATTR_STATE    (_MFI_ERROR+0x3FFF001EL) /* BFFF001E, -1073807330 */
#define MFI_ERROR_ATTR_READONLY      (_MFI_ERROR+0x3FFF001FL) /* BFFF001F, -1073807329 */
#define MFI_ERROR_INV_LOCK_TYPE      (_MFI_ERROR+0x3FFF0020L) /* BFFF0020, -1073807328 */
#define MFI_ERROR_INV_ACCESS_KEY     (_MFI_ERROR+0x3FFF0021L) /* BFFF0021, -1073807327 */
#define MFI_ERROR_INV_EVENT          (_MFI_ERROR+0x3FFF0026L) /* BFFF0026, -1073807322 */
#define MFI_ERROR_INV_MECH           (_MFI_ERROR+0x3FFF0027L) /* BFFF0027, -1073807321 */
#define MFI_ERROR_HNDLR_NINSTALLED   (_MFI_ERROR+0x3FFF0028L) /* BFFF0028, -1073807320 */
#define MFI_ERROR_INV_HNDLR_REF      (_MFI_ERROR+0x3FFF0029L) /* BFFF0029, -1073807319 */
#define MFI_ERROR_INV_CONTEXT        (_MFI_ERROR+0x3FFF002AL) /* BFFF002A, -1073807318 */
#define MFI_ERROR_QUEUE_OVERFLOW     (_MFI_ERROR+0x3FFF002DL) /* BFFF002D, -1073807315 */
#define MFI_ERROR_NENABLED           (_MFI_ERROR+0x3FFF002FL) /* BFFF002F, -1073807313 */
#define MFI_ERROR_ABORT              (_MFI_ERROR+0x3FFF0030L) /* BFFF0030, -1073807312 */
#define MFI_ERROR_RAW_WR_PROT_MFIOL   (_MFI_ERROR+0x3FFF0034L) /* BFFF0034, -1073807308 */
#define MFI_ERROR_RAW_RD_PROT_MFIOL   (_MFI_ERROR+0x3FFF0035L) /* BFFF0035, -1073807307 */
#define MFI_ERROR_OUTP_PROT_MFIOL     (_MFI_ERROR+0x3FFF0036L) /* BFFF0036, -1073807306 */
#define MFI_ERROR_INP_PROT_MFIOL      (_MFI_ERROR+0x3FFF0037L) /* BFFF0037, -1073807305 */
#define MFI_ERROR_BERR               (_MFI_ERROR+0x3FFF0038L) /* BFFF0038, -1073807304 */
#define MFI_ERROR_IN_PROGRESS        (_MFI_ERROR+0x3FFF0039L) /* BFFF0039, -1073807303 */
#define MFI_ERROR_INV_SETUP          (_MFI_ERROR+0x3FFF003AL) /* BFFF003A, -1073807302 */
#define MFI_ERROR_QUEUE_ERROR        (_MFI_ERROR+0x3FFF003BL) /* BFFF003B, -1073807301 */
#define MFI_ERROR_ALLOC              (_MFI_ERROR+0x3FFF003CL) /* BFFF003C, -1073807300 */
#define MFI_ERROR_INV_MASK           (_MFI_ERROR+0x3FFF003DL) /* BFFF003D, -1073807299 */
#define MFI_ERROR_IO                 (_MFI_ERROR+0x3FFF003EL) /* BFFF003E, -1073807298 */
#define MFI_ERROR_INV_FMT            (_MFI_ERROR+0x3FFF003FL) /* BFFF003F, -1073807297 */
#define MFI_ERROR_NSUP_FMT           (_MFI_ERROR+0x3FFF0041L) /* BFFF0041, -1073807295 */
#define MFI_ERROR_LINE_IN_USE        (_MFI_ERROR+0x3FFF0042L) /* BFFF0042, -1073807294 */
#define MFI_ERROR_NSUP_MODE          (_MFI_ERROR+0x3FFF0046L) /* BFFF0046, -1073807290 */
#define MFI_ERROR_SRQ_NOCCURRED      (_MFI_ERROR+0x3FFF004AL) /* BFFF004A, -1073807286 */
#define MFI_ERROR_INV_SPACE          (_MFI_ERROR+0x3FFF004EL) /* BFFF004E, -1073807282 */
#define MFI_ERROR_INV_OFFSET         (_MFI_ERROR+0x3FFF0051L) /* BFFF0051, -1073807279 */
#define MFI_ERROR_INV_WIDTH          (_MFI_ERROR+0x3FFF0052L) /* BFFF0052, -1073807278 */
#define MFI_ERROR_NSUP_OFFSET        (_MFI_ERROR+0x3FFF0054L) /* BFFF0054, -1073807276 */
#define MFI_ERROR_NSUP_VAR_WIDTH     (_MFI_ERROR+0x3FFF0055L) /* BFFF0055, -1073807275 */
#define MFI_ERROR_WINDOW_NMAPPED     (_MFI_ERROR+0x3FFF0057L) /* BFFF0057, -1073807273 */
#define MFI_ERROR_RESP_PENDING       (_MFI_ERROR+0x3FFF0059L) /* BFFF0059, -1073807271 */
#define MFI_ERROR_NLISTENERS         (_MFI_ERROR+0x3FFF005FL) /* BFFF005F, -1073807265 */
#define MFI_ERROR_NCIC               (_MFI_ERROR+0x3FFF0060L) /* BFFF0060, -1073807264 */
#define MFI_ERROR_NSYS_CNTLR         (_MFI_ERROR+0x3FFF0061L) /* BFFF0061, -1073807263 */
#define MFI_ERROR_NSUP_OPER          (_MFI_ERROR+0x3FFF0067L) /* BFFF0067, -1073807257 */
#define MFI_ERROR_INTR_PENDING       (_MFI_ERROR+0x3FFF0068L) /* BFFF0068, -1073807256 */
#define MFI_ERROR_ASRL_PARITY        (_MFI_ERROR+0x3FFF006AL) /* BFFF006A, -1073807254 */
#define MFI_ERROR_ASRL_FRAMING       (_MFI_ERROR+0x3FFF006BL) /* BFFF006B, -1073807253 */
#define MFI_ERROR_ASRL_OVERRUN       (_MFI_ERROR+0x3FFF006CL) /* BFFF006C, -1073807252 */
#define MFI_ERROR_TRIG_NMAPPED       (_MFI_ERROR+0x3FFF006EL) /* BFFF006E, -1073807250 */
#define MFI_ERROR_NSUP_ALIGN_OFFSET  (_MFI_ERROR+0x3FFF0070L) /* BFFF0070, -1073807248 */
#define MFI_ERROR_USER_BUF           (_MFI_ERROR+0x3FFF0071L) /* BFFF0071, -1073807247 */
#define MFI_ERROR_RSRC_BUSY          (_MFI_ERROR+0x3FFF0072L) /* BFFF0072, -1073807246 */
#define MFI_ERROR_NSUP_WIDTH         (_MFI_ERROR+0x3FFF0076L) /* BFFF0076, -1073807242 */
#define MFI_ERROR_INV_PARAMETER      (_MFI_ERROR+0x3FFF0078L) /* BFFF0078, -1073807240 */
#define MFI_ERROR_INV_PROT           (_MFI_ERROR+0x3FFF0079L) /* BFFF0079, -1073807239 */
#define MFI_ERROR_INV_SIZE           (_MFI_ERROR+0x3FFF007BL) /* BFFF007B, -1073807237 */
#define MFI_ERROR_WINDOW_MAPPED      (_MFI_ERROR+0x3FFF0080L) /* BFFF0080, -1073807232 */
#define MFI_ERROR_NIMPL_OPER         (_MFI_ERROR+0x3FFF0081L) /* BFFF0081, -1073807231 */
#define MFI_ERROR_INV_LENGTH         (_MFI_ERROR+0x3FFF0083L) /* BFFF0083, -1073807229 */
#define MFI_ERROR_INV_MODE           (_MFI_ERROR+0x3FFF0091L) /* BFFF0091, -1073807215 */
#define MFI_ERROR_SESN_NLOCKED       (_MFI_ERROR+0x3FFF009CL) /* BFFF009C, -1073807204 */
#define MFI_ERROR_MEM_NSHARED        (_MFI_ERROR+0x3FFF009DL) /* BFFF009D, -1073807203 */
#define MFI_ERROR_LIBRARY_NFOUND     (_MFI_ERROR+0x3FFF009EL) /* BFFF009E, -1073807202 */
#define MFI_ERROR_NSUP_INTR          (_MFI_ERROR+0x3FFF009FL) /* BFFF009F, -1073807201 */
#define MFI_ERROR_INV_LINE           (_MFI_ERROR+0x3FFF00A0L) /* BFFF00A0, -1073807200 */
#define MFI_ERROR_FILE_ACCESS        (_MFI_ERROR+0x3FFF00A1L) /* BFFF00A1, -1073807199 */
#define MFI_ERROR_FILE_IO            (_MFI_ERROR+0x3FFF00A2L) /* BFFF00A2, -1073807198 */
#define MFI_ERROR_NSUP_LINE          (_MFI_ERROR+0x3FFF00A3L) /* BFFF00A3, -1073807197 */
#define MFI_ERROR_NSUP_MECH          (_MFI_ERROR+0x3FFF00A4L) /* BFFF00A4, -1073807196 */
#define MFI_ERROR_INTF_NUM_NCONFIG   (_MFI_ERROR+0x3FFF00A5L) /* BFFF00A5, -1073807195 */
#define MFI_ERROR_CONN_LOST          (_MFI_ERROR+0x3FFF00A6L) /* BFFF00A6, -1073807194 */
#define MFI_ERROR_MACHINE_NAVAIL     (_MFI_ERROR+0x3FFF00A7L) /* BFFF00A7, -1073807193 */
#define MFI_ERROR_NPERMISSION        (_MFI_ERROR+0x3FFF00A8L) /* BFFF00A8, -1073807192 */
#define MFI_ERROR_SESN_NUM           (_MFI_ERROR+0x3FFF00A9L) /* BFFF00A9, -1073807191 */
#define MFI_ERROR_NO_DATA            (_MFI_ERROR+0x3FFF00AAL) /* BFFF00AA, -1073807191 */
#define MFI_ERROR_BUF_FULL           (_MFI_ERROR+0x3FFF00ABL) /* BFFF00AB, -1073807191 */
#define MFI_ERROR_ASYNC_THR_FAIL     (_MFI_ERROR+0x3FFF00ACL) /* BFFF00AC, -1073807191 */
#define MFI_ERROR_NCREAT_MANAGER     (_MFI_ERROR+0x3FFF00ADL) /* BFFF00AD, -1073807191 */
#define MFI_ERROR_BUF_EMPTY          (_MFI_ERROR+0x3FFF00AEL) /* BFFF00AE, -1073807191 */
#define MFI_ERROR_OPEN_RSRC          (_MFI_ERROR+0x3FFF00AFL) /* BFFF00AF, -1073807191 */

/*- Other MFI Definitions --------------------------------------------------*/

#define MFI_VERSION_MAJOR(ver)       ((((MfiVersion)ver) & 0xFFF00000UL) >> 20)
#define MFI_VERSION_MINOR(ver)       ((((MfiVersion)ver) & 0x000FFF00UL) >>  8)
#define MFI_VERSION_SUBMINOR(ver)    ((((MfiVersion)ver) & 0x000000FFUL)      )

#define MFI_TMO_IMMEDIATE            (0L)
#define MFI_TMO_INFINITE             (0xFFFFFFFFUL)
#define MFI_TMO_USEATTRVAL           (0xFFFFFFFEUL)

#define MFI_TRIG_ALL                 (-2)
#define MFI_TRIG_TTL0                (0)
#define MFI_TRIG_TTL1                (1)
#define MFI_TRIG_TTL2                (2)
#define MFI_TRIG_TTL3                (3)
#define MFI_TRIG_TTL4                (4)
#define MFI_TRIG_TTL5                (5)
#define MFI_TRIG_TTL6                (6)
#define MFI_TRIG_TTL7                (7)

#define TRIGCONDTION(a)              (1<<a)

#define MFI_QUEUE                    (1)
#define MFI_HNDLR                    (2)
#define MFI_SUSPEND_HNDLR            (4)
#define MFI_ALL_MECH                 (0xFFFF)

#define MFI_ASYNC_READ_MSG           (0)
#define MFI_ASYNC_READ_DATA          (1)
#define MFI_ASYNC_WRITE_MSG          (2)
#define MFI_ASYNC_WRITE_DATA         (3)


#if defined(__cplusplus) || defined(__cplusplus__)
   }
#endif

#endif