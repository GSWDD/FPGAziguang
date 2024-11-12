/*
 * yolo_load_param.h
 *
 *  Created on: Oct 31, 2024
 *      Author: 10261
 */
#include "system.h"

int BiasAddr[15];
int ActAddr[15];
int WeightAddr[15];

// 需要确认
//#define BIAS_STORE_BASEADDR     0x2000000
//#define ACT_STORE_BASEADDR      0x2004000
//#define WEIGHT_STORE_BASEADDR   0x2005000

#define BIAS_STORE_BASEADDR     0x400000
#define ACT_STORE_BASEADDR      0x404000
#define WEIGHT_STORE_BASEADDR   0x405000

#ifndef APP_YOLO_LOAD_PARAM_H_
#define APP_YOLO_LOAD_PARAM_H_

void UART_Init();
void SD_Init();
void Trap_Init();
void FPIOA_Init();
void Time_Init();
void load_SD_param();

#endif /* APP_YOLO_LOAD_PARAM_H_ */
