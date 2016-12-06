#include "com_zju_simulation_handlejni_HandleJni.h"

#include "mfi_linux.h"
#include "mfiapi.h"
#include "mfi_test_define.h"
#include "mfi_rsrc_manager.h"
#include "bus_control_datatype.h"
#include "bus_control_front_module.h"

#ifdef HOST_TEST
#define jint int
#endif

// int initSuccessFlag = 0;//初始化成功标志位，正为初始化成功，负为失败，0为未操作

/***************************************************************************
*********************可以被李佳林的VISA调用的读写函数***********************
****************************************************************************/

void syncClockTime(unsigned int clockTime){
	unsigned int syncTime = 0;
	if( clockTime == 0 ){
		syncTime = getHostTime();
	}else{
		syncTime = clockTime;
	}
	syncModulesTime(syncTime);
}

 /*
 * VISA当中用来往总线控制发送数据的函数
 *这个方法跟writeDataDown有区别，这个不需要判断是否可写，参数也不一样
 */
 void sendDataToIbusSimulation(char *pDataSend, int DataLenS)
{
 	//获取仲裁场32位，然后进行解析
 	MID_BITS temp = *(MID_BITS *)pDataSend;
 	paraseMessageType(temp);

	 if( 0 == canBeSend() ){
		 return;
	 }
 	
 	if(m_class_big == (jint)5 || (m_class_big == (jint)15)){//如果消息大类是5，就是app消息，那么就把这个app消息的源地址和目标地址进行交换
 		char *p_data_temp = (char *)malloc(DataLenS);//分配DataLenS个字节用来存放消息内容
 		int i = 0;
 		for(;i<DataLenS;i++){//完成原来消息当中的数据到我自己的内存当中的拷贝
 			*(p_data_temp+i) = *(pDataSend+i);//循环执行数据长度次，完成数据全拷贝
 		}

		// temp.m_dst_addr = (*(MID_BITS *)p_data_temp).m_src_addr;//完成源地址和目的地址的交换
		// (*(MID_BITS *)p_data_temp).m_src_addr = (*(MID_BITS *)p_data_temp).m_dst_addr;//完成源地址和目的地址的交换
		// (*(MID_BITS *)p_data_temp).m_dst_addr = temp.m_dst_addr;//完成源地址和目的地址的交换
		// putDataToModule(m_destination_addr, p_data_temp, DataLenS);//将需要发送的数据放入虚拟前端模块当中，待发送
		
 	}else if(m_class_big == (jint)6){//如果消息大类是6，就是触发器消息或者属性设置消息，那么就把这个触发器消息的源地址和目标地址进行交换
		char *p_data_temp = (char *)malloc(DataLenS);//分配DataLenS个字节用来存放消息内容
 		int i = 0;
 		for(;i<DataLenS;i++){//完成原来消息当中的数据到我自己的内存当中的拷贝
 			*(p_data_temp+i) = *(pDataSend+i);//循环执行数据长度次，完成数据全拷贝
 		}

		// temp.m_dst_addr = (*(MID_BITS *)p_data_temp).m_src_addr;//完成源地址和目的地址的交换
		// (*(MID_BITS *)p_data_temp).m_src_addr = (*(MID_BITS *)p_data_temp).m_dst_addr;//完成源地址和目的地址的交换
		// (*(MID_BITS *)p_data_temp).m_dst_addr = temp.m_dst_addr;//完成源地址和目的地址的交换
		(*(MID_BITS *)p_data_temp).m_class1 = temp.m_class0;//class3210=0110=6,所以设置为5就是设置为0101；即设置为5，编程app消息
		(*(MID_BITS *)p_data_temp).m_class0 = temp.m_class2;//class3210=0110=6,所以设置为5就是设置为0101；即设置为5，编程app消息
		
		//！！！！！！！！！！！------------------触发配置消息------------------------！！！！！！！！！
		if(m_class_small == (jint)0x1000){//如果消息小类是0x1000，则为触发配置消息
			jint triggerNumber = (jint)(*(p_data_temp+Head_Len)) | ((jint)(*(p_data_temp+Head_Len+1))<<8);//去掉前四个字节的仲裁场以及两个备用字节
			setModuleTriggerNum(m_destination_addr, (int)triggerNumber);//给目标前端模块设置触发线
		}
		//！！！！！！！！！！！------------------触发取消消息------------------------！！！！！！！！！
		if(m_class_small == (jint)0x0FFF){//如果消息小类是0x0FFF，则为触发取消消息
			//由于这里考虑一个前端模块只能被一条触发线支配，所以取消的时候直接取消即可，不管触发线的编号
			//jint triggerNumber = (jint)(*(p_data_temp+6)) | ((jint)(*(pDataSend+7))<<8);//去掉前四个字节的仲裁场以及两个备用字节然后获取一个char的数据（一个字节）
			cancelModuleTriggerNum(m_destination_addr);//给目标前端模块取消触发线
		}
		//！！！！！！！！！！！------------------属性设置消息------------------------！！！！！！！！！
		if(m_class_small == (jint)0x1003){//如果消息小类是0x1003，则为设置消息滤波器消息
			setModuleRegisterBuf(m_destination_addr, (p_data_temp+ Head_Len), DataLenS - Head_Len);//将收到的属性值设置到对应IP地址的模块中
		}
		
 		putDataToModule(m_destination_addr, p_data_temp, DataLenS);//将需要发送的数据放入虚拟前端模块当中，待发送
 	}
	else if(m_class_big == (jint)7){//如果消息大类是7，就是属性获取消息，那么就把这个消息的源地址和目标地址进行交换

		//！！！！！！！！！！！------------------属性获取消息------------------------！！！！！！！！！
		if(m_class_small == (jint)0x1003){//如果消息小类是0x1003，则为获取消息滤波器消息
			char *p_register_buf;//这里好像是不用分配地址的，不太懂，所以后面这些字不删除，以备后用// = (char *)malloc(4);//分配四个字节用来存放属性值
			unsigned int DataLenR = 0;
			int i = 0;
			if(1 == getModuleRegisterBuf(m_destination_addr, &p_register_buf, &DataLenR)){//获取相应IP值的module当中的属性值，这里使用二级指针来进行指针赋值
			
				char *p_data_temp = (char *)malloc(Head_Len + DataLenR);//分配十个字节用来存放消息内容，这个是属性获取消息，其回复是确认的10字节
				// *(p_data_temp+0) = *pDataSend;//1.帧头不变，已改收发地址
				// *(p_data_temp+1) = *(pDataSend+1);//2.帧头不变，已改收发地址
				// *(p_data_temp+2) = *(pDataSend+2);//3.帧头不变，已改收发地址
				// *(p_data_temp+3) = *(pDataSend+3);//4.帧头不变，已改收发地址
				// *(p_data_temp+4) = *(pDataSend+4);//1.内容前两个字节不变，VISA开发人员备用
				// *(p_data_temp+5) = *(pDataSend+5);//2.内容前两个字节不变，VISA开发人员备用
				// *(p_data_temp+6) = *p_register_buf;//1.内容3-6字节为属性值，合起来是一个int
				// *(p_data_temp+7) = *(p_register_buf+1);//2.内容3-6字节为属性值，合起来是一个int
				// *(p_data_temp+8) = *(p_register_buf+2);//3.内容3-6字节为属性值，合起来是一个int
				// *(p_data_temp+9) = *(p_register_buf+3);//4.内容3-6字节为属性值，合起来是一个int
				for(i=0; i<Head_Len; i++){
					*(p_data_temp+i) = *(pDataSend+i);
				}
				for(i=0; i<DataLenR; i++){
					*(p_data_temp + i + Head_Len) = *(p_register_buf + i);
				}

				temp.m_dst_addr = (*(MID_BITS *)p_data_temp).m_src_addr;//完成源地址和目的地址的交换
				(*(MID_BITS *)p_data_temp).m_src_addr = (*(MID_BITS *)p_data_temp).m_dst_addr;//完成源地址和目的地址的交换
				(*(MID_BITS *)p_data_temp).m_dst_addr = temp.m_dst_addr;//完成源地址和目的地址的交换
			
				free(p_register_buf);//释放的是这个指针指向的那片内存区域，不一定就是针对这个指针开辟的空间
				putDataToModule(m_destination_addr, p_data_temp, Head_Len + DataLenR);//将需要发送的数据放入虚拟前端模块当中，待发送
			}
 		}
 	}
	else if(m_class_big == (jint)9){	//如果消息大类是9，就是时间同步消息
		char *p_data_temp = (char *)malloc(DataLenS);//分配DataLenS个字节用来存放消息内容
 		int i = 0;
 		for(;i<DataLenS;i++){//完成原来消息当中的数据到我自己的内存当中的拷贝
 			*(p_data_temp+i) = *(pDataSend+i);//循环执行数据长度次，完成数据全拷贝
 		}

		// temp.m_dst_addr = (*(MID_BITS *)p_data_temp).m_src_addr;//完成源地址和目的地址的交换
		// (*(MID_BITS *)p_data_temp).m_src_addr = (*(MID_BITS *)p_data_temp).m_dst_addr;//完成源地址和目的地址的交换
		// (*(MID_BITS *)p_data_temp).m_dst_addr = temp.m_dst_addr;//完成源地址和目的地址的交换
		// putDataToModule(m_destination_addr, p_data_temp, DataLenS);//将需要发送的数据放入虚拟前端模块当中，待发送

		unsigned int syncTime = *(pDataSend + Head_Len)<<24 | *(pDataSend + Head_Len+1)<<16 | *(pDataSend + Head_Len+2)<<8 |  *(pDataSend + Head_Len+3);

		syncClockTime(syncTime);
	 }
}

 /*
 * VISA当中用来从总线控制获取数据的函数
 *这个方法跟readDataUp有区别，这个不需要判断是否可读，是由总线控制器发出可读中断，参数也不一样
 这个是跟真实的visa测试的，上面的那个是个人直接使用java进行测试的
 */
