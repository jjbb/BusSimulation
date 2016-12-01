/*
 * BusControlSimulation4Lin.h
 *
 *  Created on: 2016-11-23
 *      Author: jiabin
 */

#ifndef BUSCONTROLSIMULATION4LIN_H_
#define BUSCONTROLSIMULATION4LIN_H_

void sendDataToIbusSimulation(char *pDataSend, int DataLenS);	
void receiveDataFromIbusSimulation(char **pDataReceive, int *DataLenR); 
void syncClockTime(unsigned int clockTime);
#define jint int
#endif /* BUSCONTROLSIMULATION4LIN_H_ */
