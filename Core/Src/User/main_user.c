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

#define TogglePedestrianLight(x) (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, (x)))
enum Timers 
{
	STOPLIGHT_TIMER, 
	PEDESTRIAN_TIMER,
	NUM_TIMERS
};

enum StopLightStates 
{
	STATE_GREEN,
	STATE_ORANGE,
	STATE_RED
};

enum pedestrianLightStates 
{
	STATE_OFF,
	STATE_ON
};

// Set timer period to 100ms so that the light toggles 10 times a second, leading to a 5Hz blinking light.
static const TickType_t pedestrianTimerPeriod = pdMS_TO_TICKS(100);


TimerHandle_t xTimers[NUM_TIMERS];

volatile static enum StopLightStates StopLightContoller_State = STATE_GREEN;

volatile static enum pedestrianLightStates pedestrianLight_state = STATE_ON;

char main_string[50];
uint32_t main_counter = 0;


void pedestrianLightBlink(TimerHandle_t xTimer)
{
	pedestrianLight_state = (pedestrianLight_state == STATE_ON) ? STATE_OFF : STATE_ON; 
	TogglePedestrianLight((pedestrianLight_state == STATE_ON) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void StopLightController( TimerHandle_t xTimer)
{
	uint32_t TimerID;
	TimerID = ( uint32_t ) pvTimerGetTimerID(xTimer);
	if(StopLightContoller_State == STATE_GREEN)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
		pedestrianLight_state = STATE_ON;
		StopLightContoller_State = STATE_ORANGE;
	}
	else if(StopLightContoller_State == STATE_ORANGE)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
		xTimerStart(xTimers[PEDESTRIAN_TIMER], 0);
		StopLightContoller_State = STATE_RED;
	}
	else if(StopLightContoller_State == STATE_RED)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
		xTimerStop(xTimers[PEDESTRIAN_TIMER], 0);
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

	xTimers[STOPLIGHT_TIMER] = xTimerCreate("Timer1", 3000, pdTRUE, (void * ) 0, StopLightController);
	xTimerStart(xTimers[STOPLIGHT_TIMER], 0);

	xTimers[PEDESTRIAN_TIMER] = xTimerCreate("Timer2", pedestrianTimerPeriod, pdTRUE, (void * ) 0, pedestrianLightBlink);

	xTaskCreate(main_task, "Task 1", STACK_DEPTH, NULL, BASE_PRIORITY, NULL);

	vTaskStartScheduler();

	while(1);
}
