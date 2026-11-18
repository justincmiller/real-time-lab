/*
 * main_user.c
 *
 *  Created on: Aug 8, 2022
 *      Author: Andre Hendricks
 */

#include <stdio.h>

//STM32 generated header files
#include "main.h"

//User generated header files
#include "User/main_user.h"
#include "User/util.h"
#include "User/USART_Driver.h"
#include "User/Comm_Datalink.h"
#include "User/SensorPlatform.h"
#include "User/SensorController.h"
#include "User/common.h"

//Required FreeRTOS header files
#include "FreeRTOS.h"
#include "task.h"

#define SENSORPLATFORM_MODE 0
#define SENSORCONTROLLER_MODE 1

//User must state the mode this code will run in. SENSORPLATFORM_MODE or SENSORCONTROLLER_MODE
#define CODE_MODE SENSORPLATFORM_MODE
//#define CODE_MODE SENSORCONTROLLER_MODE

void main_user(){
	util_init();

	initialize_sensor_datalink();
#if CODE_MODE == SENSORCONTROLLER_MODE
	initialize_hostPC_datalink();
#endif

#if CODE_MODE == SENSORCONTROLLER_MODE
	xTaskCreate(HostPC_RX_Task, "HostPC_RX_Task", STACK_DEPTH, NULL, BASE_PRIORITY, NULL);
	xTaskCreate(SensorPlatform_RX_Task, "SensorPlatform_RX_Task", STACK_DEPTH, NULL, BASE_PRIORITY, NULL);
	xTaskCreate(SensorControllerTask, "Sensor_Controller_Task", STACK_DEPTH, NULL, BASE_PRIORITY, NULL);
#elif CODE_MODE == SENSORPLATFORM_MODE
	xTaskCreate(SensorPlatformTask, "Sensor_Platform_Task", STACK_DEPTH, NULL, BASE_PRIORITY, NULL);
#endif
	vTaskStartScheduler();

	while (1);
}
