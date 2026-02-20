#include "mef_luces.h"
#include "sapi.h"
#include "chip.h"
#include <stdio.h> // Necesario para sprintf
#include "manejo_uart.h"

//aaaaaaaaaaaaaaaaaaaaaaaaaa

// --- HARDWARE ---
static const gpioMap_t FocoTriacPin[MAX_FOCOS] = { PIN_TRIAC_1, PIN_TRIAC_2 };

// --- CONFIGURACI�N ZCD (P7_5 -> GPIO3[13]) ---
#define ZCD_SCU_PORT  7
#define ZCD_SCU_PIN   5
#define ZCD_GPIO_PORT 3
#define ZCD_GPIO_PIN  13

// CONSTANTES
#define SEMICICLO_TICKS 100  
#define ANCHO_PULSO     2    

// --- VARIABLES ---
volatile uint32_t timer_tick_counter = 0;
volatile bool zcd_flag = false;
static foco_t focos[MAX_FOCOS];

static bool estado_zcd_anterior = 0;

// --- PROTOTIPOS ---
static uint16_t calcular_ticks(uint8_t brillo);
static bool TimerService_Elapsed(uint32_t start, uint32_t delay_ms);
extern bool_t get_current_luminosity(void); 

// ================================================================
// 1. INTERRUPCI�N DEL TIMER (RIT)
// ================================================================
void RIT_IRQHandler(void) {
    Chip_RIT_ClearInt(LPC_RITIMER);

  
    bool estado_zcd_actual = Chip_GPIO_GetPinState(LPC_GPIO_PORT, ZCD_GPIO_PORT, ZCD_GPIO_PIN);

    if (estado_zcd_actual == 1 && estado_zcd_anterior == 0) {
        timer_tick_counter = 0;
        zcd_flag = true;
        
        gpioWrite(FocoTriacPin[0], OFF);
        gpioWrite(FocoTriacPin[1], OFF);
    }
    estado_zcd_anterior = estado_zcd_actual;
    // -------------------------------

    if (!zcd_flag) return;

    timer_tick_counter++;

    for (int i = 0; i < MAX_FOCOS; i++) {
        uint32_t disparo = focos[i].delay_disparo; 
        
        if (timer_tick_counter == disparo) {
            gpioWrite(FocoTriacPin[i], ON);
        }
        else if (timer_tick_counter == (disparo + ANCHO_PULSO)) {
            gpioWrite(FocoTriacPin[i], OFF);
        }
    }
    
    if (timer_tick_counter > (SEMICICLO_TICKS + 20)) zcd_flag = false; 
}

// ================================================================
// 2. INICIALIZACI�N
// ================================================================
void MEF_Luz_Init(void) {
   
    for(int i=0; i<MAX_FOCOS; i++) {
        focos[i].estado = LUZ_OFF; 
        focos[i].brillo = 0; 
        focos[i].delay_disparo = SEMICICLO_TICKS + 50; 
        
        gpioInit(FocoTriacPin[i], GPIO_OUTPUT);
        gpioWrite(FocoTriacPin[i], OFF);
    }

    Chip_SCU_PinMuxSet(ZCD_SCU_PORT, ZCD_SCU_PIN, SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP | SCU_MODE_FUNC0);
    Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, ZCD_GPIO_PORT, ZCD_GPIO_PIN);

    Chip_RIT_Init(LPC_RITIMER);
    uint32_t ticks = SystemCoreClock / 10000; 
    Chip_RIT_SetCOMPVAL(LPC_RITIMER, ticks); 
    Chip_RIT_EnableCTRL(LPC_RITIMER, RIT_CTRL_ENCLR | RIT_CTRL_TEN);
    Chip_RIT_ClearInt(LPC_RITIMER);
    
    NVIC_SetPriority(RITIMER_IRQn, 2); 
    NVIC_EnableIRQ(RITIMER_IRQn);
}

// ================================================================
// 3. L�GICA DE CONTROL
// ================================================================

static uint16_t calcular_ticks(uint8_t brillo) {
    if (brillo >= 95) return 5;           // M�nimo retardo seguro (500us)
    if (brillo <= 5)  return SEMICICLO_TICKS + 50; 
    return 100 - brillo;                 
}

static bool TimerService_Elapsed(uint32_t start, uint32_t delay_ms) {
   return ((int32_t)(tickRead() - start) >= (int32_t)delay_ms);
}

