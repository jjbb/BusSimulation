#include "bus_control_front_module.h"
#include "mfi_test_define.h"
#include "bus_control_datatype.h"
#include "mfi_define.h"
#include "bus_control_datatype.h"
#ifdef HOST_TEST
// #include <pthread.h>
extern void* canBeRead(void* arg);
#endif

busModule busModuleSimulation[32];//�����������ǰ��ģ���������Ľṹ�����
int usingModuleNumber = 0;//���������Ͽ���ʹ�õ�ǰ��ģ������������
//̫�鷳����������Ȳ���int callForGetDataModuleIp = 0;//�������ݴ洢���Ѿ������ж��ź��������ȡ�����ݵ�ģ��IP
int countToAbandon = 0;//��Ϊ��Ҫ����һЩ֡��һ������
int initSuccessFlag = 0;
int TriggerSig1 = 0;
int TriggerSig2 = 0;

/*
 * Method:    getModuleIP
 * ʹ�õײ㺯�����а���ip��ȡ��û��ʹ��api������
*/
int getModuleIP(int boardNumber){
  return (int)Module.Module_Info_p[boardNumber].mod_ip;//�����ض����ӵ�ip��ַ
}

/*
 * Method:    creatFrontModuleSimulation
 * ���ݰ�����������ǰ��ģ������������
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
 * ʹ�õײ㺯�����а���������ȡ��û��ʹ��api������
*/
int getModuleNum(){
  return (int)Module.number;//����ģ��������
}

/*
 * Method:    getModuleIP
 * ʹ�õײ㺯�����а���ip��ȡ��û��ʹ��api������
*/
void initBusModules(){
	int moduleNum = 0;//�½�������Ű�������
	initModuleInfo();
	
	// #ifdef HOST_TEST
	// pthread_t pid;
	// pthread_create(&pid,NULL,canBeRead,0);
	// #endif
	
	moduleNum = getModuleNum();//��ȡ������������ֵ
	if(moduleNum > 0 && moduleNum < 32){//ֻ���ڰ���������������Χ�ڲŽ��г�ʼ��
		creatFrontModuleSimulation(moduleNum);//����ǰ��ģ������������ɳ�ʼ������
		initSuccessFlag = 1;
	}else{
		printf("!!!!!!init bus modules failed failed failed!!!!!!----------Linux\n");
	}
}

/*
 * Method:    putDataToModule
 * ���յ������ݷŵ�ģ���ǰ��ģ������������
*/
void putDataToModule(int ipNumber, char* pDataHead, int length){
	int count = 0;
	busDataNode* temp=NULL;
	if(m_class_big != 15 && m_retry !=1){
		countToAbandon++;
		if(countToAbandon%100 == 0){
			return;//һ������7�ı������Ǿ�ֱ�ӷ��أ���ʾ������֡���ݣ���ÿ��7�����ݾͻᶪ��һ����
		}
	}	//��ͬ
	while(count < usingModuleNumber){
		if(ipNumber == busModuleSimulation[count].boardIP){//�ҵ�����Ҫ���͵�Ŀ�ĵ�ģ�飬�ͽ����������
			temp=(busDataNode*)malloc(sizeof(busDataNode));
			temp->pdata=pDataHead;
			//temp.pdata = (char *)malloc(length);//��������ݵ�ָ������ڴ�
			//int cntNum = 0;//���ü�������
			//for(;cntNum < length;cntNum++){//ѭ���������ݿ���
			//	*(temp.pdata + cntNum) = *(pDataHead + cntNum);//�������������ݷŵ��ҿ��ٵ��ڴ浱��
			//}
			temp->mdatalen = length;//���ñ��ڵ㵱�д�ŵ����ݵĳ���
			temp->next = NULL;//���ڵ㽫��Ϊtail������������һ���ڵ�ΪNULL
			// pthread_mutex_lock(&(busModuleSimulation[count].lock));
			if(busModuleSimulation[count].dataLength != 0){//�����Ȳ�Ϊ0��ʾ�Ѿ���������
				busModuleSimulation[count].dataTailNode->next = temp;//���½ڵ���ӵ�β�ڵ����
				busModuleSimulation[count].dataTailNode = temp;//��������ӵĽڵ�Ϊβ�ڵ�
				busModuleSimulation[count].dataLength++;
			}else{//������Ϊ0�ͱ�ʾ������ӵ�һ�����ݵ�������
				busModuleSimulation[count].dataHeadNode = temp;//��������ӵĽڵ�Ϊͷ���
				busModuleSimulation[count].dataTailNode = temp;//��������ӵĽڵ�Ϊβ�ڵ�
				busModuleSimulation[count].dataLength=1;
			}
			pthread_mutex_unlock(&(busModuleSimulation[count].lock));
			temp=NULL;
			return;//����������ݽڵ��ֱ�ӷ��أ���ʾ����������
		}
		count++;
	}
	printf("!!!!!!put data to module failed failed failed!No right ip number!!!!!----------Linux\n");
}

