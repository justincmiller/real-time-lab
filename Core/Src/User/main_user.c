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

//Required FreeRTOS header files
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// Utility header
#include "common.h"

static SemaphoreHandle_t buttonSm = NULL;

/******************************************************************************
This is a task handler that prints periodically.
No changes are required.
******************************************************************************/
static void main_task(void *param) {
	uint32_t main_counter = 0;

	while (1) {
        print_fmt("Main task loop executing\r\n"
			"Main task iteration: 0x%08lx\r\n", main_counter++);

        vTaskDelay(DELAY_1S);
    }
}

/******************************************************************************
This task handler needs to be completed.
It should toggle the LED on a rising edge.  Use the call to
"HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_A);
And send a message to the console of the host computer to indicate
that a new loop of the task_led has been run.
The message should show the number of times it has been run.
This task handler should not use any delays
******************************************************************************/
static void task_led(void *params) {
	uint32_t LEDCount = 0;

	while (1) {
		if (xSemaphoreTake(buttonSm, portMAX_DELAY) == pdTRUE) {
			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
			print_fmt(ANSI_BOLD ANSI_BR_GREEN "LED task loop executing\r\n"
				"LED task iteration: 0x%08lx" ANSI_RESET "\r\n", LEDCount++);
		}
	}
}


/******************************************************************************
This is the callback function for onboard blue button.
To be completed.
******************************************************************************/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == GPIO_PIN_13 && buttonSm != NULL) {
		BaseType_t woken = pdFALSE;
		xSemaphoreGiveFromISR(buttonSm, &woken);
		portYIELD_FROM_ISR(woken);
	}
}

/******************************************************************************
main to be completed.
Make sure that the buttons and interrupts are configured.
Create a binary semaphore after initialization.
Additionally to the main_task, if the semaphore is created, create a second
task with the handler task_led.
Don't forget to start the scheduler!
******************************************************************************/
void main_user() {
	util_init();

	buttonSm = xSemaphoreCreateBinary();
	if (buttonSm == NULL)
		fatalError("Failed to create binary semaphore");

	if (xTaskCreate(main_task, "Main Task", STACK_DEPTH, NULL, BASE_PRIORITY, NULL) != pdPASS)
		fatalError("Failed to create main task");

	if (xTaskCreate(task_led, "LED task", STACK_DEPTH, NULL, BASE_PRIORITY, NULL) != pdPASS)
		fatalError("Failed to create LED task");

	vTaskStartScheduler();

	fatalError("Failed to start FreeRTOS scheduler");
}
