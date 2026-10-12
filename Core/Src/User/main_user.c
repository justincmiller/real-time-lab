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

#include "common.h"

#define LED_A_PORT  (GPIOC)
#define LED_B_PORT  (GPIOC)
#define LED_OB_PORT (GPIOA)

#define BTN_A_PIN  (GPIO_PIN_10)
#define BTN_B_PIN  (GPIO_PIN_12)
#define BTN_OB_PIN (GPIO_PIN_13)

#define LED_A_PIN  (GPIO_PIN_0)
#define LED_B_PIN  (GPIO_PIN_1)
#define LED_OB_PIN (GPIO_PIN_5)

enum {
	BTN_A = (1U << 0),
	BTN_B = (1U << 1)
};


char main_string[50];
uint32_t main_counter = 0;

char task_string[50];
uint32_t task_id = 0;

volatile uint8_t triggers = 0;

static void PrintTaskID(void *params) {
	int *id = (int *)params;
	do {
		sprintf(task_string, "Task %lu is printing", *id);
		print_str(task_string);
		vTaskDelay(DELAY_1S);
	} while (1);
}

static void main_task(void *params) {
	char ibuff[50];
	TaskHandle_t xHandle[50];
	/*Variable used to keep track of sub tasks created with button*/

	size_t val;

	do {
		print_str("Main task loop executing ");

		// Update hexadecimal 32-bit integer in string, and print it
		sprintf(main_string, "Main task iteration: 0x%08x\r\n", (unsigned int)main_counter++);
		print_str(main_string);

		// Create new task if button A was pressed
		if (triggers & BTN_A) {
			if (xTaskCreate(&PrintTaskID, (const char*)"subtask", STACK_DEPTH,
				&task_id, BASE_PRIORITY, &xHandle[task_id]) != pdPASS) {
				sprintf(ibuff, "Failed creating new task %lu \r\n", task_id);
				print_str(ibuff);
			} else {
				sprintf(ibuff, "Created a new task %lu \r\n", task_id);
				print_str(ibuff);
				task_id++;
			}
			triggers &= ~BTN_A;
		// Delete new task if button B was pressed
		} else if ((triggers & BTN_B) && task_id > 0) {
			/*Decrement taskid counter*/
			if (task_id > 0) task_id--;
			vTaskDelete(xHandle[task_id]);
			sprintf(ibuff, "Deleted task new task %lu \r\n", task_id);
			print_str(ibuff);
			triggers &= ~BTN_B;
		}
		val = xPortGetFreeHeapSize() ;
		sprintf(ibuff, "Size = %d Bytes\r\n", val);
		print_str(ibuff);
		vTaskDelay(DELAY_1S);
	} while (1);
}

void HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin) {
	if (GPIO_Pin == BTN_A_PIN) {
		triggers |= BTN_A;
		print_str_ISR("A button pressed\r\n");
		HAL_GPIO_TogglePin(LED_A_PORT, LED_A_PIN);
	} else if (GPIO_Pin == BTN_B_PIN) {
		triggers |= BTN_B;
		print_str_ISR("B button pressed\r\n");
		HAL_GPIO_TogglePin(LED_B_PORT, LED_B_PIN);
	} else if (GPIO_Pin == BTN_OB_PIN) {
		print_str_ISR("Onboard button pressed\r\n");
		HAL_GPIO_TogglePin(LED_OB_PORT, LED_OB_PIN);
	}
}

void main_user() {
	util_init();

	xTaskCreate(main_task, "Main Task", STACK_DEPTH, NULL, BASE_PRIORITY, NULL);

	vTaskStartScheduler();

	while(1);
}
