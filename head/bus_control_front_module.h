#ifndef _BUS_CONTROL_FRONT_MODULE_HEADER_
#define _BUS_CONTROL_FRONT_MODULE_HEADER_

#include "bus_control_general_lib.h"
#include "mfi_module_info.h"
#include "mfi_define.h"

//���ݽڵ�ṹ��
typedef struct dataNode{
	char* pdata;//��������
	int mdatalen;//��ʾ���ݳ���
	struct dataNode *next;//����ָ�룬ָ����һ���ڵ�
}busDataNode;

//ģ��ǰ��ģ��Ľṹ��
typedef struct busSimulationModule{
	int working;//����˵�����ģ���Ƿ���ڣ�1�Ǵ��ڣ�-1�ǲ�����
	busDataNode* dataHeadNode;//��������������������ͷ�ڵ㣬δʹ��ʱ���ṹ������ֵ��Ĭ������Ϊnull
	busDataNode* dataTailNode;//��������������������β�ڵ㣬δʹ��ʱ���ṹ������ֵ��Ĭ������Ϊnull
	int dataLength;//���������������ĳ���
	int boardIP;//ǰ��ģ���IP��δʹ��ʱĬ������Ϊ-1
	char* registerBuf;//�Ĵ�����ֵ��δʹ��ʱĬ������ΪNULL
	int triggerNumber;//�������Ĵ����߱�ţ��޴���ʱĬ������Ϊ-1
	unsigned int bufLength;
	unsigned int clockTime;
	// pthread_mutex_t lock;

}busModule;

typedef struct moduleTime{
	unsigned int globalTime;		//ȫ��ʱ������ı��ؼĴ�
	unsigned int subWindowStart[SUBWIN_NUM];	//�̶�ʱ���Ӵ�
	unsigned int subWindowLength[SUBWIN_NUM];		//�̶�ʱ���Ӵ�����
	unsigned int subWinEN[SUBWIN_NUM];	//�̶��Ӵ���Ӧ��ģ��IP
	// unsigned int windowLength;		//����ʱ�䴰����
	// int subWindowIndex;	//��ʹ�õ��Ӵ�
}moduleSubWin;

#define fixedTimeWindowLen 50

//�����������������ļ�ʹ��
void initBusModules();//��ʼ��������ǰ��ģ�������������IP��ʼ��
int isDataEmpty();//�ж��������Ƿ������ݣ�Ҳ�����ж�����ģ�飩
void putDataToModule(int ipNumber, char* pDataHead, int length);//���յ������ݷŵ�ָ��IP������ǰ��ģ��ĵ���
int getDataFromModule(char** pDataGotHead, int* gotLength);//��ȡ��Ҫ�����͵����ݺͳ���
int setModuleRegisterBuf(int ipNumber, char* RegisterBuf, unsigned int bufLength);//���յ�������ֵ���õ���ӦIP��ַ��ģ����
int getModuleRegisterBuf(int ipNumber, char** gotRegisterBuf, unsigned int *bufLength);//����ӦIP��ַ��ģ���е�����ֵ��ȡ����
int setModuleTriggerNum(int ipNumber, int TriggerNum);//���յ��Ĵ����߱��ֵ���õ���ӦIP��ַ��ģ����
int cancelModuleTriggerNum(int ipNumber);//����ӦIP��ַ��ģ���еĴ����߱��ֵȡ����
void generateModuleTriggerSignal();//��Ϊ������ѭ������ģ����û��ź�
void handleSimulationData();
void timeGoesBy();
int getHostTime();
unsigned int getModuleTime(int ipNumber);
void syncModulesTime(unsigned int syncTime);
void setTimeWindow(unsigned int *subWindowStart, unsigned int*subWindowLength, int *subWinEN);
void setSubWindow( unsigned int *windowLength, int *subWinEN );
int canBeSend();

extern int TriggerSig1;
extern int TriggerSig2;
extern unsigned int globalTime;

#endif