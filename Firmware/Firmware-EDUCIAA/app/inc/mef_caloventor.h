#ifndef MEF_CALOVENTOR_H
#define MEF_CALOVENTOR_H

#include "sapi.h"

// --- Definiciones de hw
#define CALOVENTOR_CALIENTE_PIN LEDR
#define CALOVENTOR_FRIO_PIN     LED3

// --- Umbrales de Temperatura ---
#define TEMP_UMBRAL_FRIO_C      22  // Si la temperatura es <= 22 se activa el calor.
#define TEMP_UMBRAL_CALIENTE_C  26  // Si la temperatura es >= 26 se activa el fr�o.
#define TEMP_HISTERESIS_C       1   // Histeresis por si el sensor tiene error

typedef enum {
    CALO_OFF,           // 0. Todo apagado (temperatura en rango normal)
    CALO_CALENTANDO,    // 1. Caloventor caliente encendido (Control Autom�tico)
    CALO_ENFRIANDO,     // 2. Caloventor fr�o
    CALO_MANUAL_OFF,    // 3. forzar apagado
    CALO_MANUAL_CALOR,  // 4. forzar calor
    CALO_MANUAL_FRIO    // 5. forzar fr�o
} caloventor_estado_t;


void MEF_Caloventor_Init(void);


void MEF_Caloventor_Update(float temperatura_actual);

caloventor_estado_t MEF_Caloventor_GetEstado(void);


void MEF_Caloventor_SetOff(void);

void MEF_Caloventor_SetCalor(void);


void MEF_Caloventor_SetFrio(void);

void MEF_Caloventor_SetAuto(void);

#endif 