#ifndef _BUS_CONTROL_FRONT_MODULE_HEADER_
#define _BUS_CONTROL_FRONT_MODULE_HEADER_

#include "bus_control_general_lib.h"
#include "mfi_module_info.h"
#include <pthread.h>

//数据节点结构体
typedef struct dataNode{
	char* pdata;//数据内容
	int mdatalen;//表示数据长度
	struct dataNode *next;//数据指针，指向下一个节点
}busDataNode;

//模拟前端模块的结构体
typedef struct busSimulationModule{
	int working;//用来说明这个模块是否存在，1是存在，-1是不存在
	busDataNode* dataHeadNode;//用来存放数据内容链表的头节点，未使用时将结构体所有值都默认设置为null
	busDataNode* dataTailNode;//用来存放数据内容链表的尾节点，未使用时将结构体所有值都默认设置为null
	int dataLength;//用来存放数据链表的长度
	int boardIP;//前端模块的IP，未使用时默认设置为-1
	char* registerBuf;//寄存器的值，未使用时默认设置为NULL
	int triggerNumber;//被触发的触发线编号，无触发时默认设置为-1
	unsigned int bufLength;
	unsigned int clockTime;
	// pthread_mutex_t lock;

}busModule;

typedef struct moduleTime{
	unsigned int globalTime;		//全局时间变量
	unsigned int timeWindowStart[MAX_MODULE_NUM];	//时间窗
	unsigned int windowLength;		//时间窗长度
}

#define fixedTimeWindowLen 5

//函数声明，给其他文件使用
void initBusModules();//初始化，创建前端模块虚拟器并完成IP初始化
int isDataEmpty();//判断总线上是否有数据（也就是判断所有模块）
void putDataToModule(int ipNumber, char* pDataHead, int length);//将收到的数据放到指定IP的虚拟前端模块的当中
int getDataFromModule(char** pDataGotHead, int* gotLength);//获取将要被发送的数据和长度
int setModuleRegisterBuf(int ipNumber, char* RegisterBuf, unsigned int bufLength);//将收到的属性值设置到对应IP地址的模块中
int getModuleRegisterBuf(int ipNumber, char** gotRegisterBuf, unsigned int *bufLength);//将对应IP地址的模块中的属性值获取出来
int setModuleTriggerNum(int ipNumber, int TriggerNum);//将收到的触发线编号值设置到对应IP地址的模块中
int cancelModuleTriggerNum(int ipNumber);//将对应IP地址的模块中的触发线编号值取消掉
void generateModuleTriggerSignal();//因为触发线循环产生模块的用户信号
void handleSimulationData();
void syncModulesTime(unsigned int syncTime);

extern int TriggerSig1;
extern int TriggerSig2;
extern unsigned int globalTime;

#endif