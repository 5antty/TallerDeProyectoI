/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>

#include "manejo_uart.h"
#include "manejador_wifi.h"
extern int isConnected;
extern const char *TAG_UART;
extern const char *TAG_WIFI;
extern uint8_t uart_data_ready;
// extern char uart_buffer[RX_BUF_SIZE];

#include "manejador_mqtt.h"
extern esp_mqtt_client_handle_t mqttClient;

void comunicaciones(void *pvParameters)
{
    ESP_LOGI(TAG_WIFI, "Iniciando conexion a WIFI...");
    wifi_init_sta();
    if (isConnected)
        mqtt_start();
    vTaskDelete(NULL); // Se borra a sí misma al terminar
}

void app_main(void)
{
    // Inicializacion de memoria no volatil
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    // ESP_ERROR_CHECK(nvs_flash_init());

    xTaskCreatePinnedToCore(comunicaciones, "WIFI", 4096, NULL, 1, NULL, 0);
    ESP_LOGI(TAG_UART, "Inicializando UART...");
    uart_init();
}