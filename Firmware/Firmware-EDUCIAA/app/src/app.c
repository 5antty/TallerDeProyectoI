#include "sapi.h"
#include "sapi_uart.h"
#include "mef_alarma.h"
#include "mef_luces.h"
#include "mef_caloventor.h"
#include "mef_ui.h"
#include "manejo_uart.h"
#include "lvgl.h"
#include "lvgl_drivers.h"
#include "sapi_rtc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// --- DEFINICIONES DE PINES  --- en lvgl_drivers.h

// PIN_PIR ESTA DECLARADO EN ALARMA.H PIN_PIR EN GPIO6
#define GPIO_DHT11 GPIO8
#define PIN_LDR_DIGITAL GPIO7

// --- VARIABLES GLOBALES Y LVGL ---
static float ultima_temperatura_valida = 25.0;
static float ultima_humedad_valida = 50.0;
static uint32_t t_dht = 0;
static uint32_t t_uartESP = 0;
static uint32_t t_alarma = 0;
static uint32_t t_luces = 0;
static uint32_t t_ui_caloventor = 0;
static uint32_t t_lvgl = 0;
static uint32_t t_rtc_update = 0;
static uint32_t t_ui_update_status = 0;

static bool_t estado_ldr_anterior = FALSE;
static bool_t ultima_luminosidad_valida = FALSE;
static uint32_t t_ldr = 0;

// Variables LVGL/Touch
#define DISP_BUF_SIZE (240 * 17)
lv_disp_draw_buf_t disp_buf;
lv_color_t buf[DISP_BUF_SIZE];
static lv_indev_drv_t indev_drv;
int touch_press_count = 0;

// Variables para la lectura del PIR
static bool_t estado_pir_anterior = FALSE;
static uint32_t t_pir_muestra = 0;

// --- FUNCIONES EXTERNAS ---
extern void lvgl_spi_init(spiMap_t spi_bus);
extern void lvgl_touch_init(uint8_t cs, uint8_t irq);
extern bool lvgl_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
extern void lvgl_display_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);

static void btn_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        lv_obj_t *btn = lv_event_get_target(e);
        const char *btn_text = lv_label_get_text(lv_obj_get_child(btn, 0));

        char buffer[64];
        sprintf(buffer, "Bot?n tocado: %s\r\n", btn_text);
        uartWriteString(UART_USB, buffer);
    }
}

void lvgl_register_display(void)
{
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 320;
    disp_drv.flush_cb = lvgl_display_flush;
    lv_disp_drv_register(&disp_drv);
}

// esta funcion esta de adorno

void gui_init(void)
{

    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    extern void lvgl_display_init(uint8_t cs, uint8_t dc, uint8_t rst);
    lvgl_display_init(PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST);
}

// para inicializar la pantallita

void LVGL_Init_System(void)
{
    lvgl_spi_init(SPI_BUS);
    lv_init();

    lvgl_register_display();

    lvgl_touch_init(PIN_TOUCH_CS, PIN_TOUCH_IRQ);
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_read;
    lv_indev_drv_register(&indev_drv);
}

float get_current_temperature(void)
{
    return ultima_temperatura_valida;
}

float get_current_humidity(void)
{
    return ultima_humedad_valida;
}

bool_t get_current_luminosity(void)
{
    return ultima_luminosidad_valida;
}

uint8_t calcular_dia_semana(uint16_t anio, uint8_t mes, uint8_t dia)
{
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    anio -= mes < 3;
    int resultado = (anio + anio / 4 - anio / 100 + anio / 400 + t[mes - 1] + dia) % 7;

    if (resultado == 0)
        return 7;
    return (uint8_t)resultado;
}

// --- Bucle principal ---

