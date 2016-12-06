#ifndef _MFI_DEFINE_HEADER_
#define _MFI_DEFINE_HEADER_

//rsrc_manager
#define BUS_RSRC_TYPE 1
#define MODULE_RSRC_TYPE 2

#define SESSION_MAX_NUM 31
#define FINDRSRC_MAX_NUM 31
#define RSRC_MAX_SESSION_ID 31
#define RM_MIN_SESSION_ID 256
#define RM_MAX_SESSION_ID 356
#define FINDLIST_MIN_ID 512
#define FINDLIST_MAX_ID 1023
#define EVENT_MIN_ID 1024
#define EVENT_MAX_ID 0xFFFFFFFF

//rsrc_module
#define TRIG_LINE_NUM 8
#define MODULE_RSRC_EVENT_NUM 9

//rsrc_bus
#define SUBWIN_NUM 32
#define BUS_RSRC_EVENT_NUM 2

//attribute
#define MODULE_RSRC_ATTR_NUM   32
#define BUS_RSRC_ATTR_NUM      33
#define RM_ATTR_NUM            9
#define RSRC_NAME_LEN          30

#define TRIG_EVENT_ATTR_NUM    2
#define ASYNC_EVENT_ATTR_NUM   9
#define FAULT_EVENT_ATTR_NUM   4

#define ATTR_OTHER_VALUE 8

#define READONLY        1
#define READWRITE       0
#define HDWARE          1
#define NHDWARE         0
#define REALTIME        1
#define NREALTIME       0

#define INTTYPE         0
#define UINTTYPE        1
#define STRINGTYPE      2

#define NO_VAL 0

#define MFI_MANF_ID            0x00000001
#define BUS_MANF_ID            MFI_MANF_ID
#define BUS_RSRC_TYPE_S        "BUS"
#define MODULE_RSRC_TYPE_S     "MODULE"
#define DEF_QUEUE_LEN          50
#define DEF_TWO_VALUE          2000   //ms
#define DEF_MSG_BASIC_TLEN     60000 //待确认
#define MAX_MSG_BASIC_TLEN     65535 //待确认
#define MIN_MSG_BASIC_TLEN     10//待确认
#define DEF_MSG_FIX_TLEN       160 //待确认
#define MAX_MSG_FIX_TLEN       65535 //待确认
#define MIN_MSG_FIX_TLEN       10//待确认    
#define DEF_DATA_BASIC_TLEN    60000 //待确认
#define MAX_DATA_BASIC_TLEN    65535 //待确认
#define MIN_DATA_BASIC_TLEN    10//待确认    
#define DEF_DATA_FIX_TLEN      160 //待确认
#define MAX_DATA_FIX_TLEN      65535 //待确认
#define MIN_DATA_FIX_TLEN      10//待确认    
#define DEF_MSG_RFIFO_LEN  10
#define MAX_MSG_RFIFO_LEN  100
#define MIN_MSG_RFIFO_LEN  1
#define DEF_DATA_RFIFO_LEN  10
#define MAX_DATA_RFIFO_LEN  100
#define MIN_DATA_RFIFO_LEN  1
#define DEF_BROADCAST_FILTER    0x000007C0
#define DEF_HOST_ACR0_FILTER    0x00000040
#define DEF_HOST_ACR1_FILTER    DEF_BROADCAST_FILTER
#define DEF_AMR0_FILTER         0xFFFFF83F
#define CLOSE_AMR_FILTER        0xFFFFFFFF
#define HOST_BIRTHDAY           0x07E10101  //2017.1.1
#define HOST_MAX_CURRENT        2000
#define HOST_FIX_TWIN           0x00000001  //系统默认主机板占用第一个固定时间窗

//event
#define NOMECH       0
#define CALLBACKMECH 2
#define SUSPENDMECH  4
#define QUEUEMECH    1

#define MECHCHANGEFIFOLEN 10

//io

//session
#ifdef SIMPLE
#define INUSE 1
#define NOINUSE 0
#define MfiSesStatus                MfiUInt32
#define SesStatusCheck(psession)   ((psession)->is_inuse)  
#define SesStatusFree(psession)     
#define SesStatusSetUSE(psession)    ((psession)->is_inuse=INUSE)  
#define SesStatusSetNOUSE(psession)  ((psession)->is_inuse=NOINUSE)  
#define SesStatusInit(psession)
#define SesStatusClear(psession)
#else
//读写锁的使用用来解决close会话接口调用时，与其他异步操作的同步问题
#define INUSE 0
#define NOINUSE EBUSY
#define MfiSesStatus                pthread_rwlock_t
#define SesStatusCheck(psession)   (pthread_rwlock_tryrdlock(&((psession)->is_inuse)))
#define SesStatusFree(psession)    (pthread_rwlock_unlock(&((psession)->is_inuse)))
#define SesStatusSetUSE(psession)    (pthread_rwlock_unlock(&((psession)->is_inuse)))
#define SesStatusSetNOUSE(psession)  (pthread_rwlock_wrlock(&((psession)->is_inuse)))
#define SesStatusInit(psession)     \
({ \
	pthread_rwlock_init(&((psession)->is_inuse),MFI_NULL); \
	pthread_rwlock_wrlock(&((psession)->is_inuse)); \
	})
#define SesStatusClear(psession)     \
({ \
	pthread_rwlock_unlock(&((psession)->is_inuse)); \
	pthread_rwlock_destroy(&((psession)->is_inuse)); \
	})
#endif

//message
#define HOST_IP 1
#define MY_IP 0
#define BROADCAST_IP   31

#define Head_Len 4

#define MESSAGE_MAX_LEN 12
#define MSG_FIFO_DEF_LEN 100
#define COMBMSG_KEEP_TIME 500
#define MSG_TMP_BUF_DEF_LEN 255

//data
#define DATA_MAX_LEN 1000
#define DATA_FIFO_DEF_LEN 100
#define DATA_TMP_FIFO_DEF_LEN 512
#define COMBDATA_KEEP_TIME 500

//trigger
#define MODE_TRIGGER_MASTER 1
#define MODE_TRIGGER_SLAVER 0

#define MAX_MODULE_NUM 32

#endif