#ifndef _MFI_SYSTEM_COMMAND_HEAD_
#define _MFI_SYSTEM_COMMAND_HEAD_

#define URGENT_ORDER_CLASS        0xF
#define URGENT_ORDER_DEF_PORITY   0x7
#define RETRANS_APPLY_OUT         0x0FFF
#define RETRANS_APPLY_IN          0x0FFE

#define EVENT_REPORT_CLASS        0xE
#define EVENT_REPORT_DEF_PORITY   0x7

#define MSG_GET_CLASS             0x7
#define MSG_GET_DEF_PORITY        0x3
#define GET_MANF_ID_CMD           0x1000
#define GET_MANF_NAME_CMD         0x0FFF
#define GET_MODEL_CODE_CMD        0x0FFE
#define GET_MODEL_NAME_CMD        0x0FFD
#define GET_MODEL_BIRTHDAY_CMD    0x0FFC
#define GET_MSG_TLEN_CMD          0x1001
#define GET_DATA_TLEN_CMD         0x1002
#define GET_MSG_FILTER0_CMD       0x1003
#define GET_MSG_FILTER1_CMD       0x1004
#define GET_DATA_FILTER0_CMD      0x1005
#define GET_DATA_FILTER1_CMD      0x1006

#define MSG_SET_CLASS             0x6
#define MSG_SET_DEF_PORITY        0x2
#define SET_TRIGGER_DISABLE       0x0FFF
#define SET_TRIGGER_ENABLE        0x1000
#define SET_MSG_TLEN_CMD          0x1001
#define SET_DATA_TLEN_CMD         0x1002
#define SET_MSG_FILTER1_CMD       0x1003
#define SET_DATA_FILTER1_CMD      0x1004
#define SET_MSG_FIX_TWIN_CMD      0x1005
#define SET_DATA_FIX_TWIN_CMD     0x1006

#define APP_MSG_CLASS             0x5
#define APP_MSG_DEF_PORITY        0x2

#define APP_DATA_CLASS            0x5
#define APP_DATA_DEF_PORITY       0x2

#endif