int main(void)
{
    boardInit();

    // ====================================================
    // UART para comunicarse con ESP32
    // ====================================================
    uartConfig(UART_232, 9600); // UART_232 es la UART1 (RS232)
    // ====================================================
    uartConfig(UART_USB, 115200);

    tickConfig(1); // Tick de 1 ms
    uartWriteString(UART_USB, "Iniciando sistema integrado con LVGL...\r\n");
    gpioWrite(LEDB, ON);
    char msg[10];

    // ====================================================
    // 1. INICIALIZACION DE HARDWARE
    // ====================================================
    dht11Init(GPIO_DHT11);
    // gpioInit(PIN_PIR, GPIO_INPUT);
    gpioInit(PIN_LDR_DIGITAL, GPIO_INPUT);

    // Inicializacion de RTC
    rtc_t hora_por_defecto = {
        .year = 2026,
        .month = 2,
        .mday = 3,
        .hour = 12,
        .min = 0,
        .sec = 0,
        .wday = 2};

    rtc_t hora_actual;

    bool_t lectura_ok = rtcRead(&hora_actual);

    if (!lectura_ok || hora_actual.year < 2025)
    {
        rtcInit(&hora_por_defecto);
        uartWriteString(UART_USB, "RTC: Hora invalida/reset, se configuro por defecto.\r\n");
    }
    else
    {
        rtcInit(&hora_actual);
        uartWriteString(UART_USB, "RTC: Bateria OK. Hora mantenida.\r\n");
    }

    LVGL_Init_System();

    MEF_Alarm_Init();
    MEF_Luz_Init();
    MEF_Caloventor_Init();
    MEF_UI_INIT();
    UI_InitStyles();

    gui_init();
    UI_ShowPantallaPrincipal();

    t_alarma = tickRead();
    t_luces = tickRead();
    t_ui_caloventor = tickRead();
    t_lvgl = tickRead();
    t_dht = tickRead();

    t_uartESP = tickRead();

    t_pir_muestra = tickRead();
    t_rtc_update = tickRead();
    t_ui_update_status = tickRead();
    t_ldr = tickRead();

    uint32_t t_lvgl_tick = tickRead(); // Inicializa

    while (true)
    {
        uint32_t ahora = tickRead();

        uint32_t tiempo_transcurrido = ahora - t_lvgl_tick;
        if (tiempo_transcurrido > 0)
        {
            lv_tick_inc(tiempo_transcurrido); // Le sumamos los ms que pasaron
            t_lvgl_tick = ahora;
        }
        // ----------------------------------------------------
        // TAREA 0: LVGL HANDLER (TOUCH & DISPLAY) (cada 5 ms)
        // ----------------------------------------------------

        if ((int32_t)(ahora - t_lvgl) >= 5)
        {
            t_lvgl = ahora;
            lv_timer_handler();
        }

        // ----------------------------------------------------
        // TAREA 0.5: ACTUALIZACI?N DEL RTC (cada 1000 ms)
        // ----------------------------------------------------
        if ((int32_t)(ahora - t_rtc_update) >= 1000)
        {
            t_rtc_update = ahora;
            UI_UpdateRTC();
        }

        // ----------------------------------------------------
        // TAREA 1: MEF Alarma (cada 10 ms)
        // ----------------------------------------------------
        if ((int32_t)(ahora - t_alarma) >= 10)
        {
            t_alarma = ahora;
            MEF_Alarm_Update();
        }

        // ----------------------------------------------------
        // TAREA 1.5: LEER UART232 POR EL ESP32 (cada 20 ms)
        // ----------------------------------------------------
        if ((int32_t)(ahora - t_uartESP) >= 50)
        {
            t_uartESP = ahora;
            leer_uart_esp32();
        }

        // ----------------------------------------------------
        // TAREA 2: MEF Luces (cada 20 ms)
        // ----------------------------------------------------
        if ((int32_t)(ahora - t_luces) >= 20)
        {
            t_luces = ahora;
            MEF_Luz_Update();
        }

        // ----------------------------------------------------
        // TAREA 3: Lectura del DHT11 (cada 5000 ms)
        // ----------------------------------------------------
        if ((int32_t)(ahora - t_dht) >= 5000)
        {
            t_dht = ahora;

            float temp_leida = 0.0;
            float hum_leida = 0.0;

            if (dht11Read(&hum_leida, &temp_leida) == TRUE)
            {
                ultima_temperatura_valida = temp_leida;
                ultima_humedad_valida = hum_leida;

                snprintf(msg, sizeof(msg), "%.1f,%.1f", ultima_temperatura_valida, ultima_humedad_valida);
                txCmd('T', msg);
            }
            else
            {
                // uartWriteString(UART_USB, "DHT11: Error de lectura\r\n");
            }
        }

        // --- TAREA 4: MEF Caloventor y UI Update (cada 100 ms) ---
        if ((int32_t)(ahora - t_ui_caloventor) >= 100)
        {
            t_ui_caloventor = ahora;
            MEF_Caloventor_Update(ultima_temperatura_valida);
            UI_Update();
        }
        // ----------------------------------------------------
        // TAREA 4.5: ACTUALIZACI?N Rapida DE ESTADO DE ALARMA
        // ----------------------------------------------------
        if ((int32_t)(ahora - t_ui_update_status) >= 100)
        {
            t_ui_update_status = ahora;

            // Esta funcion es para cambiar la pantalla de mef alarma si esta activada o no
            // UI_UpdateAlarmStatus();
        }

        // ----------------------------------------------------
        // TAREA 5: Lectura del Sensor PIR (cada 50 ms)
        // ----------------------------------------------------
        if ((int32_t)(ahora - t_pir_muestra) >= 50)
        {
            t_pir_muestra = ahora;

            // Leo el estado del pin conectado al PIR
            bool_t estado_pir_actual = gpioRead(PIN_PIR);

            // Verifico si el estado ha cambiado
            if (estado_pir_actual != estado_pir_anterior)
            {
                if (estado_pir_actual == TRUE)
                {
                    uartWriteString(UART_USB, "PIR DETECCIoN: MOVIMIENTO DETECTADO!\r\n");
                }
                else
                {
                    uartWriteString(UART_USB, "PIR SIN DETECCIoN: Movimiento cesado\r\n");
                }
                // Actualizo el estado anterior
                estado_pir_anterior = estado_pir_actual;
            }
        }
        // ----------------------------------------------------
        // TAREA 6: Lectura del Sensor LDR (cada 50 ms)
        // ----------------------------------------------------
        if ((int32_t)(ahora - t_ldr) >= 1000)
        {
            t_ldr = ahora;

            // Leo el estado del pin
            bool_t estado_ldr_actual = gpioRead(PIN_LDR_DIGITAL);
            //ultima_luminosidad_valida = estado_ldr_actual;
           
            // Verifico si el estado ha cambiado
            if (estado_ldr_actual != estado_ldr_anterior)
            //{
                
                // Actualizo el estado y la variable global
                estado_ldr_anterior = estado_ldr_actual;
                ultima_luminosidad_valida = estado_ldr_actual;
            //}
        }
    }
}