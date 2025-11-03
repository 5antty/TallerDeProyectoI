#ifndef MANEJADOR_MQTT_H
#define MANEJADOR_MQTT_H

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

void mqtt_publish(char *topic, char *message);
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void mqtt_start(void);

#endif