int receiveDataFromIbusSimulation(char **pDataReceive, int *DataLenR)
{
	return getDataFromModule(pDataReceive, DataLenR);
			
}


// #ifdef EMULATOR
// /***************************************************************************
// ******************com_zju_simulation_handlejni_HandleJni.h********************
// ****************************************************************************/
// /*
//  * Class:     com_zju_simulation_handlejni_HandleJni
//  * Method:    startVisaApplication
//  * Signature: ()V
//  */
// JNIEXPORT void JNICALL Java_com_zju_simulation_handlejni_HandleJni_startVisaApplication
//   (JNIEnv *env, jobject obj)
//   {
//   	mymain();//调用李佳林的VISA程序
//   }
// /*
//  * Class:     com_zju_simulation_handlejni_HandleJni
//  * Method:    canBeRead
//  * Signature: ()V
//  */
// JNIEXPORT void JNICALL Java_com_zju_simulation_handlejni_HandleJni_canBeRead
//   (JNIEnv *env, jobject obj)
//   {
//   	//printf("the method of DataJNI_canBeRead is working--6666666666666-Linux\n");
//   	if(isDataEmpty() < 0){//如果前端模块数据非空
//   		kill(0,SIGIO);//发送中断信号
//   	}else{//如果前端模块数据为空
//   		printf("!!!!!!!!No Data to be read!!!!!!!!---------Linux\n");
//   	}
//   }