/*
 * 1��
 * Method:    isModuleDataEmpty
 * �ж�ĳ��ģ���ǰ��ģ�������������Ƿ�������
 * param��ģ��IP
 * return��-1��ʾ�ǿգ�1��ʾ��
*/
int isModuleDataEmpty(int ipNumber){
	int count = 0;
	while(count < usingModuleNumber){
		if(ipNumber == busModuleSimulation[count].boardIP){
			//pthread_mutex_lock(&(busModuleSimulation[count].lock));
			if(busModuleSimulation[count].dataLength != 0){//�����Ȳ�Ϊ0��ʾ�Ѿ���������
			//	pthread_mutex_unlock(&(busModuleSimulation[count].lock));
				return -1;//���������򷵻�-1��ʾ�ǿ�
			}else{
				//pthread_mutex_unlock(&(busModuleSimulation[count].lock));
				return 1;//�������򷵻�1��ʾΪ��
			}
		}
		count++;
	}
	return 1;//����˴����ʾû�����ip��ַ���򷵻�1��ʾΪ��
}

/*
 * 2��
 * Method:    isDataEmpty
 * �ж�ģ���ǰ��ģ�������������Ƿ�������
 * return��-1��ʾ�ǿգ�1��ʾ��
*/
int isDataEmpty(){
	int count = 0;
	while(count < usingModuleNumber){
		if(busModuleSimulation[count].dataLength != 0){//�����Ȳ�Ϊ0��ʾ�Ѿ���������
			return -1;//���������򷵻�-1��ʾ�ǿ�
		}
		count++;
	}
	return 1;//����˴����ʾ����ģ�鶼û�����ݣ��򷵻�1��ʾΪ��
}


/*
 * Method:    getDataFromModule
 * ��ȡģ���ǰ��ģ�����������е����ݣ��ŵ������ָ�뵱��
 * return��-1��ʾ��ȡʧ�ܣ�1��ʾ�ɹ�
*/
int getDataFromModule(char** pDataGotHead, int* gotLength){
	int count = 0;
	while(count < usingModuleNumber){
//		if(ipNumber == busModuleSimulation[count].boardIP){
		// pthread_mutex_lock(&(busModuleSimulation[count].lock));
		if(busModuleSimulation[count].dataLength != 0){//�����Ȳ�Ϊ0��ʾ�Ѿ���������
			*gotLength = busModuleSimulation[count].dataHeadNode->mdatalen;//�������ݳ���ֵ������
			*pDataGotHead = busModuleSimulation[count].dataHeadNode->pdata;//��ͷ�ڵ������ָ�븳���Է���ָ��

			if(busModuleSimulation[count].dataLength == 1){//������Ϊ1�ͱ�ʾ�Ѿ�ͷβ�ڵ���ͬһ��
				//busModuleSimulation[count].dataHeadNode.pdata = NULL;//����һ������֮���Ѿ�û��������
				//busModuleSimulation[count].dataHeadNode.mdatalen = 0;//����һ������֮���Ѿ�û��������

				free(busModuleSimulation[count].dataHeadNode);
				busModuleSimulation[count].dataHeadNode=NULL;
				busModuleSimulation[count].dataTailNode=NULL;
				busModuleSimulation[count].dataLength=0;
				//busModuleSimulation[count].dataHeadNode.next = NULL;//����һ������֮���Ѿ�û��������
				//busModuleSimulation[count].dataTailNode.pdata = NULL;//����һ������֮���Ѿ�û��������
				//busModuleSimulation[count].dataTailNode.mdatalen = 0;//����һ������֮���Ѿ�û��������
				//busModuleSimulation[count].dataTailNode.next = NULL;//����һ������֮���Ѿ�û��������
			}else{//�����Ȳ�Ϊ1�ͱ�ʾͷβ�ڵ㲻��ͬһ��
				busDataNode* temp;
				temp=busModuleSimulation[count].dataHeadNode;
				busModuleSimulation[count].dataHeadNode=temp->next;
				// free(temp); �ýڵ��Ǳ���ȡ�Ľڵ㣬�����������ͷ�
				temp=NULL;
				busModuleSimulation[count].dataLength--;
				//temp = busModuleSimulation[count].dataHeadNode;//��ȡͷ�ڵ㱸��
				//busModuleSimulation[count].dataHeadNode.next = NULL;//��ԭ����ͷ�ڵ�������Ľڵ�Ͽ����˴���Ӱ��temp��ֵ��
				//busModuleSimulation[count].dataHeadNode = *(temp.next);//��ԭ���ĵڶ��ڵ�����Ϊͷ�ڵ�
			}
			// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
			return 1;//�����������أ��򷵻�1��ʾ�ɹ�
		}
//			else{
//				return -1;//�������򷵻�-1��ʾ��ȡʧ��
//			}
//		}
		//  pthread_mutex_unlock(&(busModuleSimulation[count].lock));
		count++;
	}
	return -1;//����˴����ʾû�����ip��ַ���򷵻�-1��ʾ��ȡʧ��
}

