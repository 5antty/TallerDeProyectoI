#ifndef MEF_LUCES_H
#define MEF_LUCES_H

#include "sapi.h"
#include "sapi_rtc.h" 
#include <stdint.h>
#include <stdbool.h>

// --- PINES DE HARDWARE ---
// Interrupcion Cruce por Cero (P1 - Pin 26)
#define PIN_ZCS_INPUT   T_COL2 

// Salidas a los Opto-Triacs (P2)

#define PIN_TRIAC_1     LCDEN // Pin 25
#define PIN_TRIAC_2     GPIO2 // Pin 31

#define MAX_FOCOS 2

typedef enum {
   LUZ_OFF = 0,
   LUZ_ON,
   LUZ_DIMMING,
   LUZ_TIMER_DELAY, 
   LUZ_RTC_SCHEDULE,
   SENSOR_LUZ 
} luz_estado_t;

/* ==== Estructura de Datos ==== */
typedef struct {
    luz_estado_t estado;
    uint8_t brillo;         // 0 a 100%
    uint16_t delay_disparo; // Tiempo en us para disparar el triac (0 a 10000)
    uint32_t t_start;
    uint32_t t_delay;
    
    uint8_t rtc_on_h;
    uint8_t rtc_on_m;
    uint8_t rtc_off_h;
    uint8_t rtc_off_m;
} foco_t;

/* ==== API pública ==== */
void MEF_Luz_Init(void);
void MEF_Luz_SetMode(uint8_t id, luz_estado_t modo, uint8_t brillo);
void MEF_Luz_SetTimerDelay(uint8_t id, uint32_t duracion_ms); 
void MEF_Luz_SetRTC_OnTime(uint8_t id, uint8_t hora, uint8_t min);
void MEF_Luz_SetRTC_OffTime(uint8_t id, uint8_t hora, uint8_t min);
void MEF_Luz_Update(void);

// Función que debe llamarse desde la Interrupción de GPIO (ZCS)
void MEF_Luz_ZCS_Callback(void *noUsed);

// Función simulada de Timer (Debería llamarse cada 50us o 100us mediante un Timer Hardware)
void MEF_Luz_Timer_Callback(void *noUsed);

#endif