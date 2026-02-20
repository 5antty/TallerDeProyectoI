#ifndef MEF_CALOVENTOR_H
#define MEF_CALOVENTOR_H

#include "sapi.h"
#include <stdint.h>
#include <stdbool.h>

// --- PINES DE HARDWARE (P1 Header) ---
// Caloventor -> Pin 33 (T_FIL0)
#define PIN_CALOVENTOR      T_FIL0 
// Ventilador -> Pin 35 (T_FIL3)
#define PIN_VENTILADOR      T_FIL3

// --- CONFIGURACIÓN DE TEMPERATURA ---
#define TEMP_UMBRAL_CALOR   20.0f 
#define TEMP_UMBRAL_FRIO    26.0f 
#define TEMP_HISTERESIS     1.0f


typedef enum {
    CALO_OFF = 0,           // Apagado total
    CALO_AUTO_CALENTANDO,   // Automático: Prendido por frío
    CALO_AUTO_ENFRIANDO,    // Automático: Prendido por calor
    CALO_MANUAL_CALOR,      // Forzado Manualmente (Botón UI)
    CALO_MANUAL_VENTILADOR  // Forzado Manualmente (Botón UI)
} caloventor_estado_t;

void MEF_Caloventor_Init(void);
void MEF_Caloventor_Update(float temp_actual);

// Comandos Manuales
void MEF_Caloventor_SetCalor(void);      // Prende Caloventor, apaga Ventilador
void MEF_Caloventor_SetVentilador(void); // Prende Ventilador, apaga Caloventor
void MEF_Caloventor_SetOff(void);        // Apaga todo (Modo Manual OFF)
void MEF_Caloventor_SetAuto(void);       // Vuelve al modo automático



// Getters
caloventor_estado_t MEF_Caloventor_GetEstado(void);
bool MEF_Caloventor_IsAuto(void); // Retorna TRUE si está en modo automático
int MEF_Caloventor_GetUmbralFrio(void);
int MEF_Caloventor_GetUmbralCaliente(void);

#endif