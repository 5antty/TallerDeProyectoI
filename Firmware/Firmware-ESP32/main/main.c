/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "manejo_uart.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "mqtt_client.h"
esp_mqtt_client_handle_t mqttClient;
#define MQTT_BROKER_URI "mqtt://broker.emqx.io"
#define MQTT_PORT 1883

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define WIFI_SSID "Black Hole"
#define WIFI_PASS "Mamix51423"
#define EXAMPLE_ESP_MAXIMUM_RETRY 5

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG_WIFI = "wifi station";
static const char *TAG_MQTT = "mqtt";

static int s_retry_num = 0;
static int isConnected = 0;

extern uint8_t uart_buffer[RX_BUF_SIZE];
extern uint8_t uart_data_ready;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // Evento que inicia la conexion al AP
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        ESP_LOGI(TAG_WIFI, "Conectando WiFi...");
    }
    // Evento que reintenta la conexion al AP si falla
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG_WIFI, "Reintentando conectar al AP");
        }
        else
        {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG_WIFI, "Fallo al conectar al AP");
        isConnected = 0;
    }
    // Evento que ocurre si se conecta correctamente al AP
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_WIFI, "Se obtuvo la IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        isConnected = 1;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    // Inicializacion del grupo de eventos
    wifi_event_group = xEventGroupCreate();

    // Inicializacion de la interfaz de red tcp/ip
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Registro de los manejadores de eventos
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        }};
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_WIFI, "Inicializacion del WiFi STA terminado.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    ESP_LOGI(TAG_WIFI, "Conexion Wi-Fi establecida. Lanzando el cliente MQTT.");

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG_WIFI, "Conectado al AP SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASS);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG_WIFI, "No se pudo conectar al AP SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASS);
    }
    else
    {
        ESP_LOGE(TAG_WIFI, "Evento INESPERADO");
    }
}

// ESTO NO DEBERIA ESTAR, SERIA MEJOR SOLO ENVIAR CUANDO RECIBO ALGO POR UART
void mqtt_publish_task(void *pvParameters)
{
    char datatoSend[20];
    while (1)
    {
        int val = esp_random() % 100;
        //  Valor obtenido por serie
        // REVER ESTO
        sprintf(datatoSend, "%d", val);
        // OSEA HACER ESTO EN EL RECEIVE DE UART
        int msg_id = esp_mqtt_client_publish(mqttClient, "smarthome/web/temperatura", datatoSend, 0, 0, 0);
        if (msg_id == 0)
            ESP_LOGI(TAG_MQTT, "Sent Data: %d", val);
        else
            ESP_LOGI(TAG_MQTT, "Error msg_id:%d while sending data", msg_id);
        vTaskDelay(6000 / portTICK_PERIOD_MS);
        // vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static char *topics[] = {
    "smarthome/esp32/luz1",
    "smarthome/esp32/luz2",
    "smarthome/esp32/caloventor",
    "smarthome/esp32/ventilador",
    "smarthome/esp32/alarma"};

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG_MQTT, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_MQTT, "Se estableció conexión con el broker MQTT => %s", MQTT_BROKER_URI);
        // xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 2048, NULL, 5, NULL);
        for (int i = 0; i < sizeof(topics) / sizeof(topics[0]); i++)
        {
            esp_mqtt_client_subscribe(client, topics[i], 0);
        }
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_MQTT, "Se DESCONECTÓ del broker MQTT => %s", MQTT_BROKER_URI);
        break;

    case MQTT_EVENT_SUBSCRIBED:
        // ESP_LOGI(TAG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d, return code=0x%02x ", event->msg_id, (uint8_t)*event->data);
        ESP_LOGI(TAG_MQTT, "Se SUSCRIBIÓ correctamente al topic => %.*s", event->topic_len, event->topic);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "Se DESUSCRIBIÓ correctamente del topic => %.*s", event->topic_len, event->topic);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG_MQTT, "Se PUBLICÓ correctamente en el topic => %.*s, el mensaje => %.*s", event->topic_len, event->topic, event->data_len, event->data);
        break;

    case MQTT_EVENT_DATA: // Me devuelve el mensaje que mando el broker
        ESP_LOGI(TAG_MQTT, "Se RECIBIÓ un mensaje del broker MQTT: %.*s, del topic => %.*s", event->data_len, event->data, event->topic_len, event->topic);
        // sendData(event->data);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG_MQTT, "ERROR DE EVENTO MQTT");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            ESP_LOGI(TAG_MQTT, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG_MQTT, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG_MQTT, "Last captured errno : %d (%s)", event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        }
        else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
        {
            ESP_LOGI(TAG_MQTT, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        }
        else
        {
            ESP_LOGW(TAG_MQTT, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        break;

    default:
        ESP_LOGI(TAG_MQTT, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = MQTT_BROKER_URI, // cambiar por la ip del broker
            .address.port = MQTT_PORT,
        }};

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    mqttClient = client;
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
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

    ESP_LOGI(TAG_WIFI, "Iniciando conexion a WIFI...");
    wifi_init_sta();
    if (isConnected)
        mqtt_start();

    /* uart_init();
    while (1)
    {
        if (uart_data_ready)
        {
            // Por ejemplo, enviar por MQTT
            printf("Recibido por UART: %s\n", uart_buffer);

            // Publicar por MQTT si ya está inicializado
            // esp_mqtt_client_publish(mqttClient, "esp32/entrada_uart", (const char *)uart_buffer, 0, 0, 0);

            uart_data_ready = 0; // Limpiamos el flag
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    } */
}