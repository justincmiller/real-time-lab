/*
 * main_user.c
 *
 *  Created on: Aug 8, 2022
 *      Author: Andre Hendricks
 */

#include <stdint.h>
#include <stdlib.h>

#include "main.h"
#include "User/main_user.h"
#include "User/util.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "common.h"

static EventGroupHandle_t xEventGroup = NULL;

static void PrintTaskID(void *params);

static void main_task(void *param) {
    uint32_t main_counter = 0;
    int taskcount = 0;
    TaskHandle_t xHandle[50] = {0};

    while (1) {
        print_fmt("Main task iteration: 0x%08lx\r\n", main_counter++);

        // Create a new task when Button A is pressed
        if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_10) == GPIO_PIN_RESET) {
            BaseType_t result = xTaskCreate(PrintTaskID, "Subtask",
                STACK_DEPTH, (void *)taskcount, BASE_PRIORITY, &xHandle[taskcount]);

            if (result != pdPASS)
                print_fmt("Failed creating new task %d\r\n", taskcount);
            else
                print_fmt("Created task %d\r\n", taskcount++);
        }

        // Delete the newest task when Button B is pressed
        else if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_12) == GPIO_PIN_RESET && taskcount > 0) {
            taskcount--;
            vTaskDelete(xHandle[taskcount]);
            print_fmt("Deleted task %d\r\n", taskcount);
        }

        print_fmt("Size = %d Bytes\r\n", xPortGetFreeHeapSize());

        // Reset synchronization for the next cycle
        xEventGroupClearBits(xEventGroup, 0xFFFF);
        xEventGroupSetBits(xEventGroup, 1U << 0);

        vTaskDelay(DELAY_1S);
    }
}

/******************************************************************************
******************************************************************************/
static void PrintTaskID(void *params) {
    int IDval = (int)params;

    while (1) {
        xEventGroupWaitBits(xEventGroup, 1U << IDval, pdTRUE, pdTRUE, portMAX_DELAY);
        print_fmt(ANSI_BOLD ANSI_BR_GREEN, "Task %d printing" ANSI_RESET "\r\n", IDval);

        // Allow the next task to execute
        xEventGroupSetBits(xEventGroup, 1U << (IDval + 1));
        vTaskDelay(rand() % 1000);
    }
}

void main_user(void) {
    util_init();

    xEventGroup = xEventGroupCreate();
    if (xEventGroup == NULL)
        fatalError("Failed to create event group");

    if (xTaskCreate(main_task, "Main Task", STACK_DEPTH, NULL, BASE_PRIORITY, NULL) != pdPASS)
        fatalError("Failed to create main task");

    vTaskStartScheduler();

    fatalError("Failed to start scheduler");
}
