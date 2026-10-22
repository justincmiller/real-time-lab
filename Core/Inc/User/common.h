#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "main.h"
#include "util.h"

#include "FreeRTOS.h"
#include "task.h"

#define PRINT_BUF (96U)
#define UART_TIMEOUT (100U)

// UART Handle - must be initialized
extern UART_HandleTypeDef huart2;

static const uint16_t STACK_DEPTH = configMINIMAL_STACK_SIZE + 100;
static const UBaseType_t BASE_PRIORITY = tskIDLE_PRIORITY + 2;
static const TickType_t DELAY_1S = pdMS_TO_TICKS(1000U);

/**
 * @brief Formats and prints a message to the serial console.
 *
 * Accepts printf-style arguments, formats them into a fixed local buffer, and outputs the
 * result using print_str(). Messages exceeding the buffer capacity will be truncated.
 *
 * @param fmt Printf-style format string.
 * @param ... Values to substitute into the format string.
 *
 * @note Do not call from an ISR.
 */
static inline void print_fmt(const char *fmt, ...) {
    char buffer[PRINT_BUF];
    va_list args;

    va_start(args, fmt);
    int result = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (result >= 0)
        print_str(buffer);
}

/**
 * @brief Prints a fatal error message and halts execution.
 *
 * @param msg Error message to display.
 *
 * @note Do not call from an ISR.
 */
static inline void fatalError(const char *msg) {
    char buffer[PRINT_BUF];

    int len = snprintf(buffer, sizeof(buffer), "Fatal Error: %s\r\n", msg);
    if (len > 0) {
        size_t size = (size_t)len < sizeof(buffer) ? (size_t)len : sizeof(buffer) - 1;
        HAL_UART_Transmit(&huart2, (uint8_t *)buffer, (uint16_t)size, UART_TIMEOUT);
    }

    Error_Handler();
}