void MEF_Luz_Update(void) {
    rtc_t rtc_time;
    static uint32_t div_debug = 0;

    for(int i=0; i<MAX_FOCOS; i++) {
        foco_t *f = &focos[i];
        switch(f->estado){
            case LUZ_OFF: 
                f->delay_disparo = SEMICICLO_TICKS + 50; 
                break;

            case LUZ_ON: 
                f->delay_disparo = 15; // 1.5ms para asegurar disparo
                
                div_debug++;
                if (div_debug > 20) { // Ajusta esto si sale muy r�pido
                     uartWriteString(UART_USB, "DEBUG: Estado activo -> LUZ_ON\r\n");
                     div_debug = 0;
                }
                break;

            case LUZ_DIMMING: 
                f->delay_disparo = calcular_ticks(f->brillo); 
                break;

            case LUZ_TIMER_DELAY:
                if (TimerService_Elapsed(f->t_start, f->t_delay)) {
                    f->estado = LUZ_OFF; 
                    f->delay_disparo = SEMICICLO_TICKS + 50;
                    f->brillo = 0; 
                } else {
                    f->delay_disparo = 15; 
                    f->brillo = 85;
                    // uartWriteString(UART_USB, "CMD: Timer iniciado. Luz aaa.\r\n");
                }
                break;

            case SENSOR_LUZ:
                if (get_current_luminosity() == TRUE) f->delay_disparo = SEMICICLO_TICKS + 50;
                else f->delay_disparo = 15; 
                break;

            case LUZ_RTC_SCHEDULE:
                 if (rtcRead(&rtc_time)) {
                    uint16_t ahora = rtc_time.hour * 60 + rtc_time.min;
                    uint16_t on = f->rtc_on_h * 60 + f->rtc_on_m;
                    uint16_t off = f->rtc_off_h * 60 + f->rtc_off_m;
                    
                    bool encender = false;
                    if (on < off) encender = (ahora >= on && ahora < off);
                    else encender = (ahora >= on || ahora < off);

                    f->delay_disparo = encender ? 15 : (SEMICICLO_TICKS + 50);
                }
                break;
        }
    }
}

// --- API P�BLICA ---

luz_estado_t MEF_Luz_GetEstado(uint8_t id) {
    if (id >= MAX_FOCOS) return LUZ_OFF;
    return focos[id].estado;
}

void MEF_Luz_SetMode(uint8_t id, luz_estado_t modo, uint8_t brillo) {
    if (id >= MAX_FOCOS) return;
    static char msg[10];

    if (modo == LUZ_ON) {
        uartWriteString(UART_USB, "CMD: Recibida orden LUZ_ON\r\n");
        focos[id].estado = LUZ_ON;
        focos[id].brillo = 85; // Solo para guardar el dato
         
       
        snprintf(msg, MAX_MSG_SIZE, "%d%s", id+1, "ON");
        //sprintf(buffer, "%d%s", id, "ON");
        txCmd('L', msg);
    }
    else if (modo == LUZ_OFF) {
        uartWriteString(UART_USB, "CMD: Recibida orden LUZ_OFF\r\n");
        focos[id].estado = LUZ_OFF;
        focos[id].brillo = 0;
       
        //sprintf(buffer, "%d%s", id, "OFF");
        snprintf(msg, MAX_MSG_SIZE, "%d%s", id+1, "OFF");
        txCmd('L', msg);
    }
    else {
        // Para Dimming, Timer, etc.
        focos[id].estado = modo;
        if (modo == LUZ_DIMMING) {
            focos[id].brillo = brillo;
           
            //sprintf(buffer, "%d,%d", id, brillo);
           
            snprintf(msg, MAX_MSG_SIZE, "%d,%d", id+1, brillo);
            txCmd('S', msg);
        }
    }
}

void MEF_Luz_SetTimerDelay(uint8_t id, uint32_t duracion_ms) {
    if (id >= MAX_FOCOS) return;

    // 1. Configuramos el estado
    focos[id].estado = LUZ_TIMER_DELAY;
    
    // 2. Iniciamos el cron�metro
    focos[id].t_start = tickRead();
    focos[id].t_delay = duracion_ms;

    // 3. --- FIX CR�TICO: ENCENDIDO INMEDIATO ---

    focos[id].brillo = 85;        // Para que la UI sepa que est� al 85%
    focos[id].delay_disparo = 15; // Mismo valor que pusiste en el Update (1.5ms)
}

void MEF_Luz_SetRTC_OnTime(uint8_t id, uint8_t hora, uint8_t min) {
    if (id >= MAX_FOCOS) return;
    focos[id].rtc_on_h = hora; focos[id].rtc_on_m = min;
}

void MEF_Luz_SetRTC_OffTime(uint8_t id, uint8_t hora, uint8_t min) {
    if (id >= MAX_FOCOS) return;
    focos[id].rtc_off_h = hora; focos[id].rtc_off_m = min;
}