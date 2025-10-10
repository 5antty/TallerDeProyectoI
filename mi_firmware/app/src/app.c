#include "sapi.h"
#include "mef_alarma.h"
#include "mef_luces.h"
#include "mef_caloventor.h"    

//#include "mef_ui.h"         


#define GPIO_DHT11 T_COL2 

static float ultima_temperatura_valida = 25.0; 
static float ultima_humedad_valida = 50.0; 
static uint32_t t_dht = 0;

// --- Bucle principal ---

int main(void) {
    boardConfig();
    uartConfig(UART_USB, 115200);
    tickConfig(1); // Tick de 1 ms
     uartWriteString(UART_USB, "Iniciando sistema...\r\n"); // Mensaje de prueba
   gpioWrite(LEDB, ON);
   
    dht11Init(GPIO_DHT11); 

    MEF_Alarm_Init();
    MEF_Luz_Init();
   MEF_Caloventor_Init(); 
   // MEF_UI_INIT(); 

    uint32_t t_alarma = tickRead();
    uint32_t t_luces = tickRead();
    uint32_t t_ui_caloventor = tickRead(); 
    t_dht = tickRead(); 

    while(true) {
        uint32_t ahora = tickRead();

        // ----------------------------------------------------
        // TAREA 1: MEF Alarma (cada 10 ms). El m�s seguido para detectar cambios
        // ----------------------------------------------------
        if((int32_t)(ahora - t_alarma) >= 10) {
            t_alarma = ahora;
            MEF_Alarm_Update();
        }
        
        // ----------------------------------------------------
        // TAREA 2: MEF Luces (cada 20 ms)
        // ----------------------------------------------------
        if((int32_t)(ahora - t_luces) >= 20) {
            t_luces = ahora;
            MEF_Luz_Update();
        }
        
        // ----------------------------------------------------
         // TAREA 3: Lectura Lenta del DHT11 (cada 5000 ms)-> Recomendado por el sapi(mas lenta que las otras tareas)
         // ----------------------------------------------------
         if((int32_t)(ahora - t_dht) >= 5000) { 
             t_dht = ahora;
             
             float temp_leida = 0.0;
             float hum_leida = 0.0;
             
             // UsO la funci�n dht11Read(puntero_humedad, puntero_temperatura) de SAPI  EJEMPLO COMPIADO DEL SAPI, REVISAR AHI
             if (dht11Read(&hum_leida, &temp_leida) == TRUE) { 
                 ultima_temperatura_valida = temp_leida;
                 ultima_humedad_valida = hum_leida; 
                 
                 // MSJ de temperatura
                 char buffer[64];
                 snprintf(buffer, sizeof(buffer), "DHT11 OK. Temp: %.1f C, Hum: %.1f%%\r\n", 
                          ultima_temperatura_valida, ultima_humedad_valida);
                 uartWriteString(UART_USB, buffer);
             } else {
                 uartWriteString(UART_USB, "DHT11: Error de lectura\r\n");
             }
         }

         // ----------------------------------------------------
         // TAREA 4: MEF UI y MEF Caloventor (cada 100 ms) -> SON SIMPLEMENTE LECTURAS DE PANTALLA O ESP23, PARA  LLAMAR A FUNCIONES PUBLICAS
         // ----------------------------------------------------
         if((int32_t)(ahora - t_ui_caloventor) >= 100) {
             t_ui_caloventor = ahora;
             
             
             MEF_Caloventor_Update(ultima_temperatura_valida);
             
              //UI_Update(); 
         }                          

      
    }
}