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
#include "timers.h"

#include "common.h"

enum Timers 
{
	TIMER_1, 
	TIMER_2,
	NUM_TIMERS
};

enum StopLightStates 
{
	STATE_GREEN,
	STATE_ORANGE,
	STATE_RED,
	NUM_STATES
};



TimerHandle_t xTimers[NUM_TIMERS];


char main_string[50];
uint32_t main_counter = 0;


volatile uint8_t triggers = 0;

void StopLightController( TimerHandle_t xTimer)
{
	volatile static enum StopLightStates StopLightContoller_State = STATE_GREEN;
	uint32_t TimerID;
	TimerID = ( uint32_t ) pvTimerGetTimerID(xTimer);
	if(StopLightContoller_State == STATE_GREEN)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
		StopLightContoller_State = STATE_ORANGE;
	}
	else if(StopLightContoller_State == STATE_ORANGE)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
		StopLightContoller_State = STATE_RED;
	}
	else if(StopLightContoller_State == STATE_RED)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
		StopLightContoller_State = STATE_GREEN;
	}
}

static void main_task(void *params) {
	while(1){
		print_str("Main task loop executing\r\n");
		sprintf(main_string,"Main task iteration: 0x%08lx\r\n",main_counter++);
		print_str(main_string);
		vTaskDelay(1000/portTICK_RATE_MS);
	}
}


void main_user() {
	util_init();

	xTimers[TIMER_1] = xTimerCreate("Timer1", 3000, pdTRUE, (void * ) 0, StopLightController);
	xTimerStart(xTimers[TIMER_1], 0);

	xTaskCreate(main_task, "Task 1", STACK_DEPTH, NULL, BASE_PRIORITY, NULL);

	vTaskStartScheduler();

	while(1);
}
