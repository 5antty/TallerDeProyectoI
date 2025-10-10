#include "sapi.h"
#include "mef_luces.h"

#define MAX_FOCOS 2

// CAMBIO: uso directamente los pines de la EDU-CIAA
static const uint8_t FocoGPIO[MAX_FOCOS] = { LED1, LED2 };  
static const uint8_t FocoPWM[MAX_FOCOS]  = { 0, 1 }; // aún no mapeados a PWM real

#define LUX_SENSOR CH1  

typedef struct {
    luz_estado_t estado;
    uint8_t brillo;          
    bool habilitado_dimming; 
    uint32_t t_start;   
    uint32_t t_delay;   
} foco_t;

static foco_t focos[MAX_FOCOS];

/* ==== Helper ==== */
static bool TimerService_Elapsed(uint32_t start, uint32_t delay) {
   return ((int32_t)(tickRead() - start) >= (int32_t)delay);
}

/* ==== API ==== */
void MEF_Luz_Init(void) {
    pwmConfig(0, PWM_ENABLE);  
    pwmConfig(PWM0, PWM_ENABLE_OUTPUT);
    pwmConfig(PWM1, PWM_ENABLE_OUTPUT);

    adcConfig(ADC_ENABLE);     

    for(int i=0; i<MAX_FOCOS; i++) {
        focos[i].estado = LUZ_OFF;
        focos[i].brillo = 0;
        focos[i].habilitado_dimming = true;
        focos[i].t_start = 0;
        focos[i].t_delay = 0;

        gpioWrite(FocoGPIO[i], OFF);
        pwmWrite(FocoPWM[i], 0); // duty = 0
    }
}

void MEF_Luz_SetMode(uint8_t id, luz_estado_t modo, uint8_t brillo) {
    if (id >= MAX_FOCOS) return;

    focos[id].estado = modo;
    
    if (modo == LUZ_DIMMING) {
        focos[id].brillo = brillo;
    }
    else if (modo == LUZ_ON) {
        focos[id].brillo = 100;
    }
    else if (modo == LUZ_OFF) {
        focos[id].brillo = 0;
    }
}

void MEF_Luz_SetTimer(uint8_t id, uint32_t duracion_ms) {
    if (id >= MAX_FOCOS) return;

    focos[id].estado = LUZ_TIMER;
    focos[id].t_start = tickRead();  // CAMBIO: tickRead() en vez de TimerService_GetTick()
    focos[id].t_delay = duracion_ms;
}

void MEF_Luz_Update(void) {
    uint32_t ahora = tickRead();

    for(int i=0; i<MAX_FOCOS; i++) {
        foco_t *f = &focos[i];

        switch(f->estado){
            case LUZ_OFF:
                gpioWrite(FocoGPIO[i], OFF);
                pwmWrite(FocoPWM[i], 0);
                break;

            case LUZ_ON:
                gpioWrite(FocoGPIO[i], ON);
                pwmWrite(FocoPWM[i], 255); // duty 100%
                break;

            case LUZ_DIMMING:
                if(f->habilitado_dimming){
                    pwmWrite(FocoPWM[i], (f->brillo * 255) / 100); // duty proporcional
                }
                break;

            case LUZ_TIMER:
                if (TimerService_Elapsed(f->t_start, f->t_delay)) {
                    f->estado = LUZ_OFF;
                    gpioWrite(FocoGPIO[i], OFF);
                    pwmWrite(FocoPWM[i], 0);
                } else {
                    gpioWrite(FocoGPIO[i], ON);
                }
                break;

            case SENSOR_LUZ: {
                uint16_t lux = adcRead(LUX_SENSOR); // CAMBIO: uso adcRead sapi
                uint8_t duty;
                if(lux < 200) duty = 255;
                else if(lux < 500) duty = 128;
                else duty = 0;

                pwmWrite(FocoPWM[i], duty);
            } break;
        }
    }
}
