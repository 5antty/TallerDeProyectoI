
/* UART asynchronous example, that uses separate RX and TX tasks

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "manejo_uart.h"
#include "manejador_mqtt.h"
#include <string.h>
const char *TAG_UART = "uart";

// UART 0
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define BAUD_RATE (9600)

// Buffer global y flag
// char uart_buffer[RX_BUF_SIZE];
// uint8_t uart_data_ready = 0;

// Buffer para mensajes
#define MAX_MSG_SIZE 20
static SemaphoreHandle_t uart_mutex;

int sendData(const char *data)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    const int len = strlen(data);
    xSemaphoreTake(uart_mutex, portMAX_DELAY);
    const int txBytes = uart_write_bytes(UART_NUM, data, len);
    xSemaphoreGive(uart_mutex);
    ESP_LOGI(TX_TASK_TAG, "Se envio %s", data);
    return txBytes;
}
/*
       #T,25.5,60.2*\n          // Temperatura y humedad
       #A,ON*\n       // Acción: ENCENDER ALARMA
       #C,ON*\n       // Acción: ENCENDER CALOVENTOR
       #V,OFF*\n       // Acción: APAGAR VENTILADOR
       #L,1ON*\n             // Acción: ENCENDER LUZ 1
       #L,2OFF*\n            // Acción: APAGAR LUZ 2
       #S,1,89*\n            // Estado de del slider 1 y 2
       */
void txCommand(char *tipo, const char *datos)
{
    char *topic = strstr(tipo, "smarthome/esp32/");
    char mensaje[MAX_MSG_SIZE];
    if (topic)
    {
        // Mueve el resto de la cadena incluyendo el '\0'
        memmove(topic, topic + strlen("smarthome/esp32/"), strlen(topic + strlen("smarthome/esp32/")) + 1);
    }
    switch (topic[0])
    {
    case 'l':
        if (topic[3] == '1')
            snprintf(mensaje, MAX_MSG_SIZE, "#L,1%s*\r", datos);
        else if (topic[3] == '2')
            snprintf(mensaje, MAX_MSG_SIZE, "#L,2%s*\r", datos);
        sendData(mensaje);
        break;

    case 'c':
        snprintf(mensaje, MAX_MSG_SIZE, "#C,%s*\r", datos);
        sendData(mensaje);
        break;

    case 'v':
        snprintf(mensaje, MAX_MSG_SIZE, "#V,%s*\r", datos);
        sendData(mensaje);
        break;
    case 's':
        if (topic[5] == '1')
            snprintf(mensaje, MAX_MSG_SIZE, "#S,1,%s*\r", datos);
        else if (topic[5] == '2')
            snprintf(mensaje, MAX_MSG_SIZE, "#S,2,%s*\r", datos);
        sendData(mensaje);
        break;
    default:
        break;
    }
}
/*

static char *topicsWeb[] = {
    "smarthome/web/luz1",
    "smarthome/web/luz2",
    "smarthome/web/caloventor",
    "smarthome/web/ventilador",
    "smarthome/web/alarma",
    "smarthome/web/slide1",
    "smarthome/web/slide2",
    "smarthome/web/temp",
    "smarthome/web/hum",
};
*/

void procesar_mensaje(char *mensaje)
{

    ESP_LOGI(TAG_UART, "Mensaje recibido: %s", mensaje);

    // Verificar formato básico
    if (mensaje[0] != '#')
    {
        ESP_LOGW(TAG_UART, "Mensaje sin inicio válido");
        return;
    }

    char *fin = strchr(mensaje, '*'); // Buscar fin de mensaje
    if (fin == NULL)
    {
        ESP_LOGW(TAG_UART, "Mensaje sin fin válido");
        return;
    }

    // Extraer tipo de mensaje
    char tipo = mensaje[1];

    // Extraer datos (después de tipo y coma, antes de *)
    char datos[MAX_MSG_SIZE];
    int datos_len = fin - (mensaje + 3);
    if (datos_len > 0 && datos_len < MAX_MSG_SIZE)
    {
        strncpy(datos, mensaje + 3, datos_len);
        datos[datos_len] = '\0';
    }
    else
    {
        datos[0] = '\0';
    }

    // Procesar según tipo
    switch (tipo)
    {
        /*
       #T,25.5,60.2*\n          // Temperatura y humedad
       #A,ON*\n       // Acción: ENCENDER ALARMA
       #C,ON*\n       // Acción: ENCENDER CALOVENTOR
       #V,OFF*\n       // Acción: APAGAR VENTILADOR
       #L,1ON*\n             // Acción: ENCENDER LUZ 1
       #L,2OFF*\n            // Acción: APAGAR LUZ 2
       #S,1,89*\n            // Estado de del slider 1 y 2
       */
    case 'T':
        char *temp = strtok(datos, ",");
        char *hum = strtok(NULL, ",");
        mqtt_publish("smarthome/web/temp", temp);
        mqtt_publish("smarthome/web/hum", hum);
        break;
    case 'L':
        if (datos[0] == '1')
        {
            if (datos[2] == 'N')
                mqtt_publish("smarthome/web/luz1", "ON");
            else if (datos[2] == 'F')
                mqtt_publish("smarthome/web/luz1", "OFF");
        }
        else if (datos[0] == '2')
        {
            if (datos[2] == 'N')
                mqtt_publish("smarthome/web/luz2", "ON");
            else if (datos[2] == 'F')
                mqtt_publish("smarthome/web/luz2", "OFF");
        }
        break;
    case 'S':
        if (datos[0] == '1')
            mqtt_publish("smarthome/web/slide1", datos + 2);
        else if (datos[0] == '2')
            mqtt_publish("smarthome/web/slide2", datos + 2);
        break;
    case 'C':
        if (datos[1] == 'N')
            mqtt_publish("smarthome/web/caloventor", "ON");
        else if (datos[1] == 'F')
            mqtt_publish("smarthome/web/caloventor", "OFF");
        break;
    case 'V':
        if (datos[1] == 'N')
            mqtt_publish("smarthome/web/ventilador", "ON");
        else if (datos[1] == 'F')
            mqtt_publish("smarthome/web/ventilador", "OFF");
        break;
    case 'A':
        mqtt_publish("smarthome/web/alarma", datos);
        break;
    default:
        ESP_LOGW(TAG_UART, "Tipo de mensaje desconocido: %c", tipo);
    }
}

void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    // esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    // char mensaje[MAX_MSG_SIZE];
    // char c;
    // int msg_idx = 0;
    bool recibiendo = false;
    while (1)
    {
        // Recibe cada 5 segundos porque la educia manda datos por UART cada 5
        const int rxBytes = uart_read_bytes(UART_NUM, data, RX_BUF_SIZE - 1, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0)
        {
            // ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            data[rxBytes] = '\0';
            procesar_mensaje((char *)data);
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

    uart_mutex = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(rx_task, "uart_rx_task", 1024 * 2, NULL, 2, NULL, 1);
    ESP_LOGI(TAG_UART, "Sistema iniciado, esperando datos de EDU-CIAA...");
}