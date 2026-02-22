#ifndef MEF_ALARMA_H_
#define MEF_ALARMA_H_

#include "sapi.h"
#include <stdbool.h>
#include <stdint.h>

#define PIN_PIR GPIO6
#define PIN_MAGNETICO T_FIL2
#define PIN_SIRENA T_COL1
#define PIN_BUZZER GPIO5

#define ARM_DELAY_GRACIA_MS 5000u   // 30 seg
#define INGRESANDO_PIN_DELAY 60000u // 1 min
#define PIN_MAX_LONGITUD 6          // PIN de hasta 6 d�gitos
#define MAX_INTENTOS_CLAVE 3

typedef enum
{
    ALM_DESARMADO = 0,
    ALM_ARMANDO,
    ALM_ARMADO,
    ALM_IDENTIFICACION,
    ALM_ACTIVADO
} alarma_estado_t;

typedef enum
{
    CODE_ACTION_NONE = 0,
    CODE_ACTION_ARM,
    CODE_ACTION_DISARM
} code_action_t;

typedef enum
{
    SENSOR_MAGNETICO = (1 << 0),                        // 0001
    SENSOR_MOVIMIENTO = (1 << 1),                       // 0010
    SENSOR_AMBOS = SENSOR_MAGNETICO | SENSOR_MOVIMIENTO // 0011
} alarma_sensores_t;

// ej 0011 & 0001 = 0001 V (ambos sensores coin magnetico
// ej 0010 & 0001 = 0000 F(magnetico con movimiento)

void MEF_Alarm_Init(void);
void MEF_Alarm_Update(void);
void MEF_Alarm_Pedido_Armar(void);
void MEF_Alarm_Pedido_Desarmar(void);
void MEF_Alarm_SetPIN(const char *pin);
void MEF_Alarm_SetSensores(uint8_t sensores);
void MEF_Alarm_ProcessPIN(const char *pin_ingresado);

alarma_estado_t get_alarm_state(void);
bool_t get_pir_active_status(void);
bool_t get_magnetico_active_status(void);
bool MEF_Alarm_VerificarPin(const char *pin_ingresado);
uint8_t MEF_Alarm_GetSensoresConfigurados(void);

#endif