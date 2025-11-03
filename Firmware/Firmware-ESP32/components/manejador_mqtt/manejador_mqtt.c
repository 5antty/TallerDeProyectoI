#include "manejador_mqtt.h"

esp_mqtt_client_handle_t mqttClient;
#define MQTT_BROKER_URI "mqtt://broker.emqx.io"
#define MQTT_PORT 1883
static const char *TAG_MQTT = "mqtt";

static char *topics[] = {
    "smarthome/esp32/luz1",
    "smarthome/esp32/luz2",
    "smarthome/esp32/caloventor",
    "smarthome/esp32/ventilador",
    "smarthome/esp32/alarma"};

void mqtt_publish(char *topic, char *message)
{
    int msg_id = esp_mqtt_client_publish(mqttClient, topic, message, 0, 0, 0);
    if (msg_id == 0)
        ESP_LOGI(TAG_MQTT, "Sent Data: %s", message);
    else
        ESP_LOGI(TAG_MQTT, "Error msg_id:%d while sending data", msg_id);
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
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

void mqtt_start(void)
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