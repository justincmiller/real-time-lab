/*
 * main_user.c
 *
 *
 */

#include <stdio.h>
#include <stdlib.h> // for random numbers

//STM32 generated header files
#include "main.h"

//User generated header files
#include "User/main_user.h"
#include "User/util.h"

//Required FreeRTOS header files
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

QueueHandle_t xQueue;

/******************************************************************************
******************************************************************************/

static void vSenderTask(void *pvParameters) {
	uint32_t views;
	BaseType_t xStatus;

	while (1) {
		views = (uint32_t)(rand() % 100 + 1);

		xStatus = xQueueSendToBack(xQueue, &views, 0);
		if (xStatus != pdPASS) {
			print_str("Could not send to queue.\r\n");
		}

		vTaskDelay(pdMS_TO_TICKS(50));
	}
}


/******************************************************************************
******************************************************************************/
static void vReceiverTask( void *pvParameters ) {
	char buffer[128];
	uint32_t total = 0;
	uint32_t count = 0;
	uint32_t cumulative = 0;
	uint32_t update;

	const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
	const TickType_t interval = pdMS_TO_TICKS(1000);

	TickType_t dt;
    TickType_t t = xTaskGetTickCount();
	
	while (1) {
		/* Verify if queue is empty. */
		if (uxQueueMessagesWaiting(xQueue) != 0) {
			print_str( "Queue is not empty!\r\n" );
		}
		/* Receive data from the queue. */
		if (xQueueReceive(xQueue, &update, xTicksToWait) == pdPASS) {
			total += update;
			cumulative += update;
			count++;
		} else {
			// Data was not received from the queue even after waiting
			print_str( "Could not receive from the queue.\r\n" );
		}

		dt = xTaskGetTickCount();
		if ((TickType_t)(dt - t) >= interval) {
            snprintf(buffer, sizeof(buffer), "View update: %lu | Total views: %lu | Messages: %lu\r\n",
				(unsigned long)cumulative,
            	(unsigned long)total,
                (unsigned long)count);

            print_str(buffer);

            t = dt;
			cumulative = 0;
            count = 0;
        }
	}
}

void main_user(){
	util_init();

	// initialize random generator
	srand(12345);

	// The queue is created
	xQueue = xQueueCreate( 5, sizeof(uint32_t) );
	if( xQueue != NULL )
	{
		// sender tasks.
		xTaskCreate( vSenderTask, "Sender1", 1000, NULL, 1, NULL );
		xTaskCreate( vSenderTask, "Sender2", 1000, NULL, 1, NULL );

		// Receiver task
		xTaskCreate( vReceiverTask, "Receiver", 1000, NULL, 2, NULL );

		// Start the scheduler so the created tasks start executing.
		vTaskStartScheduler();
	}

	while(1);

}
