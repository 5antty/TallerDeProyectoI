#include "mef_caloventor.h"
#include "manejo_uart.h"

/*
typedef enum {
    CALO_OFF = 0,           // Apagado total
    CALO_AUTO_CALENTANDO,   // Autom�tico: Prendido por fr�o
    CALO_AUTO_ENFRIANDO,    // Autom�tico: Prendido por calor
    CALO_MANUAL_CALOR,      // Forzado Manualmente (Bot�n UI)
    CALO_MANUAL_VENTILADOR  // Forzado Manualmente (Bot�n UI)
} caloventor_estado_t;
*/

static caloventor_estado_t estado_actual = CALO_OFF;
static bool modo_automatico = true; // Empezamos en autom�tico por defecto

// --- FUNCIONES PRIVADAS ---
static void apagar_todo(void) {
    gpioWrite(PIN_CALOVENTOR, OFF);
    gpioWrite(PIN_VENTILADOR, OFF);
}

static void activar_caloventor(void) {
    gpioWrite(PIN_VENTILADOR, OFF); 
    gpioWrite(PIN_CALOVENTOR, ON);
}

static void activar_ventilador(void) {
    gpioWrite(PIN_CALOVENTOR, OFF); 
    gpioWrite(PIN_VENTILADOR, ON);
}

// --- INICIALIZACI�N ---
void MEF_Caloventor_Init(void) {
    gpioInit(PIN_CALOVENTOR, GPIO_OUTPUT);
    gpioInit(PIN_VENTILADOR, GPIO_OUTPUT);
    apagar_todo();
    modo_automatico = true;
    estado_actual = CALO_OFF;
}

// --- SETTERS (CONTROL MANUAL DESDE PANTALLA) ---
void MEF_Caloventor_SetCalor(void) {
    modo_automatico = false; 
    estado_actual = CALO_MANUAL_CALOR;
    activar_caloventor();
    txCmd('C', "ON");
}

void MEF_Caloventor_SetVentilador(void) {
    modo_automatico = false;
    estado_actual = CALO_MANUAL_VENTILADOR;
    activar_ventilador();
   txCmd('V', "ON");
}

void MEF_Caloventor_SetOff(void) {
    modo_automatico = false; // Queda en manual pero apagado
    estado_actual = CALO_OFF;
    apagar_todo();
    txCmd('C', "OFF");
    txCmd('V', "OFF");
}

void MEF_Caloventor_SetAuto(void) {
    modo_automatico = true;
}

// --- GETTERS ---
caloventor_estado_t MEF_Caloventor_GetEstado(void) {
    return estado_actual;
}

bool MEF_Caloventor_IsAuto(void) {
    return modo_automatico;
}

int MEF_Caloventor_GetUmbralFrio(void) {
    return (int)TEMP_UMBRAL_FRIO; // Casteamos a int para mostrar en pantalla
}

// Retorna el valor para ENCENDER EL CALOVENTOR (Ej: 20 grados)
int MEF_Caloventor_GetUmbralCaliente(void) {
    return (int)TEMP_UMBRAL_CALOR; // Casteamos a int para mostrar en pantalla
}

// --- BUCLE PRINCIPAL ---
void MEF_Caloventor_Update(float temp_actual) {
    
    // Si estamos en manual, erl usuario decide que se hace
    if (!modo_automatico) return;

    // --- L�GICA AUTOM�TICA ---
    switch (estado_actual) {
        case CALO_OFF:
        case CALO_AUTO_CALENTANDO: // Evaluamos si seguimos calentando o paramos
            if (temp_actual < TEMP_UMBRAL_CALOR) {
                if (estado_actual != CALO_AUTO_CALENTANDO) {
                    activar_caloventor();
                    estado_actual = CALO_AUTO_CALENTANDO;
                }
            } 
            else if (temp_actual > TEMP_UMBRAL_FRIO) {
                // Hace calor (> 26), prender ventilador
                if (estado_actual != CALO_AUTO_ENFRIANDO) {
                    activar_ventilador();
                    estado_actual = CALO_AUTO_ENFRIANDO;
                }
            }
            else {
                // Zona muerta (ej: 23 grados), apagar todo
                if (estado_actual != CALO_OFF) {
                    apagar_todo();
                    estado_actual = CALO_OFF;
                }
            }
            break;

        case CALO_AUTO_ENFRIANDO:
            // Si baja de 26 apagamos
            if (temp_actual < (TEMP_UMBRAL_FRIO - TEMP_HISTERESIS)) {
                apagar_todo();
                estado_actual = CALO_OFF;
            }
            break;
            
        default:
            // Si veniamos de manual y activaron auto, reseteamos a OFF para evaluar
            estado_actual = CALO_OFF;
            apagar_todo();
            break;
    }
}