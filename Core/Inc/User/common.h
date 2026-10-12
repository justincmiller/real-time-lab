#pragma once

#include <stdint.h>
#include <stddef.h>

#include "FreeRTOS.h"
#include "task.h"

static const uint16_t STACK_DEPTH = configMINIMAL_STACK_SIZE + 100;
static const UBaseType_t BASE_PRIORITY = tskIDLE_PRIORITY + 2;
static const TickType_t DELAY_1S = pdMS_TO_TICKS(1000U);