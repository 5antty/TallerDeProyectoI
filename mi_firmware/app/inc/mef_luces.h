#ifndef MEF_LUCES_H
#define MEF_LUCES_H

#include "sapi.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    LUZ_OFF = 0,
    LUZ_ON,
    LUZ_DIMMING,
    LUZ_TIMER,
    SENSOR_LUZ
} luz_estado_t;

/* ==== API pública ==== */
void MEF_Luz_Init(void);
void MEF_Luz_SetMode(uint8_t id, luz_estado_t modo, uint8_t brillo);
void MEF_Luz_SetTimer(uint8_t id, uint32_t duracion_ms);
void MEF_Luz_Update(void);

#endif