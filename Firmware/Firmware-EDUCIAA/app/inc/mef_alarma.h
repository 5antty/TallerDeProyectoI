#ifndef MEF_ALARMA_H_
#define MEF_ALARMA_H_

#include "sapi.h"
#include <stdbool.h>
#include <stdint.h>

#define PIN_PIR             T_FIL0 // (GPIO44) 
#define PIN_MAGNETICO_1     TEC1
#define PIN_MAGNETICO_2     TEC2
#define PIN_SIRENA          GPIO0

#define ARM_DELAY_GRACIA_MS 30000u // 30 seg
#define INGRESANDO_PIN_DELAY 60000u // 1 min
#define PIN_MAX_LONGITUD 6 // PIN de hasta 6 dígitos
#define MAX_INTENTOS_CLAVE 3

typedef enum {
ALM_DESARMADO = 0,
ALM_ARMANDO,
ALM_ARMADO,
ALM_IDENTIFICACION,
ALM_ACTIVADO
} alarma_estado_t;

typedef enum {
CODE_ACTION_NONE = 0,
CODE_ACTION_ARM,
CODE_ACTION_DISARM
} code_action_t;

typedef enum {
SENSOR_MAGNETICO = (1 << 0),
SENSOR_MOVIMIENTO = (1 << 1),
SENSOR_AMBOS = SENSOR_MAGNETICO | SENSOR_MOVIMIENTO
} alarma_sensores_t;

void MEF_Alarm_Init(void);
void MEF_Alarm_Update(void);
void MEF_Alarm_Pedido_Armar(void);
void MEF_Alarm_Pedido_Desarmar(void);
void MEF_Alarm_SetPIN(const char *pin);
void MEF_Alarm_SetSensores(uint8_t sensores);

#endif 