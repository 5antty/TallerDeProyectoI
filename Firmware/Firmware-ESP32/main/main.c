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

#include "manejo_uart.h"
#include "manejador_wifi.h"
extern int isConnected;
extern const char *TAG_UART;
extern const char *TAG_WIFI;
extern uint8_t uart_data_ready;
extern char uart_buffer[RX_BUF_SIZE];

#include "manejador_mqtt.h"
extern esp_mqtt_client_handle_t mqttClient;

static char *topicsWeb[] = {
    "smarthome/web/luz1",
    "smarthome/web/luz2",
    "smarthome/web/caloventor",
    "smarthome/web/ventilador",
    "smarthome/web/alarma"};

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
    ESP_LOGI(TAG_UART, "Inicializando UART...");
    uart_init();
    ESP_LOGI(TAG_WIFI, "Iniciando conexion a WIFI...");
    wifi_init_sta();
    if (isConnected)
        mqtt_start();

    while (1)
    {
        if (uart_data_ready)
        {
            // Por ejemplo, enviar por MQTT
            printf("Recibido por UART: %s\n", uart_buffer);

            // Publicar por MQTT si ya está inicializado
            switch (uart_buffer[0])
            {
            case 'A':
                /*Publicacion de mensajes en el broker sobre la alarma*/
                if (strcmp((char *)uart_buffer, "AON") == 0)
                {
                    esp_mqtt_client_publish(mqttClient, topicsWeb[4], "ON", 0, 0, 0);
                }
                else if (strcmp((char *)uart_buffer, "AOFF") == 0)
                {
                    esp_mqtt_client_publish(mqttClient, topicsWeb[4], "OFF", 0, 0, 0);
                }
                break;
            case '1':
                /*Publicacion de mensajes en el broker sobre la luz 1*/
                if (strcmp((char *)uart_buffer, "1ON") == 0)
                {
                    esp_mqtt_client_publish(mqttClient, topicsWeb[0], "ON", 0, 0, 0);
                }
                else if (strcmp((char *)uart_buffer, "1OFF") == 0)
                {
                    esp_mqtt_client_publish(mqttClient, topicsWeb[0], "OFF", 0, 0, 0);
                }
                break;
            case '2':
                /*Publicacion de mensajes en el broker sobre la luz 2*/
                if (strcmp((char *)uart_buffer, "2ON") == 0)
                {
                    esp_mqtt_client_publish(mqttClient, topicsWeb[1], "ON", 0, 0, 0);
                }
                else if (strcmp((char *)uart_buffer, "2OFF") == 0)
                {
                    esp_mqtt_client_publish(mqttClient, topicsWeb[1], "OFF", 0, 0, 0);
                }
                break;
            case 'C':
                /*Publicacion de mensajes en el broker sobre la luz 1*/
                if (strcmp((char *)uart_buffer, "CON") == 0)
                {
                    esp_mqtt_client_publish(mqttClient, topicsWeb[2], "ON", 0, 0, 0);
                }
                else if (strcmp((char *)uart_buffer, "COFF") == 0)
                {
                    esp_mqtt_client_publish(mqttClient, topicsWeb[2], "OFF", 0, 0, 0);
                }
                break;
            case 'V':
                /*Publicacion de mensajes en el broker sobre la luz 1*/
                if (strcmp((char *)uart_buffer, "VON") == 0)
                {
                    esp_mqtt_client_publish(mqttClient, topicsWeb[3], "ON", 0, 0, 0);
                }
                else if (strcmp((char *)uart_buffer, "VOFF") == 0)
                {
                    esp_mqtt_client_publish(mqttClient, topicsWeb[3], "OFF", 0, 0, 0);
                }
                break;
            default:
                ESP_LOGW(TAG_UART, "Comando no reconocido: %s", uart_buffer);
                break;
            }

            uart_data_ready = 0; // Limpiamos el flag
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}