/*
 * Method:    getModuleRegisterBuf
 * ��ȡģ���ǰ��ģ�����������еļĴ������ݣ��ŵ������ָ�뵱��
 * return��-1��ʾ��ȡʧ�ܣ�1��ʾ�ɹ�
*/
int getModuleRegisterBuf(int ipNumber, char** gotRegisterBuf, unsigned int *bufLength){
	int count = 0;
	while(count < usingModuleNumber){
		if(ipNumber == busModuleSimulation[count].boardIP){
			// pthread_mutex_lock(&(busModuleSimulation[count].lock));
			if(busModuleSimulation[count].registerBuf != NULL){//�Ĵ���ֵ��Ϊ-1��ʾ����ֵ���Ա���ȡ
				*gotRegisterBuf = busModuleSimulation[count].registerBuf;//���Ĵ���ֵ�����Ǹ�ָ����
				*bufLength = busModuleSimulation[count].bufLength;	//��ȡ���Գ���
				busModuleSimulation[count].registerBuf = NULL;//����ȡ֮�������ΪĬ��ֵ
				busModuleSimulation[count].bufLength = 0;	//���Գ�������
				// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
				return 1;//�����������أ��򷵻�1��ʾ�ɹ�
			}else{
				// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
				return -1;//�������򷵻�-1��ʾ��ȡʧ��
			}
		}
		count++;
	}
	return -1;//����˴����ʾû�����ip��ַ���򷵻�-1��ʾ��ȡʧ��
}

/*
 * Method:    setModuleRegisterBuf
 * ���ݴ���Ĵ���ֵ����ģ���ǰ��ģ�����������еļĴ�������
 * return��-1��ʾ����ʧ�ܣ�1��ʾ�ɹ�
*/
int setModuleRegisterBuf(int ipNumber, char* RegisterBuf,  unsigned int bufLength){
	int count = 0;
	while(count < usingModuleNumber){
		if(ipNumber == busModuleSimulation[count].boardIP){
			char* pRegisterBuf = (char *)malloc(bufLength);//�����ĸ��ֽ������������ֵ����
			for(i=0; i<bufLength; i++){
				*(pRegisterBuf +i) = *(RegisterBuf + i);//1.?????????????
			}	
			// pthread_mutex_lock(&(busModuleSimulation[count].lock));
			busModuleSimulation[count].registerBuf = pRegisterBuf;//���ô�ģ��ǰ��ģ��ļĴ���ֵΪ�����ֵ
			busModuleSimulation[count].bufLength = bufLength;
			// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
			return 1;//�����������ã��򷵻�1��ʾ�ɹ�
		}
		count++;
	}
	return -1;//����˴����ʾû�����ip��ַ���򷵻�-1��ʾ����ʧ��
}

/*
 * Method:    cancelModuleTriggerNum
 * ��ȡģ���ǰ��ģ�����������еĴ����߱�ţ��ŵ������ָ�뵱��
 * return��-1��ʾȡ��ʧ�ܣ�1��ʾ�ɹ�
*/
int cancelModuleTriggerNum(int ipNumber){
	int count = 0;
	while(count < usingModuleNumber){
		if(ipNumber == busModuleSimulation[count].boardIP){
				// pthread_mutex_lock(&(busModuleSimulation[count].lock));
				busModuleSimulation[count].triggerNumber = -1;//���ô�ģ��ǰ��ģ��Ĵ����߱��ֵΪ�����ֵ
				// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
				return 1;//��ɴ�����ȡ�����򷵻�1��ʾ�ɹ�
		}
		count++;
	}
	return -1;//����˴����ʾû�����ip��ַ���򷵻�-1��ʾȡ��ʧ��
}

