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

//Required FreeRTOS header files
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

uint32_t viewCount = 0;
//#define SEND_

QueueHandle_t xQueue;
char receiver_string[] = "Received = 000\r\n";
char debug_str[64];
char debug_str2[64];

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
	int32_t lValueToSend;
	BaseType_t xStatus;
	int seed = ((int32_t) pvParameters == 100) ? 12345: 67890;
	srand(seed);

	
	
	for( ;; )
	{
#ifdef SEND_
		lValueToSend = ( int32_t ) pvParameters;
		
		xStatus = xQueueSendToBack( xQueue, &lValueToSend, 0 );
		if( xStatus != pdPASS )
		{
			print_str( "Could not send to the queue.\r\n" );
		}
#else
		lValueToSend = rand() % 10000;

		sprintf(debug_str2, "sending: %lu\r\n", lValueToSend);
		print_str(debug_str2);
		increment(lValueToSend);

		sprintf(debug_str2, "%lu\r\n", viewCount);
		print_str(debug_str2);
#endif
		
		vTaskDelay(pdMS_TO_TICKS(100U));
	}
}


/******************************************************************************
******************************************************************************/
static void vReceiverTask( void *pvParameters )
{
	int32_t lReceivedValue;
	BaseType_t xStatus;
	const TickType_t xTicksToWait = pdMS_TO_TICKS( 100 );

	for( ;; )
	{
#ifdef SEND_
		/* Verify if queue is empty. */
		if( uxQueueMessagesWaiting( xQueue ) != 0 )
		{
			print_str( "Queue is not empty!\r\n" );
		}
		/* Receive data from the queue. */
		xStatus = xQueueReceive( xQueue, &lReceivedValue, xTicksToWait );
		if( xStatus == pdPASS )
		{
			/* Data was successfully received from the queue, print out the received
			value. */
			sprintf(&receiver_string[11], "%ld", lReceivedValue);
			receiver_string[14] = '\r';
			receiver_string[15] = '\n';
			print_str(receiver_string);
		}
		else
		{
			// Data was not received from the queue even after waiting
			print_str( "Could not receive from the queue.\r\n" );
		}
#else
			sprintf(debug_str, "%lu\r\n", viewCount);
			print_str(debug_str);
			vTaskDelay(500);
#endif
	}
}


void main_user(){
	util_init();

	// The queue is created
	xQueue = xQueueCreate( 5, sizeof( int32_t ) );
	if( xQueue != NULL )
	{
		// sender tasks.
		xTaskCreate( vSenderTask, "Sender1", 1000, ( void * ) 100, 1, NULL );
		xTaskCreate( vSenderTask, "Sender2", 1000, ( void * ) 200, 1, NULL );

		// Receiver task
		xTaskCreate( vReceiverTask, "Receiver", 1000, NULL, 2, NULL );

		// Start the scheduler so the created tasks start executing.
		vTaskStartScheduler();
	}

	while(1);

}
