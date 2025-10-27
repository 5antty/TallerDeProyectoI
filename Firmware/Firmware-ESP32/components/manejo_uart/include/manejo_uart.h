

#ifndef MANEJO_UART_H
#define MANEJO_UART_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

uint8_t *BUFFER_RX;

void uart_init(void);
int sendData(const char *logName, const char *data);

#endif
