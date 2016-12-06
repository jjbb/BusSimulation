#include "main.h"

int main(void)
{

    int pDataHead = 0x22000080;
    char dataValue[] = "Hello";
    char newValue[] = "world";
    char *pDataSend = NULL;
    char *pReceiveData = NULL;
    int DataLenS = 0;
    int DataLenR = 0;
    int newDataValueLen = 5;
    int i = 0;
    int j = 0;
    int count = 0;
    unsigned int clockTime = 0;
    unsigned int triggerNumber = 1;
    int containHead = 0;

    initBusModules();
    initSubWinInfo();
    
    // setModuleRegisterBuf(HOST_IP, dataValue, 5);
    // setModuleTriggerNum(HOST_IP, triggerNumber);

    //synchronize the time of all modules
    // syncClockTime(clockTime);

    while (1)
    {
        if (count >= 10)
        {
            count = 0;
            // "0 00000 01000 0 0000000000000 1000100"
            //初始化要发送的数据，"0010001 0000000000000 0 00010 00000 0" + "Hello"; app消息
            //              消息大类和优先级    消息小类    是否重发 目的地址 源地址 单帧     消息内容
            j = 0;
            DataLenS = 9;
            pDataSend = (char *)malloc(sizeof(char) * DataLenS);
            while (*(dataValue + j) != 0)
            {
                *(pDataSend + 4 + j) = *(dataValue + j);
                j++;
            }
            pDataSend[3] = 0x22;
            pDataSend[2] = 0x00;
            pDataSend[1] = 0x00;
            pDataSend[0] = 0x80;
            // *(pDataSend) = pDataHead;

            sendDataToIbusSimulation((char *)pDataSend, DataLenS);
        }

        if(count == 2){
            
        }

        if (count == 4)
        {
            //初始化要发送的数据，"0010100 1000000000000 0 00001 00000 0" + "1|2"; 触发线配置消息
            //              消息大类和优先级    消息小类    是否重发 目的地址 源地址 单帧     消息内容
            j = 0;
            DataLenS = 6;
            pDataSend = (char *)malloc(sizeof(char) * DataLenS);
            pDataSend[5] = 0x00;
            pDataSend[4] = 0x02;
            pDataSend[3] = 0x29;
            pDataSend[2] = 0x00;
            pDataSend[1] = 0x00;
            pDataSend[0] = 0x40;

            sendDataToIbusSimulation((char *)pDataSend, DataLenS);
        }

        if (count == 6)
        {
            //初始化要发送的数据，"1000001 1000000000000 0 00001 00000 0" + "1"; 时间同步消息
            //              消息大类和优先级    消息小类    是否重发 目的地址 源地址 单帧     消息内容
            j = 0;
            DataLenS = 6;
            pDataSend = (char *)malloc(sizeof(char) * DataLenS);
            for(j=0; j<4; j++){
                pDataSend[j + Head_Len] = *( ((char*)(&globalTime)) +j );
            }
            pDataSend[3] = 0x83;
            pDataSend[2] = 0x00;
            pDataSend[1] = 0x00;
            pDataSend[0] = 0x40;

            sendDataToIbusSimulation((char *)pDataSend, DataLenS);
        }

        if (count == 8)
        {
            //初始化要发送的数据，"0010100 1000000000011 0 00001 00000 0" + "0x0001"; 属性配置消息
            //              消息大类和优先级    消息小类    是否重发 目的地址 源地址 单帧     消息内容
            j = 0;
            DataLenS = 9;
            pDataSend = (char *)malloc(sizeof(char) * DataLenS);
            while (*(dataValue + j) != 0)
            {
                *(pDataSend + 4 + j) = *(dataValue + j);
                j++;
            }
            pDataSend[3] = 0x29;
            pDataSend[2] = 0x00;
            pDataSend[1] = 0x30;
            pDataSend[0] = 0x40;

            sendDataToIbusSimulation((char *)pDataSend, DataLenS);
        }

        // char *pReceiveData = (char*)malloc(sizeof(char) * DataLenS);
        if ((count % 2) == 1)
        {
            if (isDataEmpty() < 1)
            {
                handleSimulationData(dataValue, newValue, newDataValueLen);
                receiveDataFromIbusSimulation(&pReceiveData, &DataLenR);
                containHead = 1;
                printf("Read the data:\n");
                printMessage(pReceiveData, DataLenR, containHead);
            }
        }

        generateModuleTriggerSignal();
        if (TriggerSig1 == 1)
        {
            //event1 happen
            printf("Event 1 is triggered!!!\n");
            if (1 == getModuleRegisterBuf(HOST_IP, &pReceiveData, &DataLenR))
            {
                containHead = 0;
                printf("Read the register:\n");
                printMessage(pReceiveData, DataLenR, containHead);
            }
        }

        if (TriggerSig2 == 1)
        {
            //event2 happen
            printf("Event 2 is triggered!!!\n");
            int clockTime = getModuleTime(HOST_IP);
            if(clockTime){
                printf("Read the time of Host Module is: %d\n", clockTime);
            }
            clockTime = getModuleTime(MY_IP);
            if(clockTime){
                printf("Read the time of this Module is: %d\n", clockTime);
            }
        }
        count++;
        timeGoesBy();
    }
    printf("Hello world");
    return 0;
}

void printMessage(char *string, unsigned int stringLen, int containHead)
{
    int i;
    if (containHead)
    {
        printf("The head of the message is:");
        for (i = 0; i < 4; i++)
        {
            printf("0x%02x ", *(string + i));
        }
        printf("\n");

        printf("The value of the message is:");
        for (; i < stringLen; i++)
        {
            if ((*(string + i) > 'a' && *(string + i) < 'z') || (*(string + i) > 'A' && *(string + i) < 'Z'))
            {
                printf("%c", *(string + i));
            }
            else
            {
                printf("0x%02x ", *(string + i));
            }
        }
        printf("\n");
    }
    else
    {
        for (i=0; i < stringLen; i++)
        {
            if ((*(string + i) > 'a' && *(string + i) < 'z') || (*(string + i) > 'A' && *(string + i) < 'Z'))
            {
                printf("%c", *(string + i));
            }
            else
            {
                printf("0x%02x ", *(string + i));
            }
        }
        printf("\n");
    }
}

void initSubWinInfo(){
    unsigned int mSubWindowStart[SUBWIN_NUM];
    unsigned int mSubWindowLength[SUBWIN_NUM];
    int mSubWinEN[SUBWIN_NUM];
    int i=0;
    for(i=0; i<SUBWIN_NUM; i++){
        mSubWindowStart[i] = i * 4;  //每4个基本单位划分一个时间窗
        mSubWindowLength[i] = 2;     //可发送时间窗长度为2
        if( i % 5 == 0){    //每5次可发送一次
            mSubWinEN[i] = 1;
        }else{
            mSubWinEN[i] = 0;
        }
    }
    setTimeWindow(mSubWindowStart, mSubWindowLength, mSubWinEN);
}