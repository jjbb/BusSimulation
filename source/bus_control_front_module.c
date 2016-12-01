#include "bus_control_front_module.h"
#include "mfi_test_define.h"
#include "bus_control_datatype.h"
#include "mfi_define.h"
#include "bus_control_datatype.h"
#ifdef HOST_TEST
// #include <pthread.h>
extern void* canBeRead(void* arg);
#endif

busModule busModuleSimulation[32];//建立变量存放前端模块虚拟器的结构体变量
int usingModuleNumber = 0;//现在仪器上可以使用的前端模块虚拟器个数
//太麻烦了这个功能先不做int callForGetDataModuleIp = 0;//具有数据存储，已经发出中断信号请求过来取走数据的模块IP
int countToAbandon = 0;//作为需要弃掉一些帧的一个计数
int initSuccessFlag = 0;
int TriggerSig1 = 0;
int TriggerSig2 = 0;

/*
 * Method:    getModuleIP
 * 使用底层函数进行板子ip获取（没有使用api函数）
*/
int getModuleIP(int boardNumber){
  return (int)Module.Module_Info_p[boardNumber].mod_ip;//返回特定板子的ip地址
}

/*
 * Method:    creatFrontModuleSimulation
 * 根据板子数量进行前端模块虚拟器创建
 * Signature: ()I
*/
void creatFrontModuleSimulation(int number){
	int i;
	usingModuleNumber = number;
	for(i=0;i < 32;i++){
		if(i<number){
			busModuleSimulation[i].working = 1;
			busModuleSimulation[i].dataHeadNode=NULL;
			busModuleSimulation[i].dataTailNode=NULL;
			//busModuleSimulation[i].dataHeadNode.pdata = NULL;
			//busModuleSimulation[i].dataHeadNode.mdatalen = 0;
			//busModuleSimulation[i].dataHeadNode.next = NULL;
			//busModuleSimulation[i].dataTailNode.pdata = NULL;
			//busModuleSimulation[i].dataTailNode.mdatalen = 0;
			//busModuleSimulation[i].dataTailNode.next = NULL;
			busModuleSimulation[i].dataLength = 0;
			busModuleSimulation[i].boardIP = getModuleIP(i);
			busModuleSimulation[i].registerBuf = NULL;
			busModuleSimulation[i].triggerNumber = -1;
			// pthread_mutex_init(&(busModuleSimulation[i].lock),NULL);
		}else{
			busModuleSimulation[i].working = -1;
		}
	}
}

/*
 * Method:    getModuleNum
 * 使用底层函数进行板子数量获取（没有使用api函数）
*/
int getModuleNum(){
  return (int)Module.number;//返回模块板的数量
}

/*
 * Method:    getModuleIP
 * 使用底层函数进行板子ip获取（没有使用api函数）
*/
void initBusModules(){
	int moduleNum = 0;//新建变量存放板子数量
	initModuleInfo();
	
	// #ifdef HOST_TEST
	// pthread_t pid;
	// pthread_create(&pid,NULL,canBeRead,0);
	// #endif
	
	moduleNum = getModuleNum();//获取板子数量并赋值
	if(moduleNum > 0 && moduleNum < 32){//只有在板子数量在正常范围内才进行初始化
		creatFrontModuleSimulation(moduleNum);//创建前端模块虚拟器并完成初始化工作
		initSuccessFlag = 1;
	}else{
		printf("!!!!!!init bus modules failed failed failed!!!!!!----------Linux\n");
	}
}

