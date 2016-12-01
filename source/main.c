
#include "main.h"

int main(void){
    
    int pDataHead = 0x22000080;
    char dataValue[] = "Hello";
    char newValue[] = "world";
    char *pDataSend = NULL;
    char *pReceiveData = NULL;
    int DataLenS = 9;
    int newDataValueLen = 5;
    int i=0;
    int j = 0;
    int count = 0;
    unsigned int clockTime = 0;
    unsigned int triggerNumber = 1;
    int containHead = 0;

    initBusModules();
    setModuleRegisterBuf(HOST_IP, dataValue, 5);
    setModuleTriggerNum(HOST_IP, triggerNumber);

    //synchronize the time of all modules
    // syncClockTime(clockTime);

    while(1){
        if(count >= 10){
            count = 0;
            j = 0;
            pDataSend = (char*)malloc(sizeof(char) * DataLenS);
            while(*(dataValue + j) != 0){
                *(pDataSend + 4 + j) = *(dataValue+j);
                j++;
            }
            pDataSend[3] = 0x22;
            pDataSend[2] = 0x00;
            pDataSend[1] = 0x00;
            pDataSend[0] = 0x80;
            // *(pDataSend) = pDataHead; 
            
            sendDataToIbusSimulation((char*)pDataSend, DataLenS);
        }
        count ++;
       
        // char *pReceiveData = (char*)malloc(sizeof(char) * DataLenS);
        int DataLenR = 0;
        if(isDataEmpty() < 1){
            handleSimulationData(dataValue, newValue, newDataValueLen);
            // transeDataFromHostSimulation(&pDataSend, &DataLenS);	//再通过主机转发给其他虚拟模块
            receiveDataFromIbusSimulation(&pReceiveData, &DataLenR);
            containHead = 1;
            printMessage(pReceiveData, DataLenR, containHead);
        }
        
        generateModuleTriggerSignal();
        if(TriggerSig1 == 1){
            //event1 happen
            printf("Event 1 is triggered!!!\n");
            if( 1 == getModuleRegisterBuf(HOST_IP, &pReceiveData, &DataLenR) ){
                containHead = 0;
                printMessage(pReceiveData, DataLenR, containHead);
            }
        }

        if(TriggerSig2 == 1){
            //event2 happen
            printf("Event 2 is triggered!!!\n");
        }
        
    }
    printf("Hello world");
    return 0;
}

void printMessage(char* string, unsigned int stringLen, int containHead){
    int i;
    if( containHead ){
        for(i=0; i<4; i++){
            printf("The head of the message is:");
            printf("%x ", *(string + i));
            printf("\n");
        }
        for(; i<stringLen; i++){
            printf("The value of the message is:");
            printf("%c", *(string + i));
            printf("\n");
        }
    }else{
        for(i=0; i<stringLen; i++){
             printf("The value of the data is:");
            printf("%c", *(string + i));
            printf("\n");
        }
    }
    
}