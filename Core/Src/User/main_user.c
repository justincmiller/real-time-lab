/*
 * main_user.c
 *
 *
 */

#include <stdio.h>

#include <stdlib.h>
#include <time.h>
//STM32 generated header files
#include "main.h"

//User generated header files
#include "User/main_user.h"
#include "User/util.h"
#include "User/common.h"

//Required FreeRTOS header files
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

uint32_t viewCount = 0;
//#define SEND_

QueueHandle_t xQueue;
char receiver_string[] = "Received = 000\r\n";
char debug_str[64];

void increment(int num)
{
	int buffer = 0;
	buffer = viewCount;
	buffer += num;
	for(int i = 0; i <100000; i++);
	viewCount = buffer;
}

/******************************************************************************
******************************************************************************/
static void vSenderTask( void *pvParameters )
{
	uint32_t incrementValue;
	int param = *(int*)(pvParameters);
	int seed = 12345;
	srand(seed);

	for( ;; )
	{
		incrementValue = (uint32_t)(rand() % 100 + 1);;

		print_fmt("%s sender %d sending update: %lu" ANSI_RESET "\r\n", (param == 100) ? ANSI_RED : ANSI_BLUE, (param == 100) ? 1 : 2, incrementValue);
		increment(incrementValue);
		
		vTaskDelay(pdMS_TO_TICKS(50U));
	}
}


/******************************************************************************
******************************************************************************/
static void vReceiverTask( void *pvParameters )
{

	for( ;; )
	{
		sprintf(debug_str, "Current Total View Count: %lu\r\n", viewCount);
		print_str(debug_str);
		vTaskDelay(500);
	}
}


uint32_t param1 = 100;
uint32_t param2 = 200;


void main_user(){
	util_init();
	print_fmt(ANSI_RESET "=====================STARTING YOUTUBE SERVER -- NO QUEUE======================\r\n");

	// sender tasks.
	xTaskCreate( vSenderTask, "Sender1", 1000, &param1, 1, NULL );
	xTaskCreate( vSenderTask, "Sender2", 1000, &param2, 1, NULL );

	// Receiver task
	xTaskCreate( vReceiverTask, "Receiver", 1000, NULL, 2, NULL );

	// Start the scheduler so the created tasks start executing.
	vTaskStartScheduler();
	

	while(1);

}