// /*
//  * Class:     com_zju_simulation_handlejni_HandleJni
//  * Method:    initBusSimulationModules
//  * Signature: ()V
//  */
// JNIEXPORT void JNICALL Java_com_zju_simulation_handlejni_HandleJni_initBusSimulationModules
//   (JNIEnv *env, jobject obj){
//   	int count = 0;
//   	while(initSuccessFlag<1){//只有当标志位大于0才表示初始化成功
// //  		if(count >500){//直接打印会太快，所以设置一个计数器
// //  			printf("!!!!!!!!Waiting for VISA init!!!!!!!!---------Linux\n");
// //  			count = 0;
// //  		}else{
// //  			count++;
// //  		}
//   	}
//   	//initBusModules();  	//进行虚拟前端模块的总体初始化(先到VISA当中去初始化)
//   }

//  /*
//  * Class:     com_zju_simulation_handlejni_HandleJni
//  * Method:    gernerSigusr
//  * Signature: ()V
//  */
// JNIEXPORT void JNICALL Java_com_zju_simulation_handlejni_HandleJni_gernerSigusr
//   (JNIEnv *env, jobject obj)
//   {
//   	generateModuleTriggerSignal();  //进行循环扫描，并根据已经触发的模块发出触发信号	
//   }

// #endif

// #ifdef HOST_TEST
// void* canBeRead(void* arg){
// 	while(1){
		
// 		while(initSuccessFlag<1);//只有当标志位大于0才表示初始化成功
// 		//printf("the method of DataJNI_canBeRead is working--6666666666666-Linux\n");
//   	if(isDataEmpty() < 0){//如果前端模块数据非空
//   		kill(getpid(),SIGIO);//发送中断信号
//   	}else{//如果前端模块数据为空
//   		//printf("!!!!!!!!No Data to be read!!!!!!!!---------Linux\n");
//   	}
  	
//   	generateModuleTriggerSignal();  //进行循环扫描，并根据已经触发的模块发出触发信号
// 	}
// }

// #endif