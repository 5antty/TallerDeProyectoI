

#ifndef MANEJO_UART_H
#define MANEJO_UART_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define RX_BUF_SIZE 1024
#define UART_NUM UART_NUM_1

// Variables globales accesibles desde main.c
extern uint8_t uart_buffer[RX_BUF_SIZE];
extern uint8_t uart_data_ready;

void uart_init(void);
int sendData(const char *data);

#endif