/*
 * Method:    putDataToModule
 * 将收到的数据放到模拟的前端模块虚拟器当中
*/
void putDataToModule(int ipNumber, char* pDataHead, int length){
	int count = 0;
	busDataNode* temp=NULL;
	if(m_class_big != 15 && m_retry !=1){
		countToAbandon++;
		if(countToAbandon%100 == 0){
			return;//一旦出现7的倍数，那就直接返回，表示弃掉本帧数据；即每隔7个数据就会丢掉一个。
		}
	}	//不同
	while(count < usingModuleNumber){
		if(ipNumber == busModuleSimulation[count].boardIP){//找到数据要发送的目的地模块，就进行数据添加
			temp=(busDataNode*)malloc(sizeof(busDataNode));
			temp->pdata=pDataHead;
			//temp.pdata = (char *)malloc(length);//给存放数据的指针分配内存
			//int cntNum = 0;//设置计数变量
			//for(;cntNum < length;cntNum++){//循环进行数据拷贝
			//	*(temp.pdata + cntNum) = *(pDataHead + cntNum);//将发过来的数据放到我开辟的内存当中
			//}
			temp->mdatalen = length;//设置本节点当中存放的数据的长度
			temp->next = NULL;//本节点将成为tail，故设置其下一个节点为NULL
			// pthread_mutex_lock(&(busModuleSimulation[count].lock));
			if(busModuleSimulation[count].dataLength != 0){//链表长度不为0表示已经有数据了
				busModuleSimulation[count].dataTailNode->next = temp;//将新节点添加到尾节点后面
				busModuleSimulation[count].dataTailNode = temp;//设置新添加的节点为尾节点
				busModuleSimulation[count].dataLength++;
			}else{//链表长度为0就表示现在添加第一个数据到链表当中
				busModuleSimulation[count].dataHeadNode = temp;//设置新添加的节点为头结点
				busModuleSimulation[count].dataTailNode = temp;//设置新添加的节点为尾节点
				busModuleSimulation[count].dataLength=1;
			}
			pthread_mutex_unlock(&(busModuleSimulation[count].lock));
			temp=NULL;
			return;//正常添加数据节点后，直接返回，表示结束本函数
		}
		count++;
	}
	printf("!!!!!!put data to module failed failed failed!No right ip number!!!!!----------Linux\n");
}

/*
 * 1号
 * Method:    isModuleDataEmpty
 * 判断某个模拟的前端模块虚拟器当中是否含有数据
 * param：模块IP
 * return：-1表示非空，1表示空
*/
int isModuleDataEmpty(int ipNumber){
	int count = 0;
	while(count < usingModuleNumber){
		if(ipNumber == busModuleSimulation[count].boardIP){
			//pthread_mutex_lock(&(busModuleSimulation[count].lock));
			if(busModuleSimulation[count].dataLength != 0){//链表长度不为0表示已经有数据了
			//	pthread_mutex_unlock(&(busModuleSimulation[count].lock));
				return -1;//有数据了则返回-1表示非空
			}else{
				//pthread_mutex_unlock(&(busModuleSimulation[count].lock));
				return 1;//无数据则返回1表示为空
			}
		}
		count++;
	}
	return 1;//进入此处则表示没有这个ip地址，则返回1表示为空
}

/*
 * 2号
 * Method:    isDataEmpty
 * 判断模拟的前端模块虚拟器当中是否含有数据
 * return：-1表示非空，1表示空
*/
int isDataEmpty(){
	int count = 0;
	while(count < usingModuleNumber){
		if(busModuleSimulation[count].dataLength != 0){//链表长度不为0表示已经有数据了
			return -1;//有数据了则返回-1表示非空
		}
		count++;
	}
	return 1;//进入此处则表示所有模块都没有数据，则返回1表示为空
}


/*
 * Method:    getDataFromModule
 * 获取模拟的前端模块虚拟器当中的数据，放到传入的指针当中
 * return：-1表示获取失败，1表示成功
*/
int getDataFromModule(char** pDataGotHead, int* gotLength){
	int count = 0;
	while(count < usingModuleNumber){
//		if(ipNumber == busModuleSimulation[count].boardIP){
		// pthread_mutex_lock(&(busModuleSimulation[count].lock));
		if(busModuleSimulation[count].dataLength != 0){//链表长度不为0表示已经有数据了
			*gotLength = busModuleSimulation[count].dataHeadNode->mdatalen;//传输数据长度值给变量
			*pDataGotHead = busModuleSimulation[count].dataHeadNode->pdata;//将头节点的数据指针赋给对方的指针

			if(busModuleSimulation[count].dataLength == 1){//链表长度为1就表示已经头尾节点是同一个
				//busModuleSimulation[count].dataHeadNode.pdata = NULL;//传出一个数据之后已经没有数据了
				//busModuleSimulation[count].dataHeadNode.mdatalen = 0;//传出一个数据之后已经没有数据了

				free(busModuleSimulation[count].dataHeadNode);
				busModuleSimulation[count].dataHeadNode=NULL;
				busModuleSimulation[count].dataTailNode=NULL;
				busModuleSimulation[count].dataLength=0;
				//busModuleSimulation[count].dataHeadNode.next = NULL;//传出一个数据之后已经没有数据了
				//busModuleSimulation[count].dataTailNode.pdata = NULL;//传出一个数据之后已经没有数据了
				//busModuleSimulation[count].dataTailNode.mdatalen = 0;//传出一个数据之后已经没有数据了
				//busModuleSimulation[count].dataTailNode.next = NULL;//传出一个数据之后已经没有数据了
			}else{//链表长度不为1就表示头尾节点不是同一个
				busDataNode* temp;
				temp=busModuleSimulation[count].dataHeadNode;
				busModuleSimulation[count].dataHeadNode=temp->next;
				// free(temp); 该节点是被读取的节点，不能在这里释放
				temp=NULL;
				busModuleSimulation[count].dataLength--;
				//temp = busModuleSimulation[count].dataHeadNode;//提取头节点备用
				//busModuleSimulation[count].dataHeadNode.next = NULL;//将原来的头节点和其后面的节点断开（此处不影响temp的值）
				//busModuleSimulation[count].dataHeadNode = *(temp.next);//将原来的第二节点设置为头节点
			}
			// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
			return 1;//数据正常返回，则返回1表示成功
		}
//			else{
//				return -1;//无数据则返回-1表示获取失败
//			}
//		}
		//  pthread_mutex_unlock(&(busModuleSimulation[count].lock));
		count++;
	}
	return -1;//进入此处则表示没有这个ip地址，则返回-1表示获取失败
}

