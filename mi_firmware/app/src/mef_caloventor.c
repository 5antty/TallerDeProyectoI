#include "mef_caloventor.h"

static caloventor_estado_t estado_caloventor;


static void apagar_todo(void) {
    gpioWrite(CALOVENTOR_CALIENTE_PIN, OFF);
    gpioWrite(CALOVENTOR_FRIO_PIN, OFF);
    // uartWriteString(UART_USB, "CALOVENTOR: Apagado.\r\n"); // Mensajes de debug opcionales
}

static void encender_calor(void) {
    gpioWrite(CALOVENTOR_CALIENTE_PIN, ON);
    gpioWrite(CALOVENTOR_FRIO_PIN, OFF);
    // uartWriteString(UART_USB, "CALOVENTOR: Encendiendo CALOR.\r\n");
}

static void encender_frio(void) {
    gpioWrite(CALOVENTOR_CALIENTE_PIN, OFF);
    gpioWrite(CALOVENTOR_FRIO_PIN, ON);
    // uartWriteString(UART_USB, "CALOVENTOR: Encendiendo FRIO.\r\n");
}

// --- Inicialización ---

void MEF_Caloventor_Init(void) {
    estado_caloventor = CALO_OFF;
    apagar_todo(); 
}

// ==========================================================
// FUNCIONES PÚBLICAS DE CONTROL MANUAL (Llamadas por ESP32/Pantalla)
// ==========================================================

void MEF_Caloventor_SetOff(void) {
    estado_caloventor = CALO_MANUAL_OFF;
    apagar_todo();
    uartWriteString(UART_USB, "CALOVENTOR: Control manual - FORZAR APAGADO.\r\n");
}

void MEF_Caloventor_SetCalor(void) {
    estado_caloventor = CALO_MANUAL_CALOR;
    encender_calor();
    uartWriteString(UART_USB, "CALOVENTOR: Control manual - FORZAR CALOR.\r\n");
}

void MEF_Caloventor_SetFrio(void) {
    estado_caloventor = CALO_MANUAL_FRIO;
    encender_frio();
    uartWriteString(UART_USB, "CALOVENTOR: Control manual - FORZAR FRIO.\r\n");
}

void MEF_Caloventor_SetAuto(void) {
    estado_caloventor = CALO_OFF; 
    apagar_todo();
    uartWriteString(UART_USB, "CALOVENTOR: Volviendo a Control AUTOMÁTICO.\r\n");
}

// ==========================================================
// ACTUALIZACIÓN DE LA MEF
// ==========================================================

void MEF_Caloventor_Update(float temperatura_actual) {
    
    // Si la MEF está en modo manual, no se hace nada, regulo todo con sensores
    if (estado_caloventor == CALO_MANUAL_OFF || 
        estado_caloventor == CALO_MANUAL_CALOR || 
        estado_caloventor == CALO_MANUAL_FRIO) {
        
        // mantengo las salidas en su estado manual actual
        return; 
    }
    
    switch (estado_caloventor) {

        case CALO_OFF:
            // Transición a CALOR
            if (temperatura_actual <= TEMP_UMBRAL_FRIO_C) {
                estado_caloventor = CALO_CALENTANDO;
                encender_calor();
            }
            // Transición a FRÍO
            else if (temperatura_actual >= TEMP_UMBRAL_CALIENTE_C) {
                estado_caloventor = CALO_ENFRIANDO;
                encender_frio();
            }
            break;

        case CALO_CALENTANDO:
            // Transición a APAGADO (con histéresis)
            if (temperatura_actual > (TEMP_UMBRAL_FRIO_C + TEMP_HISTERESIS_C)) {
                estado_caloventor = CALO_OFF;
                apagar_todo();
            }
            break;

        case CALO_ENFRIANDO:
            // Transición a APAGADO (con histéresis)
            if (temperatura_actual < (TEMP_UMBRAL_CALIENTE_C - TEMP_HISTERESIS_C)) {
                estado_caloventor = CALO_OFF;
                apagar_todo();
            }
            break;
            
        // Los estados manuales (CALO_MANUAL_...) son manejados por el "if" inicial y nunca
        // entran en la lógica del switch para que la lógica automática no los anule.
        default:
            break;
    }
}

caloventor_estado_t MEF_Caloventor_GetEstado(void) {
    return estado_caloventor;
}