/*
 * Method:    setModuleTriggerNum
 * ���ݴ��봥���߱��ֵ����ģ���ǰ��ģ�����������еĴ����߱��
 * return��-1��ʾ����ʧ�ܣ�1��ʾ�ɹ�
*/
int setModuleTriggerNum(int ipNumber, int TriggerNum){
	int count = 0;
	while(count < usingModuleNumber){//���ȶ�����ģ�����ѭ������֤�˴����߱�Ż�û��ʹ�ò��ܽ��к��������
		// pthread_mutex_lock(&(busModuleSimulation[count].lock));
		if(busModuleSimulation[count].triggerNumber == TriggerNum){
			// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
			return -1;//һ��������������߱���Ѿ���ĳ��ģ�鵱�������ˣ��Ǿʹ˴����ò��ɹ�
		}
		// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
		count++;//ѭ������
	}
	
	count = 0;
	while(count < usingModuleNumber){//ѭ���ҵ�ip��ַ��Ӧ���Ǹ�������ǰ��ģ��
		if(ipNumber == busModuleSimulation[count].boardIP){
			// pthread_mutex_lock(&(busModuleSimulation[count].lock));
			if(busModuleSimulation[count].triggerNumber != -1){//�����߱��ֵ��Ϊ-1��ʾ�Ѿ�������Ϊ��������
				// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
				return -1;//�Ѿ����ù��ˣ��򷵻�-1��ʾ����ʧ��
			}else{
				busModuleSimulation[count].triggerNumber = TriggerNum;//���ô�ģ��ǰ��ģ��Ĵ����߱��ֵΪ�����ֵ
				// pthread_mutex_unlock(&(busModuleSimulation[count].lock));
				return 1;//�����������ã��򷵻�1��ʾ�ɹ�
			}
		}
		count++;
	}
	return -1;//����˴����ʾû�����ip��ַ���򷵻�-1��ʾ����ʧ��
}

/*
 * Method:    generateModuleTriggerSignal
 * ���ݴ��봥���߱��ֵ����ģ���ǰ��ģ�����������еĴ����߱��
 * return��-1��ʾ����ʧ�ܣ�1��ʾ�ɹ�
*/
void generateModuleTriggerSignal(){
	int count = 0;
	while(count < usingModuleNumber){
		if(1 == busModuleSimulation[count].triggerNumber){//���������1�Ŵ��ڣ��ǾͲ�����Ӧ���û��ź�
			kill(0,SIGUSR1); //����SIGUSR1�ź�
			TriggerSig1 = 1;
		}
		if(2 == busModuleSimulation[count].triggerNumber){//���������2�Ŵ��ڣ��ǾͲ�����Ӧ���û��ź�
			kill(0,SIGUSR2); //����SIGUSR2�ź�
			TriggerSig2 = 1;
		}
		count++;
	}
}

/*
 * Method: handleSimulationData
 * parameter:	oldValue--Ҫ�滻����Ϣ����
 * 				newValue--�����滻����Ϣ����
 * 				newDataValueLen--����Ϣ���ݵĳ���
 */

void handleSimulationData(char *oldValue, char* newValue, int newDataValueLen){
	char* data = NULL;
	int *dataLength = 0;
	int count = 0;
	while(count < usingModuleNumber){
		//		ɨ���������ģ��
		if(busModuleSimulation[count].dataLength != 0){//�����Ȳ�Ϊ0��ʾ�Ѿ���������
			dataLength = &(busModuleSimulation[count].dataHeadNode->mdatalen);//�������ݳ���ֵ������
			data = busModuleSimulation[count].dataHeadNode->pdata;//��ͷ�ڵ������ָ�븳���Է���ָ��

			MID_BITS *temp_node = (MID_BITS *)data;
 			paraseMessageType(*temp_node);	//������Ϣ

			int temp_add = temp_node->m_dst_addr;
			temp_node->m_dst_addr = temp_node->m_src_addr;
			temp_node->m_src_addr = temp_add;		//����Դ��ַ��Ŀ�ĵ�ַ,������

			int j = 0;
			int isSame = 1;
			//ƥ��ԭ������Ϣ����
			while( isSame && *(oldValue + j) != 0 && j + 4 < *dataLength){
				if(*(data + 4 + j) != *(oldValue + j)){
					isSame = 0;
				}
				j++;
			}
			j = 0;
			//ƥ��ɹ����滻
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