/*
 * Method:    getModuleRegisterBuf
 * 获取模拟的前端模块虚拟器当中的寄存器数据，放到传入的指针当中
 * return：-1表示获取失败，1表示成功
*/
int getModuleRegisterBuf(int ipNumber, char** gotRegisterBuf, unsigned int *bufLength){
	int count = 0;
	while(count < usingModuleNumber){
		if(ipNumber == busModuleSimulation[count].boardIP){
			// pthread_mutex_lock(&(busModuleSimulation[count].lock));
			if(busModuleSimulation[count].registerBuf != NULL){//寄存器值不为-1表示有新值可以被获取
				*gotRegisterBuf = busModuleSimulation[count].registerBuf;//将寄存器值传到那个指针中
				*bufLength = busModuleSimulation[count].bufLength;	//获取属性长度
				busModuleSimulation[count].registerBuf = NULL;//被获取之后就设置为默认值
				busModuleSimulation[count].bufLength = 0;	//属性长度清零
				// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
				return 1;//数据正常返回，则返回1表示成功
			}else{
				// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
				return -1;//无数据则返回-1表示获取失败
			}
		}
		count++;
	}
	return -1;//进入此处则表示没有这个ip地址，则返回-1表示获取失败
}

/*
 * Method:    setModuleRegisterBuf
 * 根据传入寄存器值设置模拟的前端模块虚拟器当中的寄存器数据
 * return：-1表示设置失败，1表示成功
*/
int setModuleRegisterBuf(int ipNumber, char* RegisterBuf,  unsigned int bufLength){
	int count = 0;
	while(count < usingModuleNumber){
		if(ipNumber == busModuleSimulation[count].boardIP){
			char* pRegisterBuf = (char *)malloc(bufLength);//分配四个字节用来存放属性值内容
			for(i=0; i<bufLength; i++){
				*(pRegisterBuf +i) = *(RegisterBuf + i);//1.?????????????
			}	
			// pthread_mutex_lock(&(busModuleSimulation[count].lock));
			busModuleSimulation[count].registerBuf = pRegisterBuf;//设置此模拟前端模块的寄存器值为传入的值
			busModuleSimulation[count].bufLength = bufLength;
			// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
			return 1;//数据正常设置，则返回1表示成功
		}
		count++;
	}
	return -1;//进入此处则表示没有这个ip地址，则返回-1表示设置失败
}

/*
 * Method:    cancelModuleTriggerNum
 * 获取模拟的前端模块虚拟器当中的触发线编号，放到传入的指针当中
 * return：-1表示取消失败，1表示成功
*/
int cancelModuleTriggerNum(int ipNumber){
	int count = 0;
	while(count < usingModuleNumber){
		if(ipNumber == busModuleSimulation[count].boardIP){
				// pthread_mutex_lock(&(busModuleSimulation[count].lock));
				busModuleSimulation[count].triggerNumber = -1;//设置此模拟前端模块的触发线编号值为传入的值
				// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
				return 1;//完成触发器取消，则返回1表示成功
		}
		count++;
	}
	return -1;//进入此处则表示没有这个ip地址，则返回-1表示取消失败
}

