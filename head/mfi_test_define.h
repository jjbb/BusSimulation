#ifndef _TEST_DEFINE_HEADER_
#define _TEST_DEFINE_HEADER_

#include "mfiapi.h"

//#define EMULATOR
#define HOST_TEST
//#define ALL_TEST
//#define SEND_MSG_SYNC
//#define SEND_MSG_ASYNC
//#define RECV_MSG_ASYNC
//#define RECEIVE_MSG
//#define TEST_RESEND
#define TEST_ASYNC
#define PRINT_MSG_FREAM_SEND
#define PRINT_MSG_FREAM_RECV
#define RESEND_MSG
#define ATTR_MECH
#define PTHREAD_TEST
#define test_uart


void ModuleInit(int num);
MfiStatus Test_SendMsg(MfiSession mfi);
MfiStatus Test_ReceiveMsg(MfiSession Mfi, MfiUInt32 timeout);
MfiStatus test_handler(MfiSession Mfi, MfiEventType eventType, MfiEvent event, MfiAddr userHandle);
MfiStatus Test_ReceiveMsgAsync(MfiSession Mfi);
MfiStatus Test_SendMsgAsync(MfiSession Mfi);
void test_resend(MfiSession mfi);
void test_async_io(MfiSession mfi);
void*asyncWrite(void*);

void Emulator_send(char* buf, int len);
int Emulator_recv(char** buf, int* len);
#endif