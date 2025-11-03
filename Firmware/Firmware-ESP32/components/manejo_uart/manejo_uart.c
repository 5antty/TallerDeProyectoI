
/* UART asynchronous example, that uses separate RX and TX tasks

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "manejo_uart.h"
#include "string.h"
const char *TAG_UART = "uart";

// UART 0
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define BAUD_RATE (115200)

// Buffer global y flag
char uart_buffer[RX_BUF_SIZE];
uint8_t uart_data_ready = 0;

int sendData(const char *data)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM, data, len);
    ESP_LOGI(TX_TASK_TAG, "Wrote %d bytes", txBytes);
    return txBytes;
}

void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    while (1)
    {
        // Recibe cada 5 segundos porque la educia manda datos por UART cada 5
        const int rxBytes = uart_read_bytes(UART_NUM, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0)
        {
            data[rxBytes] = '\0';
            // Copiamos los datos al buffer global
            memcpy(uart_buffer, data, rxBytes + 1);
            uart_data_ready = 1;

            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}

void uart_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    xTaskCreate(rx_task, "uart_rx_task", 1024 * 2, NULL, 5, NULL);
}