/*
 * Method:    setModuleTriggerNum
 * 根据传入触发线编号值设置模拟的前端模块虚拟器当中的触发线编号
 * return：-1表示设置失败，1表示成功
*/
int setModuleTriggerNum(int ipNumber, int TriggerNum){
	int count = 0;
	while(count < usingModuleNumber){//首先对所有模块进行循环，保证此触发线编号还没被使用才能进行后面的设置
		// pthread_mutex_lock(&(busModuleSimulation[count].lock));
		if(busModuleSimulation[count].triggerNumber == TriggerNum){
			// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
			return -1;//一旦出现这个触发线编号已经在某个模块当中设置了，那就此次设置不成功
		}
		// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
		count++;//循环变量
	}
	
	count = 0;
	while(count < usingModuleNumber){//循环找到ip地址对应的那个虚拟器前端模块
		if(ipNumber == busModuleSimulation[count].boardIP){
			// pthread_mutex_lock(&(busModuleSimulation[count].lock));
			if(busModuleSimulation[count].triggerNumber != -1){//触发线编号值不为-1表示已经被配置为触发线了
				// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
				return -1;//已经配置过了，则返回-1表示设置失败
			}else{
				busModuleSimulation[count].triggerNumber = TriggerNum;//设置此模拟前端模块的触发线编号值为传入的值
				// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
				return 1;//数据正常设置，则返回1表示成功
			}
		}
		count++;
	}
	return -1;//进入此处则表示没有这个ip地址，则返回-1表示设置失败
}

/*
 * Method:    generateModuleTriggerSignal
 * 根据传入触发线编号值设置模拟的前端模块虚拟器当中的触发线编号
 * return：-1表示设置失败，1表示成功
*/
void generateModuleTriggerSignal(){
	int count = 0;
	while(count < usingModuleNumber){
		if(1 == busModuleSimulation[count].triggerNumber){//如果触发线1号存在，那就产生相应的用户信号
			kill(0,SIGUSR1); //产生SIGUSR1信号
			TriggerSig1 = 1;
		}
		if(2 == busModuleSimulation[count].triggerNumber){//如果触发线2号存在，那就产生相应的用户信号
			kill(0,SIGUSR2); //产生SIGUSR2信号
			TriggerSig2 = 1;
		}
		count++;
	}
}

/*
 * Method: handleSimulationData
 * parameter:	oldValue--要替换的消息内容
 * 				newValue--用于替换的消息内容
 * 				newDataValueLen--新消息内容的长度
 */

void handleSimulationData(char *oldValue, char* newValue, int newDataValueLen){
	char* data = NULL;
	int *dataLength = 0;
	int count = 0;
	while(count < usingModuleNumber){
		//		扫描各个虚拟模块
		if(busModuleSimulation[count].dataLength != 0){//链表长度不为0表示已经有数据了
			dataLength = &(busModuleSimulation[count].dataHeadNode->mdatalen);//传输数据长度值给变量
			data = busModuleSimulation[count].dataHeadNode->pdata;//将头节点的数据指针赋给对方的指针

			MID_BITS *temp_node = (MID_BITS *)data;
 			paraseMessageType(*temp_node);	//解析消息

			int temp_add = temp_node->m_dst_addr;
			temp_node->m_dst_addr = temp_node->m_src_addr;
			temp_node->m_src_addr = temp_add;		//交换源地址和目的地址,待发回

			int j = 0;
			int isSame = 1;
			//匹配原来的消息内容
			while( isSame && *(oldValue + j) != 0 && j + 4 < *dataLength){
				if(*(data + 4 + j) != *(oldValue + j)){
					isSame = 0;
				}
				j++;
			}
			j = 0;
			//匹配成功，替换
			if(isSame){
				while(*(newValue + j) != 0){
					realloc(data, sizeof(char) * (4 + newDataValueLen));
					*(data + 4 + j) = *(newValue + j);
					j++;
				}
			}
			j = 0;
			return;
        }
		count++;
	}
}

int getHostTime(){
	int count = 0;
	while(count < usingModuleNumber){
		if( HOST_IP == busModuleSimulation[count].boardIP ){
			return busModuleSimulation[count].clockTime;
		}else{
			count++;
		}
	}
	return -1;
}

void syncClockTime(unsigned int clockTime ){
	unsigned int synTime = clockTime;
	if(synTime == 0){
		//synchronize time of all modules to the host boardIP
		synTime = getHostTime();
	}
	//synchronize time of all modules to the parameter
	syncModulesTime(synTime);	
}

void syncModulesTime(unsigned int syncTime){
	//synchronize time of all modules to the parameter
	int count = 0;
	while(count < usingModuleNumber){
		busModuleSimulation[count].clockTime = syncTime;
		count ++;
	}
}
