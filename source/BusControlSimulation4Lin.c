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

// int initSuccessFlag = 0;//��ʼ���ɹ���־λ����Ϊ��ʼ���ɹ�����Ϊʧ�ܣ�0Ϊδ����

/***************************************************************************
*********************���Ա������ֵ�VISA���õĶ�д����***********************
****************************************************************************/
 /*
 * VISA�������������߿��Ʒ������ݵĺ���
 *����������writeDataDown����������������Ҫ�ж��Ƿ���д������Ҳ��һ��
 */
 void sendDataToIbusSimulation(char *pDataSend, int DataLenS)
{
 	//��ȡ�ٲó�32λ��Ȼ�����н���
 	MID_BITS temp = *(MID_BITS *)pDataSend;
 	paraseMessageType(temp);
 	
 	if(m_class_big == (jint)5 || (m_class_big == (jint)15)){//������Ϣ������5������app��Ϣ����ô�Ͱ�����app��Ϣ��Դ��ַ��Ŀ����ַ���н���
 		char *p_data_temp = (char *)malloc(DataLenS);//����DataLenS���ֽ�����������Ϣ����
 		int i = 0;
 		for(;i<DataLenS;i++){//����ԭ����Ϣ���е����ݵ����Լ����ڴ浱�еĿ���
 			*(p_data_temp+i) = *(pDataSend+i);//ѭ��ִ�����ݳ��ȴΣ���������ȫ����
 		}
		temp.m_dst_addr = (*(MID_BITS *)p_data_temp).m_src_addr;//����Դ��ַ��Ŀ�ĵ�ַ�Ľ���
 		(*(MID_BITS *)p_data_temp).m_src_addr = (*(MID_BITS *)p_data_temp).m_dst_addr;//����Դ��ַ��Ŀ�ĵ�ַ�Ľ���
 		(*(MID_BITS *)p_data_temp).m_dst_addr = temp.m_dst_addr;//����Դ��ַ��Ŀ�ĵ�ַ�Ľ���
 		putDataToModule(m_destination_addr, p_data_temp, DataLenS);//����Ҫ���͵����ݷ�������ǰ��ģ�鵱�У�������
 	}else if(m_class_big == (jint)6){//������Ϣ������6�����Ǵ�������Ϣ��������������Ϣ����ô�Ͱ�������������Ϣ��Դ��ַ��Ŀ����ַ���н���
		char *p_data_temp = (char *)malloc(DataLenS);//����DataLenS���ֽ�����������Ϣ����
 		int i = 0;
 		for(;i<DataLenS;i++){//����ԭ����Ϣ���е����ݵ����Լ����ڴ浱�еĿ���
 			*(p_data_temp+i) = *(pDataSend+i);//ѭ��ִ�����ݳ��ȴΣ���������ȫ����
 		}

		temp.m_dst_addr = (*(MID_BITS *)p_data_temp).m_src_addr;//����Դ��ַ��Ŀ�ĵ�ַ�Ľ���
		(*(MID_BITS *)p_data_temp).m_src_addr = (*(MID_BITS *)p_data_temp).m_dst_addr;//����Դ��ַ��Ŀ�ĵ�ַ�Ľ���
		(*(MID_BITS *)p_data_temp).m_dst_addr = temp.m_dst_addr;//����Դ��ַ��Ŀ�ĵ�ַ�Ľ���
		(*(MID_BITS *)p_data_temp).m_class1 = temp.m_class0;//class3210=0110=6,��������Ϊ5��������Ϊ0101��������Ϊ5������app��Ϣ
		(*(MID_BITS *)p_data_temp).m_class0 = temp.m_class2;//class3210=0110=6,��������Ϊ5��������Ϊ0101��������Ϊ5������app��Ϣ
		
		//����������������������------------------����������Ϣ------------------------������������������
		if(m_class_small == (jint)0x1000){//������ϢС����0x1000����Ϊ����������Ϣ
			jint triggerNumber = (jint)(*(p_data_temp+6)) | ((jint)(*(p_data_temp+7))<<8);//ȥ��ǰ�ĸ��ֽڵ��ٲó��Լ����������ֽ�
			setModuleTriggerNum(m_destination_addr, (int)triggerNumber);//��Ŀ��ǰ��ģ�����ô�����
		}
		//����������������������------------------����ȡ����Ϣ------------------------������������������
		if(m_class_small == (jint)0x0FFF){//������ϢС����0x0FFF����Ϊ����ȡ����Ϣ
			//�������￼��һ��ǰ��ģ��ֻ�ܱ�һ��������֧�䣬����ȡ����ʱ��ֱ��ȡ�����ɣ����ܴ����ߵı���
			//jint triggerNumber = (jint)(*(p_data_temp+6)) | ((jint)(*(pDataSend+7))<<8);//ȥ��ǰ�ĸ��ֽڵ��ٲó��Լ����������ֽ�Ȼ����ȡһ��char�����ݣ�һ���ֽڣ�
			cancelModuleTriggerNum(m_destination_addr);//��Ŀ��ǰ��ģ��ȡ��������
		}
		//����������������������------------------����������Ϣ------------------------������������������
		if(m_class_small == (jint)0x1003){//������ϢС����0x1003����Ϊ������Ϣ�˲�����Ϣ
			setModuleRegisterBuf(m_destination_addr, (p_data_temp+6),5);//���յ�������ֵ���õ���ӦIP��ַ��ģ����
		}
		
 		putDataToModule(m_destination_addr, p_data_temp, DataLenS);//����Ҫ���͵����ݷ�������ǰ��ģ�鵱�У�������
 	}
	else if(m_class_big == (jint)7){//������Ϣ������7���������Ի�ȡ��Ϣ����ô�Ͱ�������Ϣ��Դ��ַ��Ŀ����ַ���н���

		//����������������������------------------���Ի�ȡ��Ϣ------------------------������������������
		if(m_class_small == (jint)0x1003){//������ϢС����0x1003����Ϊ��ȡ��Ϣ�˲�����Ϣ
			char *p_register_buf;//���������ǲ��÷�����ַ�ģ���̫�������Ժ�����Щ�ֲ�ɾ�����Ա�����// = (char *)malloc(4);//�����ĸ��ֽ�������������ֵ
			int DataLenR = 0;
			getModuleRegisterBuf(m_destination_addr, &p_register_buf, &DataLenR);//��ȡ��ӦIPֵ��module���е�����ֵ������ʹ�ö���ָ��������ָ�븳ֵ
			
			char *p_data_temp = (char *)malloc(10);//����ʮ���ֽ�����������Ϣ���ݣ����������Ի�ȡ��Ϣ�����ظ���ȷ�ϵ�10�ֽ�
 			*(p_data_temp+0) = *pDataSend;//1.֡ͷ���䣬�Ѹ��շ���ַ
 			*(p_data_temp+1) = *(pDataSend+1);//2.֡ͷ���䣬�Ѹ��շ���ַ
 			*(p_data_temp+2) = *(pDataSend+2);//3.֡ͷ���䣬�Ѹ��շ���ַ
 			*(p_data_temp+3) = *(pDataSend+3);//4.֡ͷ���䣬�Ѹ��շ���ַ
 			*(p_data_temp+4) = *(pDataSend+4);//1.����ǰ�����ֽڲ��䣬VISA������Ա����
 			*(p_data_temp+5) = *(pDataSend+5);//2.����ǰ�����ֽڲ��䣬VISA������Ա����
 			*(p_data_temp+6) = *p_register_buf;//1.����3-6�ֽ�Ϊ����ֵ����������һ��int
 			*(p_data_temp+7) = *(p_register_buf+1);//2.����3-6�ֽ�Ϊ����ֵ����������һ��int
 			*(p_data_temp+8) = *(p_register_buf+2);//3.����3-6�ֽ�Ϊ����ֵ����������һ��int
 			*(p_data_temp+9) = *(p_register_buf+3);//4.����3-6�ֽ�Ϊ����ֵ����������һ��int

			temp.m_dst_addr = (*(MID_BITS *)p_data_temp).m_src_addr;//����Դ��ַ��Ŀ�ĵ�ַ�Ľ���
			(*(MID_BITS *)p_data_temp).m_src_addr = (*(MID_BITS *)p_data_temp).m_dst_addr;//����Դ��ַ��Ŀ�ĵ�ַ�Ľ���
			(*(MID_BITS *)p_data_temp).m_dst_addr = temp.m_dst_addr;//����Դ��ַ��Ŀ�ĵ�ַ�Ľ���
		
 			free(p_register_buf);//�ͷŵ�������ָ��ָ������Ƭ�ڴ����򣬲�һ��������������ָ�뿪�ٵĿռ�
 			putDataToModule(m_destination_addr, p_data_temp, 10);//����Ҫ���͵����ݷ�������ǰ��ģ�鵱�У�������
 		}
 	}
}

 /*
 * VISA�������������߿��ƻ�ȡ���ݵĺ���
 *����������readDataUp����������������Ҫ�ж��Ƿ��ɶ����������߿����������ɶ��жϣ�����Ҳ��һ��
 �����Ǹ���ʵ��visa���Եģ��������Ǹ��Ǹ���ֱ��ʹ��java���в��Ե�
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
//   	mymain();//���������ֵ�VISA����
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
//   	if(isDataEmpty() < 0){//����ǰ��ģ�����ݷǿ�
//   		kill(0,SIGIO);//�����ж��ź�
//   	}else{//����ǰ��ģ������Ϊ��
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
//   	while(initSuccessFlag<1){//ֻ�е���־λ����0�ű�ʾ��ʼ���ɹ�
// //  		if(count >500){//ֱ�Ӵ�ӡ��̫�죬��������һ��������
// //  			printf("!!!!!!!!Waiting for VISA init!!!!!!!!---------Linux\n");
// //  			count = 0;
// //  		}else{
// //  			count++;
// //  		}
//   	}
//   	//initBusModules();  	//��������ǰ��ģ����������ʼ��(�ȵ�VISA����ȥ��ʼ��)
//   }

//  /*
//  * Class:     com_zju_simulation_handlejni_HandleJni
//  * Method:    gernerSigusr
//  * Signature: ()V
//  */
// JNIEXPORT void JNICALL Java_com_zju_simulation_handlejni_HandleJni_gernerSigusr
//   (JNIEnv *env, jobject obj)
//   {
//   	generateModuleTriggerSignal();  //����ѭ��ɨ�裬�������Ѿ�������ģ�鷢�������ź�	
//   }

// #endif

// #ifdef HOST_TEST
// void* canBeRead(void* arg){
// 	while(1){
		
// 		while(initSuccessFlag<1);//ֻ�е���־λ����0�ű�ʾ��ʼ���ɹ�
// 		//printf("the method of DataJNI_canBeRead is working--6666666666666-Linux\n");
//   	if(isDataEmpty() < 0){//����ǰ��ģ�����ݷǿ�
//   		kill(getpid(),SIGIO);//�����ж��ź�
//   	}else{//����ǰ��ģ������Ϊ��
//   		//printf("!!!!!!!!No Data to be read!!!!!!!!---------Linux\n");
//   	}
  	
//   	generateModuleTriggerSignal();  //����ѭ��ɨ�裬�������Ѿ�������ģ�鷢�������ź�
// 	}
// }

// void syncClockTime(unsigned int clockTime ){
// 	unsigned int synTime = clockTime;
// 	if(synTime == 0){
// 		//synchronize time of all modules to the host boardIP
// 		synTime = getHostTime();
// 	}
// 	//synchronize time of all modules to the parameter
// 	syncModulesTime(synTime);	
// }

// #endif