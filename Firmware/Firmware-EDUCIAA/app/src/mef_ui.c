#include "mef_ui.h"
#include "sapi.h" 
#include "lvgl.h"
#include "lvgl_drivers.h" 
#include "mef_caloventor.h"
#include "mef_luces.h"
#include "mef_alarma.h"
#include "sapi_rtc.h" 
#include <stdio.h>
#include <stdlib.h>

//estas son las API de todas las mef ya implementadas
extern void MEF_Alarm_Pedido_Armar(void);
extern void MEF_Alarm_Pedido_Desarmar(void);
extern void MEF_Alarm_SetPIN(const char *pin); 
extern void pedido_entrada_PIN(uint8_t action, uint32_t timeout_ms); 
extern void MEF_Luz_SetMode(uint8_t id, luz_estado_t modo, uint8_t brillo);
extern bool_t get_current_luminosity(void); 
extern void MEF_Luz_SetTimerDelay(uint8_t id, uint32_t duracion_ms); 
extern void MEF_Luz_SetRTC_OnTime(uint8_t id, uint8_t hora, uint8_t min);
extern void MEF_Luz_SetRTC_OffTime(uint8_t id, uint8_t hora, uint8_t min);
extern void MEF_Caloventor_SetCalor(void);
extern void MEF_Caloventor_SetFrio(void);
extern void MEF_Caloventor_SetOff(void);
extern void MEF_Caloventor_SetAuto(void);
extern int MEF_Caloventor_GetUmbralFrio(void);
extern int MEF_Caloventor_GetUmbralCaliente(void);
extern luz_estado_t MEF_Luz_GetEstado(uint8_t id);

extern float get_current_temperature(void);
extern float get_current_humidity(void);
// --- VARIABLES INTERNAS DE LA MEF ---
static pantalla_estado_t estado_actual = PANTALLA_PRINCIPAL;

static lv_obj_t * label_time_panel = NULL; 
static lv_obj_t * label_date_panel = NULL; 
static lv_obj_t * label_day_panel = NULL;  
static lv_obj_t * label_alarm_status_main_panel = NULL;
static lv_obj_t * label_temp_value = NULL; 
static lv_obj_t * label_hum_value = NULL; 
static lv_obj_t * label_temp_txt = NULL; 
static lv_obj_t * label_hum_txt = NULL; 


// Definiciones de LVGL (Externs del main.c)-> corregir esto
extern lv_disp_draw_buf_t disp_buf;
extern lv_color_t buf[];

//Keypad
typedef enum {
    KEYPAD_ACTION_NONE = 0,
    KEYPAD_ACTION_ARM_ALARM,        
    KEYPAD_ACTION_VERIFY_OLD_PIN,   
    KEYPAD_ACTION_SET_NEW_PIN,          
    KEYPAD_ACTION_LIGHT_DELAY_TIME, 
    KEYPAD_ACTION_LIGHT_RTC_ON_TIME, 
    KEYPAD_ACTION_LIGHT_RTC_OFF_TIME,
} keypad_action_t;

typedef struct {
    keypad_action_t action;
    bool is_password;       
    uint8_t max_length;     
    pantalla_estado_t return_screen;
    uint8_t min_length;     
    lv_obj_t * ta_ref;
} keypad_config_t;


// --- PROTOTIPOS DE CALLBACKS ---
static void callback_cambio_pantalla(lv_event_t * e);
static void callback_transicion_armar(lv_event_t * e);
static void callback_transicion_desarmar(lv_event_t * e);
static void callback_transicion_cambiar_pin(lv_event_t * e);
static void cambiar_pantalla(pantalla_estado_t nuevo_estado);
static void callback_foco_on_off(lv_event_t * e);
static void callback_foco_brillo_fijo(lv_event_t * e);
static void callback_transicion_luz_delay(lv_event_t * e);
static void callback_transicion_luz_rtc(lv_event_t * e);
static void callback_caloventor_frio_on_off(lv_event_t * e);
static void callback_caloventor_calor_on_off(lv_event_t * e);
static void callback_caloventor_toggle(lv_event_t * e);
static void callback_config_alarma_sensores(lv_event_t * e);
static void callback_caloventor_auto_toggle(lv_event_t * e);
static void callback_transicion_a_keypad(lv_event_t * e);
static void callback_manual_calor_toggle(lv_event_t * e);
static void callback_manual_ventilador_toggle(lv_event_t * e);
static void callback_auto_mode_toggle(lv_event_t * e);
void UI_ShowKeypadInput(keypad_config_t * config);


static lv_obj_t * ptr_btn_auto = NULL;



static void cambiar_pantalla(pantalla_estado_t nuevo_estado) {
    if (nuevo_estado == estado_actual) return;
   
    label_time_panel = NULL;
    label_date_panel = NULL;
    label_day_panel = NULL;
    label_alarm_status_main_panel = NULL;
    label_temp_value = NULL; 
    label_hum_value = NULL;
    
    lv_obj_clean(lv_scr_act());
    
    
    estado_actual = nuevo_estado;
    char buffer[64];
    sprintf(buffer, "NAVEGACION: Transicion a estado %d\r\n", nuevo_estado);
    uartWriteString(UART_USB, buffer);
    
    switch (estado_actual) {
        case PANTALLA_ALARMA:
            UI_ShowPantallaAlarma();
            break;
        case PANTALLA_LUCES:
            UI_ShowPantallaLuces();
            break;
        case PANTALLA_TEMP_HUM:
            UI_ShowPantallaTempHum();
            break;
        case PANTALLA_CONFIG_ALARMA:
            UI_ShowPantallaConfigAlarma();
            break;
        case PANTALLA_CONFIG_LUCES:
            UI_ShowPantallaConfigLuces();
            break;
        case PANTALLA_CONFIG_TEMP_HUM:
            UI_ShowPantallaConfigTempHum();
            break;
        case PANTALLA_PRINCIPAL:
        default:
            UI_ShowPantallaPrincipal();
            break;
    }
}



static lv_style_t style_decoration_base;
static lv_style_t style_label_text;
static lv_style_t style_label_white;
static lv_style_t style_gradient_alarma;
static lv_style_t style_gradient_luces;
static lv_style_t style_gradient_temp;
static lv_style_t style_gradient_armar;
static lv_style_t style_gradient_desarmar;
static lv_style_t style_gradient_cambiar_pin;


void UI_InitStyles(void) {
   
   
    // Inicializaci�n Base
    lv_style_init(&style_decoration_base);
    lv_style_set_radius(&style_decoration_base, 10);
    lv_style_set_shadow_width(&style_decoration_base, 10);
    lv_style_set_shadow_opa(&style_decoration_base, LV_OPA_40);
    
    // Estilos de Texto
    lv_style_init(&style_label_text);
    lv_style_set_text_font(&style_label_text, &lv_font_montserrat_14);
    lv_style_set_text_color(&style_label_text, lv_color_white());
    
    lv_style_init(&style_label_white);
    lv_style_set_text_font(&style_label_white, &lv_font_montserrat_14);
    lv_style_set_text_color(&style_label_white, lv_color_white());

    
    // GRADIENTE ALARMA
    static const lv_color_t grad_colors_alarma[2] = {
        LV_COLOR_MAKE(0x50, 0x00, 0x00), 
        LV_COLOR_MAKE(0xFF, 0x33, 0x33) 
    };
    lv_style_init(&style_gradient_alarma);
    lv_style_set_radius(&style_gradient_alarma, 10);
    lv_style_set_shadow_width(&style_gradient_alarma, 10);
    lv_style_set_shadow_opa(&style_gradient_alarma, LV_OPA_40);
    lv_style_set_bg_color(&style_gradient_alarma, grad_colors_alarma[1]);
    lv_style_set_bg_grad_color(&style_gradient_alarma, grad_colors_alarma[0]);
    lv_style_set_bg_grad_dir(&style_gradient_alarma, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_gradient_alarma, LV_OPA_COVER);
    lv_style_set_border_width(&style_gradient_alarma, 2);
    lv_style_set_border_color(&style_gradient_alarma, lv_color_white());

    // GRADIENTE LUCES
    static const lv_color_t grad_colors_luces[2] = {
        LV_COLOR_MAKE(0x00, 0x50, 0x00), 
        LV_COLOR_MAKE(0x33, 0xFF, 0x33) 
    };
    lv_style_init(&style_gradient_luces);
    lv_style_set_radius(&style_gradient_luces, 10);
    // ... (restantes propiedades de LUCES copiadas de ALARMA) ...
    lv_style_set_bg_color(&style_gradient_luces, grad_colors_luces[1]);
    lv_style_set_bg_grad_color(&style_gradient_luces, grad_colors_luces[0]);
    lv_style_set_bg_grad_dir(&style_gradient_luces, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_gradient_luces, LV_OPA_COVER);
    lv_style_set_border_width(&style_gradient_luces, 2);
    lv_style_set_border_color(&style_gradient_luces, lv_color_white());
    
    // GRADIENTE TEMP/HUM
    static const lv_color_t grad_colors_temp[2] = {
        LV_COLOR_MAKE(0x50, 0x20, 0x10), 
        LV_COLOR_MAKE(0xFF, 0x96, 0x00) 
    };
    lv_style_init(&style_gradient_temp);
    lv_style_set_radius(&style_gradient_temp, 10);
 
    lv_style_set_bg_color(&style_gradient_temp, grad_colors_temp[1]);
    lv_style_set_bg_grad_color(&style_gradient_temp, grad_colors_temp[0]);
    lv_style_set_bg_grad_dir(&style_gradient_temp, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_gradient_temp, LV_OPA_COVER);
    lv_style_set_border_width(&style_gradient_temp, 2);
    lv_style_set_border_color(&style_gradient_temp, lv_color_white());
    
    
    // GRADIENTE ARMAR (Verde)
    static const lv_color_t grad_colors_armar_spec[2] = {
        LV_COLOR_MAKE(0x00, 0x50, 0x00),
        LV_COLOR_MAKE(0x33, 0xFF, 0x33)
    };
    lv_style_init(&style_gradient_armar);
    lv_style_set_radius(&style_gradient_armar, 10);
    // ... (copiar todas las propiedades de decoraci�n de ALARMA) ...
    lv_style_set_bg_color(&style_gradient_armar, grad_colors_armar_spec[1]);
    lv_style_set_bg_grad_color(&style_gradient_armar, grad_colors_armar_spec[0]);
    lv_style_set_bg_grad_dir(&style_gradient_armar, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_gradient_armar, LV_OPA_COVER);
    lv_style_set_border_width(&style_gradient_armar, 2);
    lv_style_set_border_color(&style_gradient_armar, lv_color_white());

    // GRADIENTE DESARMAR (Rojo)
    static const lv_color_t grad_colors_desarmar_spec[2] = {
        LV_COLOR_MAKE(0x50, 0x00, 0x00),
        LV_COLOR_MAKE(0xFF, 0x33, 0x33)
    };
    lv_style_init(&style_gradient_desarmar);
    lv_style_set_radius(&style_gradient_desarmar, 10);
    // ... (copiar todas las propiedades de decoraci�n de ALARMA) ...
    lv_style_set_bg_color(&style_gradient_desarmar, grad_colors_desarmar_spec[1]);
    lv_style_set_bg_grad_color(&style_gradient_desarmar, grad_colors_desarmar_spec[0]);
    lv_style_set_bg_grad_dir(&style_gradient_desarmar, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_gradient_desarmar, LV_OPA_COVER);
    lv_style_set_border_width(&style_gradient_desarmar, 2);
    lv_style_set_border_color(&style_gradient_desarmar, lv_color_white());
    
    // GRADIENTE CAMBIAR PIN (Gris)
    static const lv_color_t grad_colors_cambiar_pin_spec[2] = {
        LV_COLOR_MAKE(0x10, 0x10, 0x10),
        LV_COLOR_MAKE(0xA0, 0xA0, 0xA0) 
    };
    lv_style_init(&style_gradient_cambiar_pin);
    lv_style_set_radius(&style_gradient_cambiar_pin, 10);
    // ... (copiar todas las propiedades de decoraci�n de ALARMA) ...
    lv_style_set_bg_color(&style_gradient_cambiar_pin, grad_colors_cambiar_pin_spec[1]);
    lv_style_set_bg_grad_color(&style_gradient_cambiar_pin, grad_colors_cambiar_pin_spec[0]);
    lv_style_set_bg_grad_dir(&style_gradient_cambiar_pin, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_gradient_cambiar_pin, LV_OPA_COVER);
    lv_style_set_border_width(&style_gradient_cambiar_pin, 2);
    lv_style_set_border_color(&style_gradient_cambiar_pin, lv_color_white());
}

// --- CALLBACKS DE BOTONES ---
static void callback_cambio_pantalla(lv_event_t * e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    pantalla_estado_t * nuevo_estado_ptr = (pantalla_estado_t *)lv_event_get_user_data(e);

    if (nuevo_estado_ptr != NULL) {
        cambiar_pantalla(*nuevo_estado_ptr);
    } else {
        uartWriteString(UART_USB, "ERROR: Callback sin destino\r\n");
    }
}

static void callback_transicion_armar(lv_event_t * e){
   if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
      
     char buffer[100];
     sprintf(buffer, "ALARMA: Pedido de ARMADO (PIN)\r\n");
     uartWriteString(UART_USB, buffer);
   
   keypad_config_t * config = (keypad_config_t *)lv_event_get_user_data(e);
   
   if (config != NULL) {

      MEF_Alarm_Pedido_Armar(); 
      UI_ShowKeypadInput(config);
   }else{
      uartWriteString(UART_USB, "ERROR: Config armar es NULL.\r\n");
   }
}

static void callback_transicion_desarmar(lv_event_t * e){
   if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
      
     char buffer[100];
     sprintf(buffer, "ALARMA: Pedido de DESARMAR (PIN)\r\n");
     uartWriteString(UART_USB, buffer);
   
   keypad_config_t * config = (keypad_config_t *)lv_event_get_user_data(e);
   
   if (config != NULL) {
      MEF_Alarm_Pedido_Desarmar(); 
      UI_ShowKeypadInput(config);
   }else{
      uartWriteString(UART_USB, "ERROR: Config desarmar es NULL.\r\n");
   }
}

static void callback_transicion_cambiar_pin(lv_event_t * e){
   if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
      
   
   alarma_estado_t estado = get_alarm_state();
   if (estado != ALM_DESARMADO) {
       uartWriteString(UART_USB, "ERROR: No puede cambiar PIN si la alarma est� activa.\r\n");
       return; 
   }
   // ---------------------------------------------------------------

   char buffer[100];
   sprintf(buffer, "CONFIG: Pedido Validacion PIN ACTUAL\r\n");
   uartWriteString(UART_USB, buffer);
   
   keypad_config_t * config = (keypad_config_t *)lv_event_get_user_data(e);
   
   if (config != NULL) {

      config->action = KEYPAD_ACTION_VERIFY_OLD_PIN; 
      
      // 2. Configurar y mostrar el teclado
      UI_ShowKeypadInput(config);
   } else {
      uartWriteString(UART_USB, "ERROR: Config cambiar pin es NULL.\r\n");
   }
}

static void callback_transicion_luz_delay(lv_event_t * e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;


    static keypad_config_t config_delay; 
    
    // Configuraci�n espec�fica de DELAY
    config_delay.action = KEYPAD_ACTION_LIGHT_DELAY_TIME;
    config_delay.is_password = false;
    config_delay.min_length = 1;  // M�nimo 1 d�gito
    config_delay.max_length = 4;  // MMMM minutos (ej. 1200 minutos)
    config_delay.return_screen = PANTALLA_PRINCIPAL; 

    // 1. Informar a la UART
    uartWriteString(UART_USB, "LUCES: Transicion a Keypad para MODO DELAY.\r\n");
    
    // 2. Mostrar el teclado
    UI_ShowKeypadInput(&config_delay);
}

static void callback_transicion_luz_rtc(lv_event_t * e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;


    static keypad_config_t config_rtc; 
    
    // Siempre empezamos pidiendo la hora de Encendido (ON)
    config_rtc.action = KEYPAD_ACTION_LIGHT_RTC_ON_TIME;
    config_rtc.is_password = false;
    config_rtc.min_length = 4; // HHMM
    config_rtc.max_length = 4; // HHMM
    config_rtc.return_screen = PANTALLA_PRINCIPAL; 

    // 1. Informar a la UART
    uartWriteString(UART_USB, "LUCES: Transicion a Keypad para RTC ON Time.\r\n");
    
    // 2. Mostrar el teclado
    UI_ShowKeypadInput(&config_rtc);
}

//Funcion para el rtc
const char * dias_semana[] = {"", "LUNES", "MARTES", "MIERCOLES", "JUEVES", "VIERNES", "SABADO", "DOMINGO"};

void UI_UpdateRTC(void) {
    rtc_t rtc_time;
    char buffer_time[10];
    char buffer_date[10];
    
    if (rtcRead(&rtc_time) == TRUE) {
        if (label_time_panel != NULL) { 
            snprintf(buffer_time, sizeof(buffer_time), "%02d:%02d:%02d", 
                     rtc_time.hour, rtc_time.min, rtc_time.sec);
            lv_label_set_text(label_time_panel, buffer_time);
        }

        if (label_date_panel != NULL) { // <-- �Protecci�n cr�tica!
             snprintf(buffer_date, sizeof(buffer_date), "%02d/%02d", 
                      rtc_time.mday, rtc_time.month);
             lv_label_set_text(label_date_panel, buffer_date);
        }

        if (label_day_panel != NULL) {
          lv_label_set_text(label_day_panel, dias_semana[2]);     //DEFAULT EN MARTES
           
           /*
          if (rtc_time.wday >= 1 && rtc_time.wday <= 7) {
              lv_label_set_text(label_day_panel, dias_semana[rtc_time.wday]);
          } 
          else {
              lv_label_set_text(label_day_panel, "ERROR DIA"); 
          }
           */
      }
    }
}

//Esta funcion sabe de verdad y extrae el estado de mef alarma y los sensores que estan leyendo los pines y estan activos
//actua en funcion de lo que quiere el usuario
void UI_UpdateAlarmStatus(void) {
    // 1. Si no existe el panel, nos vamos
    if (label_alarm_status_main_panel == NULL) return;
    
    alarma_estado_t estado = get_alarm_state();
    uint8_t sensores = MEF_Alarm_GetSensoresConfigurados();

    lv_color_t color;
    char text_buffer[32]; 

    switch (estado) {
        case ALM_ARMADO:
        case ALM_ARMANDO:
        case ALM_IDENTIFICACION:
            if(estado == ALM_ARMADO) color = lv_color_make(0xFF, 0x33, 0x33); 
            else color = lv_color_make(0xFF, 0xA7, 0x26);

            const char* estado_str = (estado == ALM_ARMADO) ? "ARMADO" : "PROCESO...";
            
            if (sensores == SENSOR_AMBOS) snprintf(text_buffer, sizeof(text_buffer), "%s\n(AMBOS)", estado_str);
            else if (sensores == SENSOR_MOVIMIENTO) snprintf(text_buffer, sizeof(text_buffer), "%s\n(PIR)", estado_str);
            else if (sensores == SENSOR_MAGNETICO) snprintf(text_buffer, sizeof(text_buffer), "%s\n(PUERTA)", estado_str);
            else snprintf(text_buffer, sizeof(text_buffer), "%s\n(ERROR)", estado_str);
            break;

        case ALM_ACTIVADO:
            color = lv_color_make(0xCC, 0x00, 0x00);
            snprintf(text_buffer, sizeof(text_buffer), "�ALERTA!\nINTRUSO");
            break;

        case ALM_DESARMADO:
        default:
            color = lv_color_make(0x00, 0xCC, 0x00);
            if (sensores == SENSOR_AMBOS) snprintf(text_buffer, sizeof(text_buffer), "DESARMADO\n(AMBOS)");
            else if (sensores == SENSOR_MOVIMIENTO) snprintf(text_buffer, sizeof(text_buffer), "DESARMADO\n(PIR)");
            else if (sensores == SENSOR_MAGNETICO) snprintf(text_buffer, sizeof(text_buffer), "DESARMADO\n(PUERTA)");
            break;
    }
    
    // 2. Aplicar Color al Bot�n
    lv_obj_set_style_bg_color(label_alarm_status_main_panel, color, LV_PART_MAIN);

    // 3. Aplicar Texto y AJUSTAR TAMA�O
    lv_obj_t * status_label = lv_obj_get_child(label_alarm_status_main_panel, 0);
    
    if (status_label != NULL) {
        lv_label_set_text(status_label, text_buffer);
        
        lv_obj_set_style_text_font(status_label, &lv_font_montserrat_10, LV_PART_MAIN); 
        
        lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    }
}

//LUCES
typedef struct {
    uint8_t foco_id;
    lv_obj_t * lbl_feedback;
} foco_data_t;

static foco_data_t foco1_data = { .foco_id = 0, .lbl_feedback = NULL };
static foco_data_t foco2_data = { .foco_id = 1, .lbl_feedback = NULL };



typedef struct {
    uint8_t foco_id;
    uint8_t brillo;
    lv_obj_t * lbl_feedback; 
} brillo_config_t;

static brillo_config_t f1_b25 = {0, 26, NULL};
static brillo_config_t f1_b50 = {0, 40, NULL};
static brillo_config_t f1_b75 = {0, 75, NULL};

// Configuraciones de brillo Foco 2
static brillo_config_t f2_b25 = {1, 26, NULL};
static brillo_config_t f2_b50 = {1, 40, NULL};
static brillo_config_t f2_b75 = {1, 75, NULL};

//esto es para el feedback del boton de lzu automatica

typedef struct{
   lv_obj_t * label_status;
}config_luz_data_t;

static config_luz_data_t data_luz_auto={  .label_status=NULL   };


static void callback_foco_on_off(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    foco_data_t * data = (foco_data_t *)lv_event_get_user_data(e);
    
    // 1. Leemos estado visual
    bool encendido = lv_obj_has_state(btn, LV_STATE_CHECKED);

    // 2. Feedback UART
    char buffer[64];
    sprintf(buffer, "Foco %d: %s\r\n", data->foco_id, encendido ? "ON" : "OFF");
    uartWriteString(UART_USB, buffer);

    // 3. Feedback Visual
    if(data->lbl_feedback != NULL) {
        if(encendido) {
            lv_label_set_text(data->lbl_feedback, "Estado: PRENDIDO");
            lv_obj_set_style_text_color(data->lbl_feedback, lv_color_make(50, 255, 50), 0); 
        } else {
            lv_label_set_text(data->lbl_feedback, "Estado: APAGADO");
            lv_obj_set_style_text_color(data->lbl_feedback, lv_color_make(255, 100, 100), 0); 
        }
    }
    

    if (encendido) {

        MEF_Luz_SetMode(data->foco_id, LUZ_ON, 80); 
    } else {
        // Al apagar
        MEF_Luz_SetMode(data->foco_id, LUZ_OFF, 0);
    }
}

static void callback_foco_brillo_fijo(lv_event_t * e) {
    brillo_config_t * conf = (brillo_config_t *)lv_event_get_user_data(e);
    
    // 1. UART
    char buffer[64];
    sprintf(buffer, "Foco %d Brillo: %d%%\r\n", conf->foco_id, conf->brillo);
    uartWriteString(UART_USB, buffer);

    // 2. Feedback Visual
    if(conf->lbl_feedback != NULL) {
        lv_label_set_text_fmt(conf->lbl_feedback, "Brillo: %d%% OK", conf->brillo);
        lv_obj_set_style_text_color(conf->lbl_feedback, lv_color_make(255, 255, 0), 0);
    }
    

    MEF_Luz_SetMode(conf->foco_id, LUZ_DIMMING, conf->brillo);
}


typedef struct { 
   int mode_on; 
   int mode_off; 
   }
   calo_mode_data_t;
   
static calo_mode_data_t calo_calor_data = { .mode_on = 1, .mode_off = 0 };
static calo_mode_data_t calo_frio_data = { .mode_on = 2, .mode_off = 0 };

typedef struct{
   uint8_t id;
   lv_obj_t * label_estado;
}caloventor_data_t;

static lv_obj_t * ptr_btn_calor = NULL;
static lv_obj_t * ptr_btn_vent = NULL;

static caloventor_data_t mi_caloventor = { .id = 0, .label_estado = NULL };

typedef struct{
   uint8_t id;
   lv_obj_t * label_estado;
}ventilador_data_t;

static ventilador_data_t mi_ventilador = { .id = 0, .label_estado = NULL };

// Callback para el bot�n CALOVENTOR (CALOR)
static void callback_manual_calor_toggle(lv_event_t * e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    
    lv_obj_t * btn = lv_event_get_target(e);
    caloventor_data_t * data = (caloventor_data_t *)lv_event_get_user_data(e);
    
    bool encendido = lv_obj_has_state(btn, LV_STATE_CHECKED);
    
    if (encendido) {
        // 1. ENCENDER CALOR
        MEF_Caloventor_SetCalor(); 
        uartWriteString(UART_USB, "UI: Manual CALOR ON\r\n");
        
        // 2. APAGAR EL OTRO (VENTILADOR) SI ESTABA PRENDIDO
        if (ptr_btn_vent != NULL && lv_obj_has_state(ptr_btn_vent, LV_STATE_CHECKED)) {
            lv_obj_clear_state(ptr_btn_vent, LV_STATE_CHECKED); // Soltar bot�n visualmente
            
            // �IMPORTANTE! Actualizar tambi�n el TEXTO del otro label a "Apagado"
            if (mi_ventilador.label_estado != NULL) {
                lv_label_set_text(mi_ventilador.label_estado, "VENTILADOR: APAGADO");
                lv_obj_set_style_text_color(mi_ventilador.label_estado, lv_color_hex(0x888888), 0);
            }
        }
    } else {
        // APAGAR CALOR
        MEF_Caloventor_SetOff();
        uartWriteString(UART_USB, "UI: Manual OFF\r\n");
    }
    
    // 3. Feedback del propio bot�n (Calor)
    if (data->label_estado != NULL) {
        if (encendido) {
            lv_label_set_text(data->label_estado, "CALOVENTOR: ACTIVO");
            lv_obj_set_style_text_color(data->label_estado, lv_color_hex(0xFF4444), 0); // Rojo
        } else {
            lv_label_set_text(data->label_estado, "CALOVENTOR: APAGADO");
            lv_obj_set_style_text_color(data->label_estado, lv_color_hex(0x888888), 0); // Gris
        }
    }
}

static void callback_manual_ventilador_toggle(lv_event_t * e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    
    lv_obj_t * btn = lv_event_get_target(e);
    ventilador_data_t * data = (ventilador_data_t *)lv_event_get_user_data(e);
    
    bool encendido = lv_obj_has_state(btn, LV_STATE_CHECKED);

    if (encendido) {
        // 1. ENCENDER VENTILADOR
        MEF_Caloventor_SetVentilador();
        uartWriteString(UART_USB, "UI: Manual VENTILADOR ON\r\n");
        
        // 2. APAGAR EL OTRO (CALOR) SI ESTABA PRENDIDO
        if (ptr_btn_calor != NULL && lv_obj_has_state(ptr_btn_calor, LV_STATE_CHECKED)) {
            lv_obj_clear_state(ptr_btn_calor, LV_STATE_CHECKED);
            
            // �IMPORTANTE! Actualizar tambi�n el TEXTO del otro label a "Apagado"
            if (mi_caloventor.label_estado != NULL) {
                lv_label_set_text(mi_caloventor.label_estado, "CALOVENTOR: APAGADO");
                lv_obj_set_style_text_color(mi_caloventor.label_estado, lv_color_hex(0x888888), 0);
            }
        }
    } else {
        // APAGAR VENTILADOR
        MEF_Caloventor_SetOff();
        uartWriteString(UART_USB, "UI: Manual OFF\r\n");
    }
    
    // 3. Feedback del propio bot�n (Ventilador)
    if (data->label_estado != NULL) {
        if (encendido) {
            lv_label_set_text(data->label_estado, "VENTILADOR: ACTIVO");
            // SUGERENCIA: Usar color CYAN o AZUL para fr�o, queda mejor que rojo
            lv_obj_set_style_text_color(data->label_estado, lv_color_hex(0x00FFFF), 0); 
        } else {
            lv_label_set_text(data->label_estado, "VENTILADOR: APAGADO");
            lv_obj_set_style_text_color(data->label_estado, lv_color_hex(0x888888), 0); 
        }
    }
}

typedef struct {
    lv_obj_t * label_status; // Aqu� guardaremos la direcci�n del texto del panel derecho
} auto_mode_data_t;

// 2. Instancia est�tica (para que no se borre de la memoria)
static auto_mode_data_t data_modo_auto = { .label_status = NULL };

// Callback para el Switch de AUTO en Configuraci�n
static void callback_auto_mode_toggle(lv_event_t * e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    
    // A. Recuperamos objetos
    lv_obj_t * btn = lv_event_get_target(e);
    auto_mode_data_t * data = (auto_mode_data_t *)lv_event_get_user_data(e); // <--- Abrimos el sobre
    
    // B. Leemos estado
    bool activo = lv_obj_has_state(btn, LV_STATE_CHECKED);
    
    if (activo) {
        // Estado: PRESIONADO -> Activar Auto
        MEF_Caloventor_SetAuto();
        uartWriteString(UART_USB, "UI Config: Modo AUTOMATICO Activado\r\n");
    } else {
        // Estado: LIBERADO -> Desactivar Auto (Volver a Manual/Off)
        MEF_Caloventor_SetOff(); 
        uartWriteString(UART_USB, "UI Config: Modo AUTOMATICO Desactivado\r\n");
    }

    // C. --- FEEDBACK VISUAL EN EL PANEL DERECHO ---
    if (data->label_status != NULL) {
        if (activo) {
            lv_label_set_text(data->label_status, "SISTEMA: AUTO");
            // Color Verde o Cyan para resaltar que el cerebro controla todo
            lv_obj_set_style_text_color(data->label_status, lv_color_make(50, 255, 255), 0); 
        } else {
            lv_label_set_text(data->label_status, "SISTEMA: MANUAL");
            // Color Gris o Blanco
            lv_obj_set_style_text_color(data->label_status, lv_color_make(180, 180, 180), 0); 
        }
    }
}

void UI_UpdateTempHum(void) {

    if (label_temp_value == NULL || label_hum_value == NULL) {
        return;
    }

    float temp = get_current_temperature();
    float hum = get_current_humidity();
    char buffer_temp[15];
    char buffer_hum[15];

    // Formatear Temperatura (ej: 25.5 �C)
    snprintf(buffer_temp, sizeof(buffer_temp), "%.1f �C", temp);
    lv_label_set_text(label_temp_value, buffer_temp);

    // Formatear Humedad (ej: 50.0 %)
    snprintf(buffer_hum, sizeof(buffer_hum), "%.1f %%", hum);
    lv_label_set_text(label_hum_value, buffer_hum);
}

//CONFIG ALARMA
typedef struct {
    uint8_t sensor_mode; // Usaremos los valores de alarma_sensores_t (1, 2, 3)
} alarma_config_data_t;

static alarma_config_data_t config_ambos_data = { .sensor_mode = SENSOR_AMBOS }; // 3
static alarma_config_data_t config_movimiento_data = { .sensor_mode = SENSOR_MOVIMIENTO }; // 2
static alarma_config_data_t config_magnetico_data = { .sensor_mode = SENSOR_MAGNETICO }; // 1

// --- CALLBACK PARA CONFIGURAR SENSORES DE ALARMA ---
static void callback_config_alarma_sensores(lv_event_t * e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    alarma_config_data_t * data = (alarma_config_data_t *)lv_event_get_user_data(e);
    uint8_t modo_sensor = data->sensor_mode;
    char buffer[100];

    // Llama a la API de la MEF para configurar los sensores
    MEF_Alarm_SetSensores(modo_sensor);

    // Mensaje de depuraci�n (opcional, pero �til)
    if (modo_sensor == SENSOR_AMBOS) {
        sprintf(buffer, "ALARMA: Sensores configurados a AMBOS (MOV + MAG)\r\n");
    } else if (modo_sensor == SENSOR_MOVIMIENTO) {
        sprintf(buffer, "ALARMA: Sensores configurados a MOVIMIENTO\r\n");
    } else if (modo_sensor == SENSOR_MAGNETICO) {
        sprintf(buffer, "ALARMA: Sensores configurados a MAGNETICO\r\n");
    } else {
        sprintf(buffer, "ALARMA: Sensores configurados a modo desconocido (%d)\r\n", modo_sensor);
    }
    uartWriteString(UART_USB, buffer);
}

//--------CONFIGURACION LUCES------------
typedef enum {
    CONFIG_ACTION_DELAY = 1, // Para el bot�n MODO DELAY
    CONFIG_ACTION_RTC = 2    // Para el bot�n CONFIG TIEMPO
} luz_config_action_t;

// Datos de usuario que se pasan al callback de tiempo
static luz_config_action_t action_delay_data = CONFIG_ACTION_DELAY;
static luz_config_action_t action_rtc_data = CONFIG_ACTION_RTC;

// --- CALLBACK PARA EL MODO AUTOM�TICO (LDR) ---
static void callback_luz_modo_sensor(lv_event_t * e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
       
     lv_obj_t * btn = lv_event_get_target(e);
    config_luz_data_t * data = (config_luz_data_t *)lv_event_get_user_data(e);
    
    bool activo = lv_obj_has_state(btn, LV_STATE_CHECKED);
    
    if (data->label_status != NULL) {
        if (activo) {
            lv_label_set_text(data->label_status, "SENSOR: ON");
            // Color Verde Brillante para resaltar
            lv_obj_set_style_text_color(data->label_status, lv_color_make(50, 255, 50), 0);
        } else {
            lv_label_set_text(data->label_status, "SENSOR: OFF");
            // Color Blanco o Gris para estado inactivo
            lv_obj_set_style_text_color(data->label_status, lv_color_white(), 0);
        }
    }
    
    uartWriteString(UART_USB, activo ? "Luz Automatica ACTIVADA\r\n" : "Luz Automatica DESACTIVADA\r\n");

    // Foco ID 0 se usa por defecto
    
    // 1. Llama a la API para establecer el modo de sensor de luz
    MEF_Luz_SetMode(0, SENSOR_LUZ, 0); 
    MEF_Luz_SetMode(1, SENSOR_LUZ, 0); 
    uartWriteString(UART_USB, "LUCES: Foco 1 configurado a Control Autom�tico (LDR).\r\n");
}


// --- CALLBACK PARA INICIAR CONFIGURACI�N DE TIEMPO (DELAY o RTC) ---
static void callback_luz_config_tiempo(lv_event_t * e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    luz_config_action_t action_value = *(luz_config_action_t *)lv_event_get_user_data(e);
    
    // Crear y configurar la estructura para el Keypad (Usar static/heap seg�n tu elecci�n)
    // Asumo que usar�s memoria est�tica (o heap con calloc) para estas estructuras:
    static keypad_config_t config_luz; 
    
    config_luz.is_password = false; // No es PIN
    config_luz.min_length = 4;
    config_luz.max_length = 4;      // Esperamos HHMM o MMMM (4 d�gitos)
    config_luz.return_screen = PANTALLA_CONFIG_LUCES; 

    if (action_value == CONFIG_ACTION_DELAY) {
        config_luz.action = KEYPAD_ACTION_LIGHT_DELAY_TIME;
        
        uartWriteString(UART_USB, "LUCES: Modo DELAY activado. Ingrese MMMM (minutos).\r\n");

    } else if (action_value == CONFIG_ACTION_RTC) {
        // Para RTC, simplificaremos a un solo bot�n que primero pide ON y luego, con la misma acci�n, pide OFF.
        // Pero por ahora, configuraremos para pedir la hora de Encendido (ON).
        config_luz.action = KEYPAD_ACTION_LIGHT_RTC_ON_TIME;
        
        uartWriteString(UART_USB, "LUCES: Modo RTC activado. Ingrese HHMM (ON Time).\r\n");
    }
    
    UI_ShowKeypadInput(&config_luz);
}

//callback de configuracion de caloventor

typedef enum {
    CALO_ACTION_OFF = 0,
    CALO_ACTION_AUTO = 1,
    CALO_ACTION_CONFIG = 2 // para el nuevo bot�n de configuraci�n
} calo_action_t;
static calo_action_t calo_auto_data = CALO_ACTION_AUTO;
static calo_action_t calo_off_data = CALO_ACTION_OFF;
static calo_action_t calo_config_data = CALO_ACTION_CONFIG;


// --- PROTOTIPO DE CALLBACK MODIFICADO ---

static void callback_caloventor_auto_toggle(lv_event_t * e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    
    lv_obj_t * btn_actual = lv_event_get_target(e);
    calo_action_t action = *(calo_action_t *)lv_event_get_user_data(e);
    char buffer[100];

    // La l�gica aqu� es simple: si el bot�n est� CHECKED, vamos a MODO AUTO (ON). Si no, vamos a MODO MANUAL OFF.
    if (lv_obj_has_state(btn_actual, LV_STATE_CHECKED)) {
        // --- ENCENDER AUTOM�TICO ---
        MEF_Caloventor_SetAuto();
        sprintf(buffer, "CALOVENTOR: Control AUTOM�TICO ON.\r\n");
    } else {
        // --- APAGAR FORZADO ---
        MEF_Caloventor_SetOff(); 
        sprintf(buffer, "CALOVENTOR: Control AUTOM�TICO OFF (Forzar Apagado Manual).\r\n");
    }
    uartWriteString(UART_USB, buffer);
}

//Keypad



static void keypad_input_event_handler(lv_event_t * e) {
    lv_obj_t * btnm = lv_event_get_target(e);
    keypad_config_t * config = (keypad_config_t *)lv_event_get_user_data(e);
    lv_obj_t * ta = config->ta_ref; 
   
   if (ta == NULL) { return; }
   

    uint32_t id = lv_btnmatrix_get_selected_btn(btnm); 
    const char * txt = lv_btnmatrix_get_btn_text(btnm, id); 
    
    if (txt == NULL || lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    // CR�TICO: Usamos lv_btnmatrix_get_textarea para obtener el TA asociado
    lv_obj_set_user_data(btnm, config);
   
    
    const char * input_value = lv_textarea_get_text(ta);  //aca se escribe el pin o lo que ingresaste
    int len = strlen(input_value);
    char buffer[150];

    if (strcmp(txt, "OK") == 0) {
       uint32_t valor_ingresado = strtoul(input_value, NULL, 10);
       uint8_t foco_id = 0;
        if (len < config->min_length) {
            uartWriteString(UART_USB, "ERROR: Entrada muy corta.\r\n");
            return;
        }

       if (config->action == KEYPAD_ACTION_VERIFY_OLD_PIN) {
           
         
           if (MEF_Alarm_VerificarPin(input_value)) {
               uartWriteString(UART_USB, "PIN Actual Correcto. Proceda al nuevo.\r\n");
               
               config->action = KEYPAD_ACTION_SET_NEW_PIN;
               
               lv_textarea_set_text(ta, "");
               
               lv_obj_t * contenedor = lv_obj_get_parent(ta);
               lv_obj_t * label_titulo = lv_obj_get_child(contenedor, 0); // Asumimos que el t�tulo es el primer hijo
               if(label_titulo) lv_label_set_text(label_titulo, "INGRESE NUEVO PIN");
               
               return; 
           } else {
               uartWriteString(UART_USB, "ERROR: PIN Actual Incorrecto.\r\n");
               cambiar_pantalla(config->return_screen);
               return;
           }
       }

       // CASO 2: GUARDAR EL NUEVO PIN (YA PAS� LA VERIFICACI�N)
       else if (config->action == KEYPAD_ACTION_SET_NEW_PIN) {
          MEF_Alarm_SetPIN(input_value);
          sprintf(buffer, "CONFIG: �PIN CAMBIADO EXITOSAMENTE!\r\n");
          // Aqu� s� dejamos que el c�digo siga abajo y cierre la pantalla con cambiar_pantalla()
       }
          
       else if (config->action == KEYPAD_ACTION_ARM_ALARM) {
         MEF_Alarm_ProcessPIN(input_value); // meto el pin en la alarma
         sprintf(buffer, "ALARMA: PIN de control enviado a MEF.\r\n");
      }
      
         else if (config->action == KEYPAD_ACTION_LIGHT_DELAY_TIME) {
        // MMMM minutos -> convertir a milisegundos
        uint32_t duracion_minutos = valor_ingresado;
        uint32_t duracion_ms = duracion_minutos * 60 * 1000;
        
        MEF_Luz_SetTimerDelay(0, duracion_ms);
        MEF_Luz_SetTimerDelay(1, duracion_ms);
        sprintf(buffer, "LUCES: Foco  configurado a TIMER por %lu min.\r\n", duracion_minutos);
        
    } else if (config->action == KEYPAD_ACTION_LIGHT_RTC_ON_TIME) { //necesito 2 entradas (ON Time y OFF Time)
        // HHMM -> Programar ON Time
        uint8_t hora = valor_ingresado / 100;
        uint8_t min = valor_ingresado % 100;

        MEF_Luz_SetRTC_OnTime(0, hora, min); //mando para que se prenda a esta hora
        MEF_Luz_SetRTC_OnTime(1, hora, min); //mando para que se prenda a esta hora

        
        // paso a poner la hora de apagado
        config->action = KEYPAD_ACTION_LIGHT_RTC_OFF_TIME;
        
        // NO CAMBIO LA PANTALLA--Limpiamos el TA y redibujamos el t�tulo
        lv_textarea_set_text(ta, "");
        lv_textarea_set_placeholder_text(ta, "HH:MM OFF");
        
        // Actualizamos el t�tulo para la siguiente entrada
        lv_obj_t * title = lv_obj_get_child(lv_obj_get_parent(ta), 0); // Asume el t�tulo es el primer hijo del contenedor 'cont'
        if (title) lv_label_set_text(title, "CONFIGURAR HORA OFF");
        
        sprintf(buffer, "LUCES: RTC ON %02d:%02d. Esperando OFF Time.\r\n", hora, min);
        
        return; 

    } else if (config->action == KEYPAD_ACTION_LIGHT_RTC_OFF_TIME) {
        // HHMM -> Programar OFF Time
        uint8_t hora = valor_ingresado / 100;
        uint8_t min = valor_ingresado % 100;

        MEF_Luz_SetRTC_OffTime(0, hora, min);
        MEF_Luz_SetMode(0, LUZ_RTC_SCHEDULE, 0); // Activar el modo Schedule
       
        MEF_Luz_SetRTC_OffTime(1, hora, min);
        MEF_Luz_SetMode(1, LUZ_RTC_SCHEDULE, 0); // Activar el modo Schedule
        
        sprintf(buffer, "LUCES: RTC OFF %02d:%02d. PROGRAMACI�N RTC ACTIVADA.\r\n", hora, min);
    }
    
      uartWriteString(UART_USB, buffer); // <--- CORRECCI�N        
      lv_textarea_set_text(ta, "");
      cambiar_pantalla(config->return_screen); // Volver a la pantalla de origen

    } else if (strcmp(txt, "CLR") == 0) {
        lv_textarea_set_text(ta, "");
        uartWriteString(UART_USB, "Keypad: Buffer limpiado (CLR).\r\n");
        
    } else if (strcmp(txt, "CANCEL") == 0) {
       lv_textarea_set_text(ta, "");
       cambiar_pantalla(config->return_screen); // Volver a la pantalla de origen
        uartWriteString(UART_USB, "Keypad: Llamada de retorno completada. Revisando estado...\r\n");
       
    } else if(strlen(txt) == 1 && (txt[0] >= '0' && txt[0] <= '9')) {
       if (len < config->max_length)
          lv_textarea_add_char(ta, txt[0]);
       sprintf(buffer, "Keypad: Digito ingresado: %c\r\n", txt[0]);
       uartWriteString(UART_USB, buffer);
    }
}

void UI_ShowKeypadInput(keypad_config_t * config) {
    lv_obj_t * scr = lv_scr_act();
    lv_obj_clean(scr); 
    
    // 1. CONTENEDOR PRINCIPAL
    lv_obj_t * cont = lv_obj_create(scr);    //el copntenedor general que ocupa casi toda la pantalla
    lv_obj_set_size(cont, LV_PCT(95), LV_PCT(95));
    lv_obj_center(cont);

    // 2. TITULO
    lv_obj_t * title = lv_label_create(cont);   //titulo de arriba de los numeros
    
    // L�gica de T�tulos
    const char* titulo_texto = "INGRESO";
    if (config->action == KEYPAD_ACTION_VERIFY_OLD_PIN) titulo_texto = "INGRESE PIN ACTUAL";
    else if (config->action == KEYPAD_ACTION_LIGHT_DELAY_TIME) titulo_texto = "INGRESE LOS MINUTOS";
    else if (config->action == KEYPAD_ACTION_SET_NEW_PIN) titulo_texto = "INGRESE NUEVO PIN";
    else if (config->is_password) titulo_texto = "INGRESE CODIGO";
    else titulo_texto = "CONFIGURAR VALOR";

    lv_label_set_text(title, titulo_texto);
    
    // CORRECCI�N 1: ajustes para el titulo que quede centrado y arriba de los nums
    lv_obj_set_width(title, LV_PCT(90)); // Ancho fijo para evitar desplazamientos
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0); // Texto centrado
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5); // Pegado arriba

    // 3. �REA DE TEXTO (Input Display)-> ta es donde van los ****
    lv_obj_t * ta = lv_textarea_create(cont);
    lv_obj_set_size(ta, LV_PCT(80), 40); // Un poco m�s chico para que entre seguro
    
    // CORRECCI�N 2: Alinear al contenedor (fijo), donde van los * de la contrase�a
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 45); // 45px desde arriba (debajo del t�tulo)
    
    if (config->is_password) {
        // Si es contrase�a (Alarmas)
        lv_textarea_set_placeholder_text(ta, "****");
    } 
    else if (config->action == KEYPAD_ACTION_LIGHT_DELAY_TIME) {
        // Si es el Timer de luces (Minutos)
        lv_textarea_set_placeholder_text(ta, "MMMM"); 
    } 
    else {
        // Por defecto (RTC ON/OFF u otros)
        lv_textarea_set_placeholder_text(ta, "HH:MM"); 
    }
    
    lv_textarea_set_one_line(ta, true);
    
    if (config->is_password) {
        lv_obj_set_style_text_font(ta, &lv_font_montserrat_14, LV_PART_MAIN); 
    } else {
         lv_obj_set_style_text_font(ta, &lv_font_montserrat_14, LV_PART_MAIN);
    }
    lv_obj_set_style_text_color(ta, lv_color_white(), LV_PART_MAIN);
    
    lv_obj_set_style_border_color(ta, lv_color_make(0x00, 0x80, 0xFF), LV_PART_MAIN); 
    lv_obj_set_style_border_width(ta, 2, LV_PART_MAIN);
    
    lv_textarea_set_password_mode(ta, config->is_password);
    lv_textarea_set_max_length(ta, config->max_length);
    lv_textarea_set_text(ta, ""); 
    
    config->ta_ref = ta;

    // 4. KEYPAD NUM�RICO (Button Matrix)
    static const char * btnm_map[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    "CLR", "0", "\n",               
    "CANCEL", "OK", ""              
   };
    lv_obj_t * btnm = lv_btnmatrix_create(cont);      //Define qu� botones existen btnm
    lv_btnmatrix_set_map(btnm, btnm_map);       
    lv_obj_set_size(btnm, LV_PCT(90), LV_PCT(60)); // Ocupa el 60% de la altura
    
    // CORRECCI�N 3: Alinear al fondo del contenedor (fijo)
    lv_obj_align(btnm, LV_ALIGN_BOTTOM_MID, 0, -5); 
    
    lv_obj_set_style_pad_all(btnm, 3, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btnm, lv_color_make(0x20, 0x20, 0x20), LV_PART_MAIN); 
    lv_obj_set_style_radius(btnm, 10, LV_PART_MAIN);

    lv_obj_set_style_text_font(btnm, LV_FONT_DEFAULT, LV_PART_ITEMS); 
    lv_obj_set_style_bg_color(btnm, lv_color_make(0x40, 0x40, 0x40), LV_PART_ITEMS); 
    lv_obj_set_style_text_color(btnm, lv_color_white(), LV_PART_ITEMS); 
    lv_obj_set_style_radius(btnm, 5, LV_PART_ITEMS); 
    lv_obj_set_style_shadow_width(btnm, 5, LV_PART_ITEMS); 
   
    // Conexi�n del teclado y el �rea de texto
    lv_obj_set_user_data(btnm, config);
   
   // la linea mas importante so las de aca abajo
   //conecta la gr�fica con la l�gica. si alguien toca ok cancel o clr llama a la funci�n keypad_input_event_handler para que procese el n�mero.
    
    
    // Asignar el callback 
    lv_obj_add_event_cb(btnm, keypad_input_event_handler, LV_EVENT_CLICKED, config);
}

// =================================================================
// PANTALLAS DE LA MEF
// =================================================================

// ----------------------------------------------------
// PANTALLA PRINCIPAL (Men�) - 
// ----------------------------------------------------
void UI_ShowPantallaPrincipal(void) {
    lv_obj_t * scr = lv_disp_get_scr_act(NULL);
    
    // 1. DEFINICI�N DE ESTILOS Y FONDO           r     v     a    +alto +claro
    lv_obj_set_style_bg_color(scr, lv_color_make(0x18, 0x18, 0x5A), LV_PART_MAIN); 
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    
    // 2. PANEL DE DATOS EN LA DERECHA (Hora/Fecha Din�mica)
    lv_obj_t * panel_datos = lv_obj_create(scr);
    lv_obj_set_size(panel_datos, 100, 150); 
    lv_obj_align(panel_datos, LV_ALIGN_TOP_RIGHT, -10, 15);

    lv_obj_add_style(panel_datos, &style_decoration_base, 0); 

    lv_obj_set_style_border_width(panel_datos, 3, LV_PART_MAIN); 
    lv_obj_set_style_border_color(panel_datos, lv_color_make(0x00, 0x00, 0xA0), LV_PART_MAIN); 
                                                         
    lv_obj_set_style_bg_color(panel_datos, lv_color_make(0x80, 0x80, 0x80), LV_PART_MAIN); 

    lv_obj_set_style_bg_opa(panel_datos, LV_OPA_COVER, LV_PART_MAIN);
    
    
    // HORA
    label_time_panel = lv_label_create(panel_datos);
    lv_label_set_text(label_time_panel, "--:--:--"); // Texto temporal
    lv_obj_set_style_text_font(label_time_panel, &lv_font_montserrat_14, LV_PART_MAIN); 
    lv_obj_set_style_text_color(label_time_panel, lv_color_make(0x00, 0x00, 0xA0), LV_PART_MAIN);
    lv_obj_align(label_time_panel, LV_ALIGN_TOP_MID, 0, 10);

    // FECHA
    label_date_panel = lv_label_create(panel_datos);
    lv_label_set_text(label_date_panel, "--/--"); // Texto temporal
    lv_obj_set_style_text_font(label_date_panel, &lv_font_montserrat_14, LV_PART_MAIN); 
    lv_obj_set_style_text_color(label_date_panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_date_panel, LV_ALIGN_TOP_MID, 0, 30);
    
    // DIA
    label_day_panel = lv_label_create(panel_datos); 
    lv_label_set_text(label_day_panel, "---"); // Texto temporal
    lv_obj_set_style_text_font(label_day_panel, &lv_font_montserrat_14, LV_PART_MAIN); 
    lv_obj_set_style_text_color(label_day_panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_day_panel, LV_ALIGN_TOP_MID, 0, 50);


    // --- BOTONES DE NAVEGACI�N ---
    
    // 1. Bot�n ALARMA (APLICACI�N DEL GRADIENTE)
    static pantalla_estado_t destino_alarma = PANTALLA_ALARMA;
    lv_obj_t * btn_alarma = lv_btn_create(scr);
    lv_obj_set_size(btn_alarma, 110, 50); 
    
    // Aplicamos el nuevo estilo de gradiente (que ya incluye sombra y borde)
    lv_obj_add_style(btn_alarma, &style_gradient_alarma, 0); // Aplicamos el gradiente
    
    lv_obj_align(btn_alarma, LV_ALIGN_TOP_LEFT, 5, 25); 
    lv_obj_add_event_cb(btn_alarma, callback_cambio_pantalla, LV_EVENT_CLICKED, &destino_alarma);


    lv_obj_set_style_border_width(btn_alarma, 2, LV_PART_MAIN);     // Establece el grosor del borde
    lv_obj_set_style_border_color(btn_alarma, lv_color_white(), LV_PART_MAIN); // Establece el color del borde (Blanco)    
    
    lv_obj_t * label_alarma = lv_label_create(btn_alarma);
    lv_label_set_text(label_alarma,"ALARMA"); 
    lv_obj_add_style(label_alarma, &style_label_text, 0); 
    lv_obj_set_style_text_color(btn_alarma, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(label_alarma);
    
    // 2. Bot�n LUCES 
    static pantalla_estado_t destino_luces = PANTALLA_LUCES;
    lv_obj_t * btn_luces = lv_btn_create(scr);
    lv_obj_set_size(btn_luces, 110, 50);
    
    lv_obj_add_style(btn_luces, &style_gradient_luces, 0); // Aplicamos el gradiente
    
    lv_obj_align_to(btn_luces, btn_alarma, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 40); 
    lv_obj_add_event_cb(btn_luces, callback_cambio_pantalla, LV_EVENT_CLICKED, &destino_luces);
    
    lv_obj_set_style_border_width(btn_luces, 2, LV_PART_MAIN);     // Establece el grosor del borde
    lv_obj_set_style_border_color(btn_luces, lv_color_white(), LV_PART_MAIN); // Establece el color del borde (Blanco)    
    
    lv_obj_t * label_luces = lv_label_create(btn_luces);
    lv_label_set_text(label_luces,"LUCES"); 
    lv_obj_add_style(label_luces, &style_label_text, 0);
    lv_obj_set_style_text_color(label_luces, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(label_luces);

    // 3. Bot�n TEMP y HUMEDAD
    static pantalla_estado_t destino_temp_hum = PANTALLA_TEMP_HUM;
    lv_obj_t * btn_temp_hum = lv_btn_create(scr);
    lv_obj_set_size(btn_temp_hum, 110, 50); 
    
    lv_obj_add_style(btn_temp_hum, &style_gradient_temp, 0); 
    
    lv_obj_align_to(btn_temp_hum, btn_luces, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 45); 
    lv_obj_add_event_cb(btn_temp_hum, callback_cambio_pantalla, LV_EVENT_CLICKED, &destino_temp_hum);
    
    lv_obj_set_style_border_width(btn_temp_hum, 2, LV_PART_MAIN);     // Establece el grosor del borde
    lv_obj_set_style_border_color(btn_temp_hum, lv_color_white(), LV_PART_MAIN); // Establece el color del borde (Blanco)    
    
    
    lv_obj_t * label_temp_hum = lv_label_create(btn_temp_hum);
    lv_label_set_text(label_temp_hum,"T/HUM"); 
    lv_obj_add_style(label_temp_hum, &style_label_text, 0);
    lv_obj_set_style_text_color(label_temp_hum, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(label_temp_hum);

   
   UI_UpdateRTC();
}

void UI_ShowPantallaAlarma(void) {
   lv_obj_t * scr = lv_disp_get_scr_act(NULL);
    static keypad_config_t config_data_armar;
    static keypad_config_t config_data_desarmar;
    static keypad_config_t config_data_cambiar_pin;
   
   alarma_estado_t estado_alarma = get_alarm_state();
   
   char debug_buffer[64];
    sprintf(debug_buffer, "UI Debug: Estado Alarma = %d\r\n", (int)estado_alarma);
    uartWriteString(UART_USB, debug_buffer);
    
    // 1. DEFINICI�N DE ESTILOS Y FONDO           r     v     a    +alto +claro
    lv_obj_set_style_bg_color(scr, lv_color_make(0x18, 0x18, 0x5A), LV_PART_MAIN); 
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    
    // --- Contenedor para botones de Acci�n (Alineado a la Izquierda) ---
    lv_obj_t * btn_container = lv_obj_create(scr);
    lv_obj_set_size(btn_container, 130, 250); 
    lv_obj_set_style_border_width(btn_container, 0, 0); 
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_align(btn_container, LV_ALIGN_TOP_LEFT, 5, 50); 

    // 1. ARMAR (Bot�n de color Verde)
    lv_obj_t * btn_armar = lv_btn_create(btn_container);
    lv_obj_set_size(btn_armar, 120, 50); 
    
    // Aplicamos el nuevo estilo de gradiente (que ya incluye sombra y borde)
    lv_obj_add_style(btn_armar, &style_gradient_armar, 0); // Aplicamos el gradiente
    
    lv_obj_align(btn_armar, LV_ALIGN_TOP_LEFT, 0, 10); 
    
   
     config_data_armar.action = KEYPAD_ACTION_ARM_ALARM;
     config_data_armar.is_password = true; // El pin es una contrase�a
     config_data_armar.min_length = 4;
     config_data_armar.max_length = 4;
     config_data_armar.return_screen = PANTALLA_PRINCIPAL; // Vuelve a esta pantalla
     
     if (estado_alarma == ALM_DESARMADO) {
        // ACTIVO: A�adir el callback normal
     lv_obj_add_event_cb(btn_armar, callback_transicion_armar, LV_EVENT_CLICKED, &config_data_armar);
    } else {
        lv_obj_add_flag(btn_armar, LV_OBJ_FLAG_CLICKABLE); // Deshabilitar click
        // Cambiar el color de fondo para indicar que est� inactivo-> le mando gris oscuro
        lv_obj_set_style_bg_color(btn_armar, lv_color_make(0x50, 0x50, 0x50), LV_PART_MAIN);
    }
     
    
    
    lv_obj_set_style_border_width(btn_armar, 2, LV_PART_MAIN);     // Establece el grosor del borde
    lv_obj_set_style_border_color(btn_armar, lv_color_white(), LV_PART_MAIN); // Establece el color del borde (Blanco)    
    
    lv_obj_t * label_armar = lv_label_create(btn_armar);
    lv_label_set_text(label_armar,"ARMAR"); 
    lv_obj_add_style(label_armar, &style_label_text, 0); 
    lv_obj_set_style_text_color(btn_armar, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(label_armar);


    // 2. DESARMAR (Bot�n de color Rojo)
    lv_obj_t * btn_desarmar = lv_btn_create(btn_container);
    lv_obj_set_size(btn_desarmar, 120, 50); 
    
    // Aplicamos el nuevo estilo de gradiente (que ya incluye sombra y borde)
    lv_obj_add_style(btn_desarmar, &style_gradient_desarmar, 0); // Aplicamos el gradiente
    
    lv_obj_align_to(btn_desarmar, btn_armar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 30); 
   
     config_data_desarmar.action = KEYPAD_ACTION_ARM_ALARM; // Se usar� la misma l�gica de PIN y luego se decidir� si es armar o desarmar.
     config_data_desarmar.is_password = true;
     config_data_desarmar.min_length = 4;
     config_data_desarmar.max_length = 4;
     config_data_desarmar.return_screen = PANTALLA_PRINCIPAL;
     
     if (estado_alarma == ALM_ARMANDO || estado_alarma == ALM_ARMADO || estado_alarma == ALM_IDENTIFICACION || estado_alarma == ALM_ACTIVADO) {
         lv_obj_add_event_cb(btn_desarmar, callback_transicion_desarmar, LV_EVENT_CLICKED, &config_data_desarmar);
    
      } else {
          // INACTIVO (El bot�n NO debe ser clickeable)
          lv_obj_add_flag(btn_desarmar, LV_OBJ_FLAG_CLICKABLE); 
          // 3. Cambiar el color para indicar que est� inactivo (e.g., gris oscuro)
          lv_obj_set_style_bg_color(btn_desarmar, lv_color_make(0x50, 0x50, 0x50), LV_PART_MAIN);
      }
     
    lv_obj_set_style_border_width(btn_desarmar, 2, LV_PART_MAIN);     // Establece el grosor del borde
    lv_obj_set_style_border_color(btn_desarmar, lv_color_white(), LV_PART_MAIN); // Establece el color del borde (Blanco)    
    
    lv_obj_t * label_desarmar = lv_label_create(btn_desarmar);
    lv_label_set_text(label_desarmar,"DESARMAR"); 
    lv_obj_add_style(label_desarmar, &style_label_text, 0); 
    lv_obj_set_style_text_color(btn_desarmar, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(label_desarmar);
    

    // 3. CAMBIAR PIN (NUEVO BOT�N)
  
    lv_obj_t * btn_pin = lv_btn_create(btn_container);
    lv_obj_set_size(btn_pin, 120, 50); 
    
    // Aplicamos el nuevo estilo de gradiente (que ya incluye sombra y borde)
    lv_obj_add_style(btn_pin, &style_gradient_cambiar_pin, 0); // Aplicamos el gradiente
    
    lv_obj_align_to(btn_pin, btn_desarmar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 30);
    
     config_data_cambiar_pin.action = KEYPAD_ACTION_SET_NEW_PIN; // Se usar� la misma l�gica de PIN y luego se decidir� si es armar o desarmar.
     config_data_cambiar_pin.is_password = true;
     config_data_cambiar_pin.min_length = 4;
     config_data_cambiar_pin.max_length = 4;
     config_data_cambiar_pin.return_screen = PANTALLA_PRINCIPAL;
     
     // �USAMOS EL NUEVO CALLBACK DE TRANSICI�N!
     lv_obj_add_event_cb(btn_pin, callback_transicion_cambiar_pin, LV_EVENT_CLICKED, &config_data_cambiar_pin);
    
    
    lv_obj_set_style_border_width(btn_pin, 2, LV_PART_MAIN);     // Establece el grosor del borde
    lv_obj_set_style_border_color(btn_pin, lv_color_white(), LV_PART_MAIN); // Establece el color del borde (Blanco)    
    
    lv_obj_t * label_cambiar_pin = lv_label_create(btn_pin);
    lv_label_set_text(label_cambiar_pin,"CAMBIAR PIN"); 
    lv_obj_add_style(label_cambiar_pin, &style_label_text, 0); 
    lv_obj_set_style_text_color(btn_pin, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(label_cambiar_pin);
    
    // 4. Bot�n CONFIGURACI�N (Tuerca)
   static pantalla_estado_t destino_config = PANTALLA_CONFIG_ALARMA; 
   lv_obj_t * btn_config = lv_btn_create(scr);
   lv_obj_set_size(btn_config, 50, 50); 


   lv_obj_set_style_bg_color(btn_config, lv_color_make(0x55, 0x55, 0x55), LV_PART_MAIN); 
   lv_obj_add_style(btn_config, &style_decoration_base, 0); 
   lv_obj_align(btn_config, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
   lv_obj_add_event_cb(btn_config, callback_cambio_pantalla, LV_EVENT_CLICKED, &destino_config);

   lv_obj_t * label_config = lv_label_create(btn_config);
   lv_label_set_text(label_config, LV_SYMBOL_SETTINGS); 

   lv_obj_set_style_text_color(label_config, lv_color_make(0xFF, 0xEB, 0x3B), LV_PART_MAIN); 

   lv_obj_center(label_config);
    
    // Bot�n VOLVER (Flecha hacia atr�s)
    static pantalla_estado_t destino_principal = PANTALLA_PRINCIPAL;
    lv_obj_t * btn_volver = lv_btn_create(scr);
    lv_obj_set_size(btn_volver, 40, 40);
    
    lv_obj_set_style_bg_color(btn_volver, lv_color_make(0x55, 0x55, 0x55), LV_PART_MAIN); 
    lv_obj_add_style(btn_volver, &style_decoration_base, 0);
    lv_obj_align(btn_volver, LV_ALIGN_TOP_LEFT, 3, 3);
    lv_obj_add_event_cb(btn_volver, callback_cambio_pantalla, LV_EVENT_CLICKED, &destino_principal);
    
    lv_obj_t * label_volver = lv_label_create(btn_volver);
    lv_label_set_text(label_volver, LV_SYMBOL_HOME);
    lv_obj_center(label_volver);
    
    lv_obj_set_style_text_color(label_volver, lv_color_make(0xFF, 0xEB, 0x3B), LV_PART_MAIN); 


    // 2. PANEL DE DATOS EN LA DERECHA (Hora/Fecha Din�mica)
    lv_obj_t * panel_datos = lv_obj_create(scr);
    lv_obj_set_size(panel_datos, 100, 210); 
    lv_obj_align(panel_datos, LV_ALIGN_TOP_RIGHT, -10, 35);

    lv_obj_add_style(panel_datos, &style_decoration_base, 0); 

    lv_obj_set_style_border_width(panel_datos, 3, LV_PART_MAIN); 
    lv_obj_set_style_border_color(panel_datos, lv_color_make(0x00, 0x00, 0xA0), LV_PART_MAIN); 
                                                         
    lv_obj_set_style_bg_color(panel_datos, lv_color_make(0x80, 0x80, 0x80), LV_PART_MAIN); 

    lv_obj_set_style_bg_opa(panel_datos, LV_OPA_COVER, LV_PART_MAIN);
    
    
    // HORA
    label_time_panel = lv_label_create(panel_datos);
    lv_label_set_text(label_time_panel, "HH:MM:SS");
    lv_obj_set_style_text_font(label_time_panel, &lv_font_montserrat_14, LV_PART_MAIN); 
    lv_obj_set_style_text_color(label_time_panel, lv_color_make(0x00, 0x00, 0xA0), LV_PART_MAIN); // Azul p�lido
    lv_obj_align(label_time_panel, LV_ALIGN_TOP_MID, 0, 10);

    // FECHA Y D�A
    label_date_panel = lv_label_create(panel_datos);
    lv_label_set_text(label_date_panel, "DD/MM");
    lv_obj_set_style_text_font(label_date_panel, &lv_font_montserrat_14, LV_PART_MAIN); 
    lv_obj_set_style_text_color(label_date_panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_date_panel, LV_ALIGN_TOP_MID, 0, 30);
    
    label_day_panel = lv_label_create(panel_datos); 
    lv_label_set_text(label_day_panel, "JUEVES");
    lv_obj_set_style_text_font(label_day_panel, &lv_font_montserrat_14, LV_PART_MAIN); 
    lv_obj_set_style_text_color(label_day_panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_day_panel, LV_ALIGN_TOP_MID, 0, 50); 
    
    // ====================================================
   // NUEVO: ESTADO DE ALARMA (Indicador Visual)
   // ====================================================

   // Creamos un bot�n no clickeable (el bot�n es un contenedor f�cil de colorear)
   label_alarm_status_main_panel = lv_btn_create(panel_datos); 
   lv_obj_set_size(label_alarm_status_main_panel, LV_PCT(80), 40);
   lv_obj_add_style(label_alarm_status_main_panel, &style_decoration_base, 0); 
   lv_obj_align(label_alarm_status_main_panel, LV_ALIGN_TOP_MID, 0, 80);

   // IMPORTANTE: Asegurarse de que no se pueda interactuar
   lv_obj_clear_flag(label_alarm_status_main_panel, LV_OBJ_FLAG_CLICKABLE); 
   lv_obj_set_style_pad_all(label_alarm_status_main_panel, 0, 0);

   // Etiqueta interna (mostrar� el texto)
   lv_obj_t * status_label = lv_label_create(label_alarm_status_main_panel);
   
   lv_label_set_text(status_label, "Esperando"); // Texto inicial
   lv_obj_set_style_text_color(status_label, lv_color_white(), LV_PART_MAIN);
   lv_obj_center(status_label);

   // Aplicamos un color inicial (se actualiza inmediatamente despu�s en UI_UpdateAlarmStatus)
   lv_obj_set_style_bg_color(label_alarm_status_main_panel, lv_color_make(0x00, 0xAA, 0x00), LV_PART_MAIN);
   UI_UpdateAlarmStatus();
}


void UI_ShowPantallaLuces(void) {
    lv_obj_t * scr = lv_disp_get_scr_act(NULL);
   
    // Estilos Est�ticos Locales
    static lv_style_t style_decoration_base;
    static lv_style_t style_label_white; 
    static lv_style_t style_foco_cont;
    static lv_style_t style_bot; 

    // Inicializaci�n de estilos (Igual que antes)
    if (style_decoration_base.prop_cnt == 0) { // Peque�a optimizaci�n: init solo una vez
        lv_style_init(&style_decoration_base);
        lv_style_set_radius(&style_decoration_base, 10);
        lv_style_set_shadow_width(&style_decoration_base, 10);
        lv_style_set_shadow_opa(&style_decoration_base, LV_OPA_40);
        
        lv_style_init(&style_label_white);
        lv_style_set_text_font(&style_label_white, &lv_font_montserrat_14);
        lv_style_set_text_color(&style_label_white, lv_color_white());
        
        lv_style_init(&style_foco_cont);
        lv_style_set_bg_color(&style_foco_cont, (lv_color_t)LV_COLOR_MAKE(0x2A, 0x2A, 0x40)); 
        lv_style_set_bg_opa(&style_foco_cont, LV_OPA_COVER); 
        lv_style_set_pad_all(&style_foco_cont, 10);
        lv_style_set_border_width(&style_foco_cont, 2);
        lv_style_set_border_color(&style_foco_cont, (lv_color_t)LV_COLOR_MAKE(0x00, 0xCC, 0xFF)); 
        lv_style_set_radius(&style_foco_cont, 10);

        // Estilo Gradiente
        static const lv_color_t grad_col_bot[2] = {
            LV_COLOR_MAKE(0x50, 0x00, 0x00), 
            LV_COLOR_MAKE(0xFF, 0x33, 0x33)  
        };
        lv_style_init(&style_bot);
        lv_style_set_radius(&style_bot, 10); 
        lv_style_set_shadow_width(&style_bot, 10);
        lv_style_set_shadow_opa(&style_bot, LV_OPA_40);
        lv_style_set_bg_color(&style_bot, grad_col_bot[1]); 
        lv_style_set_bg_grad_color(&style_bot, grad_col_bot[0]); 
        lv_style_set_bg_grad_dir(&style_bot, LV_GRAD_DIR_VER); 
        lv_style_set_bg_opa(&style_bot, LV_OPA_COVER); 
        lv_style_set_border_width(&style_bot, 2);
        lv_style_set_border_color(&style_bot, lv_color_white());
    }

    // --- 0. FONDO ---
    lv_obj_set_style_bg_color(scr, (lv_color_t)LV_COLOR_MAKE(0x18, 0x18, 0x2A), LV_PART_MAIN); 
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    // 1. T�TULO
    lv_obj_t * label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "CONTROL DE LUCES"); 
    lv_obj_add_style(label_title, &style_label_white, 0); 
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_14, 0); 
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);
    
    // 2. Bot�n VOLVER
    static pantalla_estado_t destino_prin = PANTALLA_PRINCIPAL; 
    lv_obj_t * btn_volver = lv_btn_create(scr);
    lv_obj_set_size(btn_volver, 40, 40);
    lv_obj_set_style_bg_color(btn_volver, (lv_color_t)LV_COLOR_MAKE(0x55, 0x55, 0x55), LV_PART_MAIN);
    lv_obj_add_style(btn_volver, &style_decoration_base, 0); 
    lv_obj_align(btn_volver, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_event_cb(btn_volver, callback_cambio_pantalla, LV_EVENT_CLICKED, &destino_prin);
    
    lv_obj_t * label_volver = lv_label_create(btn_volver);
    lv_label_set_text(label_volver, LV_SYMBOL_HOME);
    lv_obj_set_style_text_color(label_volver, (lv_color_t)LV_COLOR_MAKE(0xFF, 0xEB, 0x3B), LV_PART_MAIN);
    lv_obj_center(label_volver);

    // --- CONTENEDOR PRINCIPAL ---
    lv_obj_t * container_focos = lv_obj_create(scr);
    lv_obj_set_size(container_focos, LV_PCT(95), 230); 
    lv_obj_set_style_bg_opa(container_focos, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container_focos, 0, 0);
    lv_obj_set_layout(container_focos, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container_focos, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container_focos, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(container_focos, 5, 0); 
    lv_obj_align(container_focos, LV_ALIGN_CENTER, 0, 30); 
    
    // =======================================================
    // 3. FOCO 1 CARD
    // =======================================================
    lv_obj_t * panel_foco1 = lv_obj_create(container_focos);
    lv_obj_set_size(panel_foco1, 100, 200); // Un poco m�s alto para el texto
    lv_obj_add_style(panel_foco1, &style_foco_cont, 0); 
    lv_obj_add_style(panel_foco1, &style_decoration_base, 0); 
    lv_obj_set_layout(panel_foco1, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(panel_foco1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel_foco1, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); 
    lv_obj_set_style_pad_all(panel_foco1, 5, 0);
    
    // T�tulo Foco 1 
    lv_obj_t * label_name1 = lv_label_create(panel_foco1);
    lv_label_set_text_fmt(label_name1, "%s Foco 1", LV_SYMBOL_BELL); 
    lv_obj_add_style(label_name1, &style_label_white, 0);
    
    // --- NUEVO: LABEL DE FEEDBACK FOCO 1 ---
    lv_obj_t * lbl_state1 = lv_label_create(panel_foco1);
    lv_label_set_text(lbl_state1, "Listo"); // Texto inicial
    lv_obj_set_style_text_font(lbl_state1, &lv_font_montserrat_10, 0); // Texto chico
    lv_obj_set_style_text_color(lbl_state1, lv_color_make(180,180,180), 0); // Gris
    
    // Conectamos el label a las estructuras
    foco1_data.lbl_feedback = lbl_state1;
    f1_b25.lbl_feedback = lbl_state1;
    f1_b50.lbl_feedback = lbl_state1;
    f1_b75.lbl_feedback = lbl_state1;
    // ----------------------------------------
    
    // Toggle ON/OFF Foco 1
    lv_obj_t * btn_on_off1 = lv_btn_create(panel_foco1);
    lv_obj_set_size(btn_on_off1, 90, 35);
    lv_obj_add_flag(btn_on_off1, LV_OBJ_FLAG_CHECKABLE); 
    
    if (MEF_Luz_GetEstado(0) != LUZ_OFF) {
        lv_obj_add_state(btn_on_off1, LV_STATE_CHECKED);
        // Si arranca prendido, actualizamos el texto inicial
        lv_label_set_text(lbl_state1, "Estado: PRENDIDO");
        lv_obj_set_style_text_color(lbl_state1, lv_color_make(50, 255, 50), 0);
    }
    
    lv_obj_add_style(btn_on_off1, &style_decoration_base, 0);
    lv_obj_set_style_bg_color(btn_on_off1, (lv_color_t)LV_COLOR_MAKE(0xCC, 0x00, 0x00), LV_PART_MAIN); 
    lv_obj_set_style_bg_color(btn_on_off1, (lv_color_t)LV_COLOR_MAKE(0x00, 0xAA, 0x00), LV_STATE_CHECKED); 
    lv_obj_set_style_text_color(btn_on_off1, lv_color_white(), LV_PART_MAIN);
    lv_obj_add_event_cb(btn_on_off1, callback_foco_on_off, LV_EVENT_VALUE_CHANGED, &foco1_data); 
    
    lv_label_set_text(lv_label_create(btn_on_off1), "ON / OFF");
    lv_obj_center(lv_obj_get_child(btn_on_off1, 0));
    
    // Etiqueta Brillo
    lv_obj_t * label_brillo1 = lv_label_create(panel_foco1);
    lv_label_set_text(label_brillo1, "Brillo:");
    lv_obj_add_style(label_brillo1, &style_label_white, 0);
    
    // Botones Brillo Foco 1
    lv_obj_t * btn_brillo_cont1 = lv_obj_create(panel_foco1);
    lv_obj_set_size(btn_brillo_cont1, LV_PCT(100), 30);
    lv_obj_set_style_bg_opa(btn_brillo_cont1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_brillo_cont1, 0, 0);
    lv_obj_set_layout(btn_brillo_cont1, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_brillo_cont1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_brillo_cont1, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(btn_brillo_cont1, 0, 0);

    // Botones Manuales (Mapeo directo a globales)
    lv_obj_t * b1_1 = lv_btn_create(btn_brillo_cont1);
    lv_obj_set_size(b1_1, 28, 28);
    lv_obj_add_style(b1_1, &style_bot, 0);
    lv_label_set_text(lv_label_create(b1_1), "25");
    lv_obj_center(lv_obj_get_child(b1_1, 0));
    lv_obj_add_event_cb(b1_1, callback_foco_brillo_fijo, LV_EVENT_CLICKED, &f1_b25);

    lv_obj_t * b1_2 = lv_btn_create(btn_brillo_cont1);
    lv_obj_set_size(b1_2, 28, 28);
    lv_obj_add_style(b1_2, &style_bot, 0);
    lv_label_set_text(lv_label_create(b1_2), "50");
    lv_obj_center(lv_obj_get_child(b1_2, 0));
    lv_obj_add_event_cb(b1_2, callback_foco_brillo_fijo, LV_EVENT_CLICKED, &f1_b50);

    lv_obj_t * b1_3 = lv_btn_create(btn_brillo_cont1);
    lv_obj_set_size(b1_3, 28, 28);
    lv_obj_add_style(b1_3, &style_bot, 0);
    lv_label_set_text(lv_label_create(b1_3), "75");
    lv_obj_center(lv_obj_get_child(b1_3, 0));
    lv_obj_add_event_cb(b1_3, callback_foco_brillo_fijo, LV_EVENT_CLICKED, &f1_b75);
    
    // =======================================================
    // 4. FOCO 2 CARD
    // =======================================================
    lv_obj_t * panel_foco2 = lv_obj_create(container_focos);
    lv_obj_set_size(panel_foco2, 100, 200);
    lv_obj_add_style(panel_foco2, &style_foco_cont, 0); 
    lv_obj_add_style(panel_foco2, &style_decoration_base, 0); 
    lv_obj_set_layout(panel_foco2, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(panel_foco2, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel_foco2, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); 
    lv_obj_set_style_pad_all(panel_foco2, 5, 0);
    
    lv_obj_t * label_name2 = lv_label_create(panel_foco2);
    lv_label_set_text_fmt(label_name2, "%s Foco 2", LV_SYMBOL_BELL); 
    lv_obj_add_style(label_name2, &style_label_white, 0);
    
    // --- NUEVO: LABEL DE FEEDBACK FOCO 2 ---
    lv_obj_t * lbl_state2 = lv_label_create(panel_foco2);
    lv_label_set_text(lbl_state2, "Listo");
    lv_obj_set_style_text_font(lbl_state2, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_state2, lv_color_make(180,180,180), 0);
    
    // Conectamos estructuras Foco 2
    foco2_data.lbl_feedback = lbl_state2;
    f2_b25.lbl_feedback = lbl_state2;
    f2_b50.lbl_feedback = lbl_state2;
    f2_b75.lbl_feedback = lbl_state2;
    // -------------------------------------

    // Toggle ON/OFF Foco 2
    lv_obj_t * btn_on_off2 = lv_btn_create(panel_foco2);
    lv_obj_set_size(btn_on_off2, 90, 35);
    lv_obj_add_flag(btn_on_off2, LV_OBJ_FLAG_CHECKABLE);
    
    if (MEF_Luz_GetEstado(1) != LUZ_OFF) {
        lv_obj_add_state(btn_on_off2, LV_STATE_CHECKED);
        lv_label_set_text(lbl_state2, "Estado: PRENDIDO");
        lv_obj_set_style_text_color(lbl_state2, lv_color_make(50, 255, 50), 0);
    }

    lv_obj_add_style(btn_on_off2, &style_decoration_base, 0);
    lv_obj_set_style_bg_color(btn_on_off2, (lv_color_t)LV_COLOR_MAKE(0xCC, 0x00, 0x00), LV_PART_MAIN); 
    lv_obj_set_style_bg_color(btn_on_off2, (lv_color_t)LV_COLOR_MAKE(0x00, 0xAA, 0x00), LV_STATE_CHECKED); 
    lv_obj_set_style_text_color(btn_on_off2, lv_color_white(), LV_PART_MAIN);
    lv_obj_add_event_cb(btn_on_off2, callback_foco_on_off, LV_EVENT_VALUE_CHANGED, &foco2_data); 
    
    lv_label_set_text(lv_label_create(btn_on_off2), "ON / OFF");
    lv_obj_center(lv_obj_get_child(btn_on_off2, 0));
    
    lv_obj_t * label_brillo2 = lv_label_create(panel_foco2);
    lv_label_set_text(label_brillo2, "Brillo");
    lv_obj_add_style(label_brillo2, &style_label_white, 0);
    
    // Botones Brillo Foco 2
    lv_obj_t * btn_brillo_cont2 = lv_obj_create(panel_foco2);
    lv_obj_set_size(btn_brillo_cont2, LV_PCT(100), 30);
    lv_obj_set_style_bg_opa(btn_brillo_cont2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_brillo_cont2, 0, 0);
    lv_obj_set_layout(btn_brillo_cont2, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_brillo_cont2, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_brillo_cont2, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(btn_brillo_cont2, 0, 0);

    // Botones manuales Foco 2
    lv_obj_t * b2_1 = lv_btn_create(btn_brillo_cont2);
    lv_obj_set_size(b2_1, 28, 28);
    lv_obj_add_style(b2_1, &style_bot, 0);
    lv_label_set_text(lv_label_create(b2_1), "25");
    lv_obj_center(lv_obj_get_child(b2_1, 0));
    lv_obj_add_event_cb(b2_1, callback_foco_brillo_fijo, LV_EVENT_CLICKED, &f2_b25);

    lv_obj_t * b2_2 = lv_btn_create(btn_brillo_cont2);
    lv_obj_set_size(b2_2, 28, 28);
    lv_obj_add_style(b2_2, &style_bot, 0);
    lv_label_set_text(lv_label_create(b2_2), "50");
    lv_obj_center(lv_obj_get_child(b2_2, 0));
    lv_obj_add_event_cb(b2_2, callback_foco_brillo_fijo, LV_EVENT_CLICKED, &f2_b50);

    lv_obj_t * b2_3 = lv_btn_create(btn_brillo_cont2);
    lv_obj_set_size(b2_3, 28, 28);
    lv_obj_add_style(b2_3, &style_bot, 0);
    lv_label_set_text(lv_label_create(b2_3), "75");
    lv_obj_center(lv_obj_get_child(b2_3, 0));
    lv_obj_add_event_cb(b2_3, callback_foco_brillo_fijo, LV_EVENT_CLICKED, &f2_b75);

    // 5. Bot�n CONFIGURACI�N
    static pantalla_estado_t destino_config_luc = PANTALLA_CONFIG_LUCES; 
    lv_obj_t * btn_config = lv_btn_create(scr);
    lv_obj_set_size(btn_config, 50, 50);
    lv_obj_set_style_bg_color(btn_config, (lv_color_t)LV_COLOR_MAKE(0x55, 0x55, 0x55), LV_PART_MAIN);
    lv_obj_add_style(btn_config, &style_decoration_base, 0); 
    lv_obj_align(btn_config, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_event_cb(btn_config, callback_cambio_pantalla, LV_EVENT_CLICKED, &destino_config_luc);

    lv_obj_t * label_config = lv_label_create(btn_config);
    lv_label_set_text(label_config, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(label_config, (lv_color_t)LV_COLOR_MAKE(0xFF, 0xEB, 0x3B), LV_PART_MAIN);
    lv_obj_center(label_config);
}


void UI_ShowPantallaTempHum(void) {
    lv_obj_t * scr = lv_disp_get_scr_act(NULL);
   
    lv_obj_clean(scr); 
    lv_obj_remove_style_all(scr); 
   
     
   

    // =======================================================
    // 0. DECLARACIONES Y ESTILOS
    // =======================================================
    lv_obj_set_style_bg_color(scr, lv_color_make(0x18, 0x18, 0x5A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    // Estilos Base
    static lv_style_t style_decoration_base;
    static lv_style_t style_label_text;
    static lv_style_t style_label_white;
    
    // Inicializar solo si no est�n inicializados (o reiniciar siempre, depende de tu gesti�n de memoria)
    // Asumimos inicializaci�n local simple para mantener tu estructura
    lv_style_init(&style_decoration_base);
    lv_style_set_radius(&style_decoration_base, 10);
    lv_style_set_shadow_width(&style_decoration_base, 10);
    lv_style_set_shadow_opa(&style_decoration_base, LV_OPA_40);

    lv_style_init(&style_label_text);
    lv_style_set_text_font(&style_label_text, &lv_font_montserrat_14);
    lv_style_set_text_color(&style_label_text, lv_color_white());

    lv_style_init(&style_label_white);
    lv_style_set_text_font(&style_label_white, &lv_font_montserrat_14);
    lv_style_set_text_color(&style_label_white, lv_color_white());

    // 1. T�TULO
    lv_obj_t * label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "CONTROL CALOVENTOR");
    lv_obj_add_style(label_title, &style_label_white, 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);

    // --- Contenedor de Botones ---
    lv_obj_t * btn_container = lv_obj_create(scr);
    lv_obj_set_size(btn_container, 130, 250);
    lv_obj_set_style_border_width(btn_container, 0, 0);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_align(btn_container, LV_ALIGN_TOP_LEFT, 5, 50);

    // --- OBTENER ESTADO ACTUAL PARA SINCRONIZACI�N ---
    caloventor_estado_t estado_actual = MEF_Caloventor_GetEstado();
    
    // GRADIENTE VERTICAL PARA CALOR
    static lv_style_t style_gradient_calor;
    lv_style_init(&style_gradient_calor);

    // Colores del gradiente: Rojo oscuro a Rojo vivo (para el bot�n ARMAR)
    static const lv_color_t grad_colors_calor[2] = {
        LV_COLOR_MAKE(0x00, 0x50, 0x00), // Rojo MUY Oscuro (Base/Abajo)
        LV_COLOR_MAKE(0x33, 0xFF, 0x33)  // Rojo Claro (Resaltado/Arriba)
    };
    
    // 1. Aplicamos las propiedades de decoraci�n base
    lv_style_set_radius(&style_gradient_calor, 10); 
    lv_style_set_shadow_width(&style_gradient_calor, 10);
    lv_style_set_shadow_opa(&style_gradient_calor, LV_OPA_40);
    
    lv_style_set_bg_color(&style_gradient_calor, grad_colors_calor[1]);        // Color superior (Base si falla gradiente)
    lv_style_set_bg_grad_color(&style_gradient_calor, grad_colors_calor[0]);   // Color inferior
    lv_style_set_bg_grad_dir(&style_gradient_calor, LV_GRAD_DIR_VER);           // Direcci�n vertical
    lv_style_set_bg_opa(&style_gradient_calor, LV_OPA_COVER);                   // Opacidad completa
    
    // 3. Aplicamos las propiedades del borde (SIN el par�metro 'part' en style_set_*)
    lv_style_set_border_width(&style_gradient_calor, 2);
    lv_style_set_border_color(&style_gradient_calor, lv_color_white());
    
    // GRADIENTE VERTICAL PARA FRIO
    static lv_style_t style_gradient_frio;
    lv_style_init(&style_gradient_frio);

    // Colores del gradiente: Rojo oscuro a Rojo vivo (para el bot�n ALARMA)
    static const lv_color_t grad_colors_frio[2] = {
        LV_COLOR_MAKE(0x50, 0x00, 0x00), // Verde MUY Oscuro (Base/Abajo)  
        LV_COLOR_MAKE(0xFF, 0x33, 0x33)  // Verde Claro (Resaltado/Arriba) 
    };
    
    // 1. Aplicamos las propiedades de decoraci�n base
    lv_style_set_radius(&style_gradient_frio, 10); 
    lv_style_set_shadow_width(&style_gradient_frio, 10);
    lv_style_set_shadow_opa(&style_gradient_frio, LV_OPA_40);
    
    lv_style_set_bg_color(&style_gradient_frio, grad_colors_frio[1]);        // Color superior (Base si falla gradiente)
    lv_style_set_bg_grad_color(&style_gradient_frio, grad_colors_frio[0]);   // Color inferior
    lv_style_set_bg_grad_dir(&style_gradient_frio, LV_GRAD_DIR_VER);           // Direcci�n vertical
    lv_style_set_bg_opa(&style_gradient_frio, LV_OPA_COVER);                   // Opacidad completa
    
    // 3. Aplicamos las propiedades del borde (SIN el par�metro 'part' en style_set_*)
    lv_style_set_border_width(&style_gradient_frio, 2);
    lv_style_set_border_color(&style_gradient_frio, lv_color_white());

    // =======================================================
    // 1. BOT�N CALOR (TOGGLE MANUAL)
    // =======================================================
    
    lv_obj_t * btn_calor = lv_btn_create(btn_container);
    ptr_btn_calor = btn_calor; 

    lv_obj_t * btn_frio = lv_btn_create(btn_container);
    ptr_btn_vent = btn_frio; 
    
    
    btn_calor = lv_btn_create(btn_container);
    lv_obj_set_size(btn_calor, 100, 75);
    lv_obj_add_style(btn_calor, &style_gradient_calor, 0);
    lv_obj_align(btn_calor, LV_ALIGN_TOP_LEFT, 0, 10);
    lv_obj_add_flag(btn_calor, LV_OBJ_FLAG_CHECKABLE); // Permitir estado presionado
    


    // **SINCRONIZACI�N**: Si est� calentando (manual o auto), mostrar apretado
    if (estado_actual == CALO_MANUAL_CALOR || estado_actual == CALO_AUTO_CALENTANDO) {
        lv_obj_add_state(btn_calor, LV_STATE_CHECKED);
    }

    // **CALLBACK NUEVO**
    lv_obj_add_event_cb(btn_calor, callback_manual_calor_toggle, LV_EVENT_VALUE_CHANGED, &mi_caloventor);

    lv_obj_t * label_calor = lv_label_create(btn_calor);
    lv_label_set_text(label_calor, "CALOR\nON/OFF");
    lv_obj_add_style(label_calor, &style_label_text, 0);
    lv_obj_center(label_calor);

    // =======================================================
    // 2. BOT�N VENTILADOR (TOGGLE MANUAL)
    // =======================================================
    btn_frio = lv_btn_create(btn_container);
    lv_obj_set_size(btn_frio, 100, 75);
    lv_obj_add_style(btn_frio, &style_gradient_frio, 0);
    lv_obj_align_to(btn_frio, btn_frio, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 30);
    lv_obj_add_flag(btn_frio, LV_OBJ_FLAG_CHECKABLE); // Permitir estado presionado

    // **SINCRONIZACI�N**: Si est� ventilando (manual o auto), mostrar apretado
    if (estado_actual == CALO_MANUAL_VENTILADOR || estado_actual == CALO_AUTO_ENFRIANDO) {
        lv_obj_add_state(btn_frio, LV_STATE_CHECKED);
    }

    // **CALLBACK NUEVO**
    lv_obj_add_event_cb(btn_frio, callback_manual_ventilador_toggle, LV_EVENT_VALUE_CHANGED, &mi_ventilador);

    lv_obj_t * label_frio = lv_label_create(btn_frio);
    lv_label_set_text(label_frio, "VENT\nON/OFF");
    lv_obj_add_style(label_frio, &style_label_text, 0);
    lv_obj_center(label_frio);

    
    
    // --- PANEL DE DATOS (DERECHA) ---
    lv_obj_t * panel_datos = lv_obj_create(scr);
    lv_obj_set_size(panel_datos, 120, 220);
    lv_obj_align(panel_datos, LV_ALIGN_TOP_RIGHT, -10, 35);
    lv_obj_add_style(panel_datos, &style_decoration_base, 0);
    lv_obj_set_style_border_width(panel_datos, 3, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel_datos, lv_color_make(0x00, 0x00, 0xA0), LV_PART_MAIN);
    lv_obj_set_style_bg_color(panel_datos, lv_color_make(0x80, 0x80, 0x80), LV_PART_MAIN);

    // Datos Hora/Fecha (Igual que antes)
    label_time_panel = lv_label_create(panel_datos);
    lv_label_set_text(label_time_panel, "HH:MM:SS");
    lv_obj_set_style_text_font(label_time_panel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_time_panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_time_panel, LV_ALIGN_TOP_MID, 0, 10);

    label_date_panel = lv_label_create(panel_datos);
    lv_label_set_text(label_date_panel, "DD/MM");
    lv_obj_set_style_text_font(label_date_panel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_date_panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_date_panel, LV_ALIGN_TOP_MID, 0, 30);

    label_day_panel = lv_label_create(panel_datos);
    lv_label_set_text(label_day_panel, "DIA");
    lv_obj_set_style_text_font(label_day_panel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(label_day_panel, LV_ALIGN_TOP_MID, 0, 50);

    // TEMPERATURA
    label_temp_txt = lv_label_create(panel_datos);
    lv_label_set_text(label_temp_txt, "Temp:");
    lv_obj_set_style_text_font(label_temp_txt, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_temp_txt, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_temp_txt, LV_ALIGN_TOP_MID, 0, 80);

    label_temp_value = lv_label_create(panel_datos);
    lv_label_set_text_fmt(label_temp_value, "%.1f C", get_current_temperature());
    lv_obj_align(label_temp_value, LV_ALIGN_TOP_MID, 0, 95);
    
        lv_obj_set_style_text_color(label_temp_value, lv_color_white(), LV_PART_MAIN);


    // HUMEDAD
    label_hum_txt = lv_label_create(panel_datos);
    lv_label_set_text(label_hum_txt, "Hum:");
    lv_obj_set_style_text_font(label_hum_txt, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_hum_txt, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_hum_txt, LV_ALIGN_TOP_MID, 0, 115);

    label_hum_value = lv_label_create(panel_datos);
    lv_label_set_text_fmt(label_hum_value, "%.1f %%", get_current_humidity());
    lv_obj_align(label_hum_value, LV_ALIGN_TOP_MID, 0, 130);
    
            lv_obj_set_style_text_color(label_hum_value, lv_color_white(), LV_PART_MAIN);

    
    //feedback caloventor
    lv_obj_t * lbl_estado_calor = lv_label_create(panel_datos);
    lv_label_set_text(lbl_estado_calor, "Esperando");
    lv_obj_set_style_text_font(lbl_estado_calor, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_estado_calor, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(lbl_estado_calor, LV_ALIGN_TOP_MID, 0, 170);
    
    mi_caloventor.label_estado = lbl_estado_calor;
    
    //feedback ventilador
    
    lv_obj_t * lbl_estado_frio = lv_label_create(panel_datos);
    lv_obj_set_style_text_font(lbl_estado_frio, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_label_set_text(lbl_estado_frio, "Esperando");
    lv_obj_set_style_text_color(lbl_estado_frio, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(lbl_estado_frio, LV_ALIGN_TOP_MID, 0, 190);
    
    mi_ventilador.label_estado = lbl_estado_frio;
    

    // 4. Bot�n CONFIGURACI�N (Tuerca)
   static pantalla_estado_t destino_config = PANTALLA_CONFIG_TEMP_HUM; 
   lv_obj_t * btn_config = lv_btn_create(scr);
   lv_obj_set_size(btn_config, 50, 50); 


   lv_obj_set_style_bg_color(btn_config, lv_color_make(0x55, 0x55, 0x55), LV_PART_MAIN); 
   lv_obj_add_style(btn_config, &style_decoration_base, 0); 
   lv_obj_align(btn_config, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
   lv_obj_add_event_cb(btn_config, callback_cambio_pantalla, LV_EVENT_CLICKED, &destino_config);

   lv_obj_t * label_config = lv_label_create(btn_config);
   lv_label_set_text(label_config, LV_SYMBOL_SETTINGS); 

   lv_obj_set_style_text_color(label_config, lv_color_make(0xFF, 0xEB, 0x3B), LV_PART_MAIN); 

   lv_obj_center(label_config);
    
    // Bot�n VOLVER (Flecha hacia atr�s)
    static pantalla_estado_t destino_principal = PANTALLA_PRINCIPAL;
    lv_obj_t * btn_volver = lv_btn_create(scr);
    lv_obj_set_size(btn_volver, 40, 40);
    
    lv_obj_set_style_bg_color(btn_volver, lv_color_make(0x55, 0x55, 0x55), LV_PART_MAIN); 
    lv_obj_add_style(btn_volver, &style_decoration_base, 0);
    lv_obj_align(btn_volver, LV_ALIGN_TOP_LEFT, 3, 3);
    lv_obj_add_event_cb(btn_volver, callback_cambio_pantalla, LV_EVENT_CLICKED, &destino_principal);
    
    lv_obj_t * label_volver = lv_label_create(btn_volver);
    lv_label_set_text(label_volver, LV_SYMBOL_HOME);
    lv_obj_center(label_volver);
    
    lv_obj_set_style_text_color(label_volver, lv_color_make(0xFF, 0xEB, 0x3B), LV_PART_MAIN); 

    UI_UpdateTempHum();
}


void UI_ShowPantallaConfigAlarma(void) {
    lv_obj_t * scr = lv_disp_get_scr_act(NULL);
    
    // 1. DEFINICI�N DE ESTILOS Y FONDO           r     v     a    +alto +claro
    lv_obj_set_style_bg_color(scr, lv_color_make(0x18, 0x18, 0x5A), LV_PART_MAIN); 
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    
    // Estilo Base de Decoraci�n (Sombra y Radio)
    lv_style_t style_decoration_base;
    lv_style_init(&style_decoration_base);
    lv_style_set_radius(&style_decoration_base, 10); 
    lv_style_set_shadow_width(&style_decoration_base, 10); 
    lv_style_set_shadow_opa(&style_decoration_base, LV_OPA_40); 
    
    // Estilo Base de Fuente (Usamos montserrat_14 y texto blanco)
    lv_style_t style_label_text;
    lv_style_init(&style_label_text);
    lv_style_set_text_font(&style_label_text, &lv_font_montserrat_14); 
    lv_style_set_text_color(&style_label_text, lv_color_white()); 
   
   // 1. TITULO 
   
   // Estilo Base de Fuente (Todo Texto Blanco/Claro)
    lv_style_t style_label_white;
    lv_style_init(&style_label_white);
    lv_style_set_text_font(&style_label_white, &lv_font_montserrat_14);
    lv_style_set_text_color(&style_label_white, lv_color_white());
    
    lv_obj_t * label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "CONFIG DE ALARMA");
    
    lv_obj_add_style(label_title, &style_label_white, 0);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_14, 0); 
    lv_obj_set_style_text_color(label_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);
   

    // GRADIENTE VERTICAL PARA ARMAR
    static lv_style_t style_gradient_ambos_sensores;
    lv_style_init(&style_gradient_ambos_sensores);

    // Colores del gradiente: Rojo oscuro a Rojo vivo (para el bot�n ARMAR)
    static const lv_color_t grad_colors_ambos_sensores[2] = {
        LV_COLOR_MAKE(0x00, 0x50, 0x00), // Rojo MUY Oscuro (Base/Abajo)
        LV_COLOR_MAKE(0x33, 0xFF, 0x33)  // Rojo Claro (Resaltado/Arriba)
    };
    
    // 1. Aplicamos las propiedades de decoraci�n base
    lv_style_set_radius(&style_gradient_ambos_sensores, 10); 
    lv_style_set_shadow_width(&style_gradient_ambos_sensores, 10);
    lv_style_set_shadow_opa(&style_gradient_ambos_sensores, LV_OPA_40);
    
    lv_style_set_bg_color(&style_gradient_ambos_sensores, grad_colors_ambos_sensores[1]);        // Color superior (Base si falla gradiente)
    lv_style_set_bg_grad_color(&style_gradient_ambos_sensores, grad_colors_ambos_sensores[0]);   // Color inferior
    lv_style_set_bg_grad_dir(&style_gradient_ambos_sensores, LV_GRAD_DIR_VER);           // Direcci�n vertical
    lv_style_set_bg_opa(&style_gradient_ambos_sensores, LV_OPA_COVER);                   // Opacidad completa
    
    // 3. Aplicamos las propiedades del borde (SIN el par�metro 'part' en style_set_*)
    lv_style_set_border_width(&style_gradient_ambos_sensores, 2);
    lv_style_set_border_color(&style_gradient_ambos_sensores, lv_color_white());
    
    // GRADIENTE VERTICAL PARA DESARMAR
    static lv_style_t style_gradient_movimiento;
    lv_style_init(&style_gradient_movimiento);

    // Colores del gradiente: Rojo oscuro a Rojo vivo (para el bot�n ALARMA)
    static const lv_color_t grad_colors_movimiento[2] = {
        LV_COLOR_MAKE(0x50, 0x00, 0x00), // Verde MUY Oscuro (Base/Abajo)  
        LV_COLOR_MAKE(0xFF, 0x33, 0x33)  // Verde Claro (Resaltado/Arriba) 
    };
    
    // 1. Aplicamos las propiedades de decoraci�n base
    lv_style_set_radius(&style_gradient_movimiento, 10); 
    lv_style_set_shadow_width(&style_gradient_movimiento, 10);
    lv_style_set_shadow_opa(&style_gradient_movimiento, LV_OPA_40);
    
    lv_style_set_bg_color(&style_gradient_movimiento, grad_colors_movimiento[1]);        // Color superior (Base si falla gradiente)
    lv_style_set_bg_grad_color(&style_gradient_movimiento, grad_colors_movimiento[0]);   // Color inferior
    lv_style_set_bg_grad_dir(&style_gradient_movimiento, LV_GRAD_DIR_VER);           // Direcci�n vertical
    lv_style_set_bg_opa(&style_gradient_movimiento, LV_OPA_COVER);                   // Opacidad completa
    
    // 3. Aplicamos las propiedades del borde (SIN el par�metro 'part' en style_set_*)
    lv_style_set_border_width(&style_gradient_movimiento, 2);
    lv_style_set_border_color(&style_gradient_movimiento, lv_color_white());
    
    // GRADIENTE VERTICAL PARA CAMBIAR_PIN
    static lv_style_t style_gradient_magnetico;
    lv_style_init(&style_gradient_magnetico);

    // Colores del gradiente: Rojo oscuro a Rojo vivo (para el bot�n ALARMA)
    static const lv_color_t grad_colors_magnetico[2] = {
        LV_COLOR_MAKE(0x10, 0x10, 0x10), // Gris MUY Oscuro (Base/Abajo)
        LV_COLOR_MAKE(0xA0, 0xA0, 0xA0)  // Gris Claro (Resaltado/Arriba)
    };
    
    // 1. Aplicamos las propiedades de decoraci�n base
    lv_style_set_radius(&style_gradient_magnetico, 10); 
    lv_style_set_shadow_width(&style_gradient_magnetico, 10);
    lv_style_set_shadow_opa(&style_gradient_magnetico, LV_OPA_40);
    
    lv_style_set_bg_color(&style_gradient_magnetico, grad_colors_magnetico[1]);        // Color superior (Base si falla gradiente)
    lv_style_set_bg_grad_color(&style_gradient_magnetico, grad_colors_magnetico[0]);   // Color inferior
    lv_style_set_bg_grad_dir(&style_gradient_magnetico, LV_GRAD_DIR_VER);           // Direcci�n vertical
    lv_style_set_bg_opa(&style_gradient_magnetico, LV_OPA_COVER);                   // Opacidad completa
    
    // 3. Aplicamos las propiedades del borde (SIN el par�metro 'part' en style_set_*)
    lv_style_set_border_width(&style_gradient_magnetico, 2);
    lv_style_set_border_color(&style_gradient_magnetico, lv_color_white());
    
    // --- Contenedor para botones de Acci�n (Alineado a la Izquierda) ---
    lv_obj_t * btn_container = lv_obj_create(scr);
    lv_obj_set_size(btn_container, 130, 250); 
    lv_obj_set_style_border_width(btn_container, 0, 0); 
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_align(btn_container, LV_ALIGN_TOP_LEFT, 5, 50); 

    // 1. AMBOS SENSORES (Bot�n de color Verde)
    lv_obj_t * btn_ambos_sensores = lv_btn_create(btn_container);
    lv_obj_set_size(btn_ambos_sensores, 120, 50); 
    
    // Aplicamos el nuevo estilo de gradiente (que ya incluye sombra y borde)
    lv_obj_add_style(btn_ambos_sensores, &style_gradient_ambos_sensores, 0); // Aplicamos el gradiente
    
    lv_obj_align(btn_ambos_sensores, LV_ALIGN_TOP_LEFT, 0, 10); 
    lv_obj_add_event_cb(btn_ambos_sensores, callback_config_alarma_sensores, LV_EVENT_CLICKED, &config_ambos_data);    
    lv_obj_set_style_border_width(btn_ambos_sensores, 2, LV_PART_MAIN);     // Establece el grosor del borde
    lv_obj_set_style_border_color(btn_ambos_sensores, lv_color_white(), LV_PART_MAIN); // Establece el color del borde (Blanco)    
    
    lv_obj_t * label_ambos_sensores = lv_label_create(btn_ambos_sensores);
    lv_label_set_text(label_ambos_sensores,"AMBOS\nSENSORES"); 
    lv_obj_add_style(label_ambos_sensores, &style_label_text, 0); 
    lv_obj_set_style_text_color(btn_ambos_sensores, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(label_ambos_sensores);


    // 2. MOVIMIENTO (Bot�n de color Rojo)
    lv_obj_t * btn_movimiento = lv_btn_create(btn_container);
    lv_obj_set_size(btn_movimiento, 120, 50); 
    
    // Aplicamos el nuevo estilo de gradiente (que ya incluye sombra y borde)
    lv_obj_add_style(btn_movimiento, &style_gradient_movimiento, 0); // Aplicamos el gradiente
    
    lv_obj_align_to(btn_movimiento, btn_ambos_sensores, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 30);    lv_obj_add_event_cb(btn_movimiento, callback_config_alarma_sensores, LV_EVENT_CLICKED, &config_movimiento_data);    
    lv_obj_set_style_border_width(btn_movimiento, 2, LV_PART_MAIN);     // Establece el grosor del borde
    lv_obj_set_style_border_color(btn_movimiento, lv_color_white(), LV_PART_MAIN); // Establece el color del borde (Blanco)    
    
    lv_obj_t * label_movimiento = lv_label_create(btn_movimiento);
    lv_label_set_text(label_movimiento,"MOVIMIENTO"); 
    lv_obj_add_style(label_movimiento, &style_label_text, 0); 
    lv_obj_set_style_text_color(btn_movimiento, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(label_movimiento);
    

    // 3. MAGNETICO (NUEVO BOT�N)
  
    lv_obj_t * btn_magnetico = lv_btn_create(btn_container);
    lv_obj_set_size(btn_magnetico, 120, 50); 
    
    // Aplicamos el nuevo estilo de gradiente (que ya incluye sombra y borde)
    lv_obj_add_style(btn_magnetico, &style_gradient_magnetico, 0); // Aplicamos el gradiente
    
    lv_obj_align_to(btn_magnetico, btn_movimiento, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 30);
    lv_obj_add_event_cb(btn_magnetico, callback_config_alarma_sensores, LV_EVENT_CLICKED, &config_magnetico_data);    
    lv_obj_set_style_border_width(btn_magnetico, 2, LV_PART_MAIN);     // Establece el grosor del borde
    lv_obj_set_style_border_color(btn_magnetico, lv_color_white(), LV_PART_MAIN); // Establece el color del borde (Blanco)    
    
    lv_obj_t * label_magnetico= lv_label_create(btn_magnetico);
    lv_label_set_text(label_magnetico,"MAGNETICO"); 
    lv_obj_add_style(label_magnetico, &style_label_text, 0); 
    lv_obj_set_style_text_color(btn_magnetico, lv_color_white(), LV_PART_MAIN);//cambiar
    lv_obj_center(label_magnetico);
    
    
    // Bot�n VOLVER (Flecha hacia atr�s)
    static pantalla_estado_t destino_principal = PANTALLA_PRINCIPAL;
    lv_obj_t * btn_volver = lv_btn_create(scr);
    lv_obj_set_size(btn_volver, 40, 40);
    
    lv_obj_set_style_bg_color(btn_volver, lv_color_make(0x55, 0x55, 0x55), LV_PART_MAIN); 
    lv_obj_add_style(btn_volver, &style_decoration_base, 0);
    lv_obj_align(btn_volver, LV_ALIGN_TOP_LEFT, 3, 3);
    lv_obj_add_event_cb(btn_volver, callback_cambio_pantalla, LV_EVENT_CLICKED, &destino_principal);
    
    lv_obj_t * label_volver = lv_label_create(btn_volver);
    lv_label_set_text(label_volver, LV_SYMBOL_HOME);
    lv_obj_center(label_volver);
    
    lv_obj_set_style_text_color(label_volver, lv_color_make(0xFF, 0xEB, 0x3B), LV_PART_MAIN); 


    lv_obj_t * panel_datos = lv_obj_create(scr);
    lv_obj_set_size(panel_datos, 100, 210); 
    lv_obj_align(panel_datos, LV_ALIGN_TOP_RIGHT, -10, 35);
    lv_obj_add_style(panel_datos, &style_decoration_base, 0); 
    lv_obj_set_style_border_width(panel_datos, 3, LV_PART_MAIN); 
    lv_obj_set_style_border_color(panel_datos, lv_color_make(0x00, 0x00, 0xA0), LV_PART_MAIN); 
    lv_obj_set_style_bg_color(panel_datos, lv_color_make(0x80, 0x80, 0x80), LV_PART_MAIN); 
    lv_obj_set_style_bg_opa(panel_datos, LV_OPA_COVER, LV_PART_MAIN);
    
    // HORA
    label_time_panel = lv_label_create(panel_datos);
    lv_label_set_text(label_time_panel, "HH:MM:SS");
    lv_obj_set_style_text_font(label_time_panel, &lv_font_montserrat_14, LV_PART_MAIN); 
    lv_obj_set_style_text_color(label_time_panel, lv_color_make(0x00, 0x00, 0xA0), LV_PART_MAIN); 
    lv_obj_align(label_time_panel, LV_ALIGN_TOP_MID, 0, 10);

    // FECHA Y D�A
    label_date_panel = lv_label_create(panel_datos);
    lv_label_set_text(label_date_panel, "DD/MM");
    lv_obj_set_style_text_font(label_date_panel, &lv_font_montserrat_14, LV_PART_MAIN); 
    lv_obj_set_style_text_color(label_date_panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_date_panel, LV_ALIGN_TOP_MID, 0, 30);
    
    label_day_panel = lv_label_create(panel_datos); 
    lv_label_set_text(label_day_panel, "JUEVES");
    lv_obj_set_style_text_font(label_day_panel, &lv_font_montserrat_14, LV_PART_MAIN); 
    lv_obj_set_style_text_color(label_day_panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_day_panel, LV_ALIGN_TOP_MID, 0, 50); 
    
    // ====================================================
    // NUEVO: ESTADO DE ALARMA (Indicador Visual)
    // ====================================================

    // Creamos el contenedor del bot�n
    label_alarm_status_main_panel = lv_btn_create(panel_datos); 
    lv_obj_set_size(label_alarm_status_main_panel, LV_PCT(80), 40);
    lv_obj_add_style(label_alarm_status_main_panel, &style_decoration_base, 0); 
    lv_obj_align(label_alarm_status_main_panel, LV_ALIGN_TOP_MID, 0, 80);

    // Asegurarse de que no se pueda interactuar
    lv_obj_clear_flag(label_alarm_status_main_panel, LV_OBJ_FLAG_CLICKABLE); 
    lv_obj_set_style_pad_all(label_alarm_status_main_panel, 0, 0);

    // Etiqueta interna
    lv_obj_t * status_label = lv_label_create(label_alarm_status_main_panel);
    
    // IMPORTANTE: Ya no importa que diga "Esperando" porque se sobrescribe YA MISMO
    lv_label_set_text(status_label, "..."); 
    lv_obj_set_style_text_color(status_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(status_label);

    // =========================================================================
    //  LA L�NEA M�GICA: Llamar al Pintor para actualizar el estado REAL ahora
    // =========================================================================
    UI_UpdateAlarmStatus();
}

void UI_ShowPantallaConfigLuces(void) {
   lv_obj_t * scr = lv_disp_get_scr_act(NULL);
   
    static keypad_config_t config_delay; 
    static keypad_config_t config_rtc;
    
    // 1. DEFINICI�N DE ESTILOS Y FONDO           r     v     a    +alto +claro
    lv_obj_set_style_bg_color(scr, lv_color_make(0x18, 0x18, 0x5A), LV_PART_MAIN); 
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    
    // Estilo Base de Decoraci�n (Sombra y Radio)
    lv_style_t style_decoration_base;
    lv_style_init(&style_decoration_base);
    lv_style_set_radius(&style_decoration_base, 10); 
    lv_style_set_shadow_width(&style_decoration_base, 10); 
    lv_style_set_shadow_opa(&style_decoration_base, LV_OPA_40); 
    
    // Estilo Base de Fuente (Usamos montserrat_14 y texto blanco)
    lv_style_t style_label_text;
    lv_style_init(&style_label_text);
    lv_style_set_text_font(&style_label_text, &lv_font_montserrat_14); 
    lv_style_set_text_color(&style_label_text, lv_color_white()); 
   
   // 1. TITULO 
   
   // Estilo Base de Fuente (Todo Texto Blanco/Claro)
    lv_style_t style_label_white;
    lv_style_init(&style_label_white);
    lv_style_set_text_font(&style_label_white, &lv_font_montserrat_14);
    lv_style_set_text_color(&style_label_white, lv_color_white());
    
    lv_obj_t * label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "CONFIG DE LUCES");
    
    lv_obj_add_style(label_title, &style_label_white, 0);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_14, 0); 
    lv_obj_set_style_text_color(label_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);
   

    // GRADIENTE VERTICAL PARA LDR
    static lv_style_t style_gradient_ldr_automatico;
    lv_style_init(&style_gradient_ldr_automatico);

    // Colores del gradiente: Rojo oscuro a Rojo vivo (para el bot�n ARMAR)
    static const lv_color_t grad_colors_ldr_automatico[2] = {
        LV_COLOR_MAKE(0x00, 0x50, 0x00), // Rojo MUY Oscuro (Base/Abajo)
        LV_COLOR_MAKE(0x33, 0xFF, 0x33)  // Rojo Claro (Resaltado/Arriba)
    };
    
    // 1. Aplicamos las propiedades de decoraci�n base
    lv_style_set_radius(&style_gradient_ldr_automatico, 10); 
    lv_style_set_shadow_width(&style_gradient_ldr_automatico, 10);
    lv_style_set_shadow_opa(&style_gradient_ldr_automatico, LV_OPA_40);
    
    lv_style_set_bg_color(&style_gradient_ldr_automatico, grad_colors_ldr_automatico[1]);        // Color superior (Base si falla gradiente)
    lv_style_set_bg_grad_color(&style_gradient_ldr_automatico, grad_colors_ldr_automatico[0]);   // Color inferior
    lv_style_set_bg_grad_dir(&style_gradient_ldr_automatico, LV_GRAD_DIR_VER);           // Direcci�n vertical
    lv_style_set_bg_opa(&style_gradient_ldr_automatico, LV_OPA_COVER);                   // Opacidad completa
    
    // 3. Aplicamos las propiedades del borde (SIN el par�metro 'part' en style_set_*)
    lv_style_set_border_width(&style_gradient_ldr_automatico, 2);
    lv_style_set_border_color(&style_gradient_ldr_automatico, lv_color_white());
    
    // GRADIENTE VERTICAL PARA LUZ_DELAY
    static lv_style_t style_gradient_style_LUZ_DELAY;
    lv_style_init(&style_gradient_style_LUZ_DELAY);

    // Colores del gradiente: Rojo oscuro a Rojo vivo (para el bot�n ALARMA)
    static const lv_color_t grad_colors_LUZ_DELAY[2] = {
        LV_COLOR_MAKE(0x50, 0x00, 0x00), // Verde MUY Oscuro (Base/Abajo)  
        LV_COLOR_MAKE(0xFF, 0x33, 0x33)  // Verde Claro (Resaltado/Arriba) 
    };
    
    // 1. Aplicamos las propiedades de decoraci�n base
    lv_style_set_radius(&style_gradient_style_LUZ_DELAY, 10); 
    lv_style_set_shadow_width(&style_gradient_style_LUZ_DELAY, 10);
    lv_style_set_shadow_opa(&style_gradient_style_LUZ_DELAY, LV_OPA_40);
    
    lv_style_set_bg_color(&style_gradient_style_LUZ_DELAY, grad_colors_LUZ_DELAY[1]);        // Color superior (Base si falla gradiente)
    lv_style_set_bg_grad_color(&style_gradient_style_LUZ_DELAY, grad_colors_LUZ_DELAY[0]);   // Color inferior
    lv_style_set_bg_grad_dir(&style_gradient_style_LUZ_DELAY, LV_GRAD_DIR_VER);           // Direcci�n vertical
    lv_style_set_bg_opa(&style_gradient_style_LUZ_DELAY, LV_OPA_COVER);                   // Opacidad completa
    
    // 3. Aplicamos las propiedades del borde (SIN el par�metro 'part' en style_set_*)
    lv_style_set_border_width(&style_gradient_style_LUZ_DELAY, 2);
    lv_style_set_border_color(&style_gradient_style_LUZ_DELAY, lv_color_white());
    
    // GRADIENTE VERTICAL PARA CAMBIAR_PIN
    static lv_style_t style_gradient_LUZ_RTC;
    lv_style_init(&style_gradient_LUZ_RTC);

    // Colores del gradiente: Rojo oscuro a Rojo vivo (para el bot�n ALARMA)
    static const lv_color_t grad_colors_magnetico[2] = {
        LV_COLOR_MAKE(0x10, 0x10, 0x10), // Gris MUY Oscuro (Base/Abajo)
        LV_COLOR_MAKE(0xA0, 0xA0, 0xA0)  // Gris Claro (Resaltado/Arriba)
    };
    
    // 1. Aplicamos las propiedades de decoraci�n base
    lv_style_set_radius(&style_gradient_LUZ_RTC, 10); 
    lv_style_set_shadow_width(&style_gradient_LUZ_RTC, 10);
    lv_style_set_shadow_opa(&style_gradient_LUZ_RTC, LV_OPA_40);
    
    lv_style_set_bg_color(&style_gradient_LUZ_RTC, grad_colors_magnetico[1]);        // Color superior (Base si falla gradiente)
    lv_style_set_bg_grad_color(&style_gradient_LUZ_RTC, grad_colors_magnetico[0]);   // Color inferior
    lv_style_set_bg_grad_dir(&style_gradient_LUZ_RTC, LV_GRAD_DIR_VER);           // Direcci�n vertical
    lv_style_set_bg_opa(&style_gradient_LUZ_RTC, LV_OPA_COVER);                   // Opacidad completa
    
    // 3. Aplicamos las propiedades del borde (SIN el par�metro 'part' en style_set_*)
    lv_style_set_border_width(&style_gradient_LUZ_RTC, 2);
    lv_style_set_border_color(&style_gradient_LUZ_RTC, lv_color_white());
    
    // --- Contenedor para botones de Acci�n (Alineado a la Izquierda) ---
    lv_obj_t * btn_container = lv_obj_create(scr);
    lv_obj_set_size(btn_container, 130, 250); 
    lv_obj_set_style_border_width(btn_container, 0, 0); 
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_align(btn_container, LV_ALIGN_TOP_LEFT, 5, 50); 

    // 1. RTD LUZ AUTOMATICA (Bot�n de color Verde)
    lv_obj_t * btn_luz_automatica = lv_btn_create(btn_container);
    lv_obj_set_size(btn_luz_automatica, 120, 50); 
    
    lv_obj_add_flag(btn_luz_automatica, LV_OBJ_FLAG_CHECKABLE); // hago al boton modo toggle
    
    // Aplicamos el nuevo estilo de gradiente (que ya incluye sombra y borde)
    lv_obj_add_style(btn_luz_automatica, &style_gradient_ldr_automatico, 0); // Aplicamos el gradiente
    
    lv_obj_align(btn_luz_automatica, LV_ALIGN_TOP_LEFT, 0, 10); 
    lv_obj_add_event_cb(btn_luz_automatica, callback_luz_modo_sensor, LV_EVENT_CLICKED, &data_luz_auto);
    lv_obj_set_style_border_width(btn_luz_automatica, 2, LV_PART_MAIN);     // Establece el grosor del borde
    lv_obj_set_style_border_color(btn_luz_automatica, lv_color_white(), LV_PART_MAIN); // Establece el color del borde (Blanco)    
    
    lv_obj_t * label_luz_automatica = lv_label_create(btn_luz_automatica);
    lv_label_set_text(label_luz_automatica,"LUZ\nAUTOMATICA"); 
    lv_obj_add_style(label_luz_automatica, &style_label_text, 0); 
    lv_obj_set_style_text_color(btn_luz_automatica, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(label_luz_automatica);


    // 2. LUZ DELAY (Bot�n de color Rojo)
    lv_obj_t * btn_luz_delay = lv_btn_create(btn_container);
    lv_obj_set_size(btn_luz_delay, 120, 50); 
    
    // Aplicamos el nuevo estilo de gradiente (que ya incluye sombra y borde)
    lv_obj_add_style(btn_luz_delay, &style_gradient_style_LUZ_DELAY, 0); // Aplicamos el gradiente
    
    lv_obj_align_to(btn_luz_delay, btn_luz_automatica, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 30);
    lv_obj_add_event_cb(btn_luz_delay, callback_transicion_luz_delay, LV_EVENT_CLICKED, &config_delay);
    lv_obj_set_style_border_width(btn_luz_delay, 2, LV_PART_MAIN);     // Establece el grosor del borde
    lv_obj_set_style_border_color(btn_luz_delay, lv_color_white(), LV_PART_MAIN); // Establece el color del borde (Blanco)    
    
    lv_obj_t * label_luz_delay = lv_label_create(btn_luz_delay);
    lv_label_set_text(label_luz_delay,"MODO\nDELAY"); 
    lv_obj_add_style(label_luz_delay, &style_label_text, 0); 
    lv_obj_set_style_text_color(btn_luz_delay, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(label_luz_delay);
    

    // 3. LUZ CONFIGURADA POR TIEMPO (NUEVO BOT�N)
  
    lv_obj_t * btn_luz_config_tiempo = lv_btn_create(btn_container);
    lv_obj_set_size(btn_luz_config_tiempo, 120, 50); 
    
    // Aplicamos el nuevo estilo de gradiente (que ya incluye sombra y borde)
    lv_obj_add_style(btn_luz_config_tiempo, &style_gradient_LUZ_RTC, 0); // Aplicamos el gradiente
    
    lv_obj_align_to(btn_luz_config_tiempo, btn_luz_delay, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 30);
    lv_obj_add_event_cb(btn_luz_config_tiempo, callback_transicion_luz_rtc, LV_EVENT_CLICKED, &config_rtc);
    lv_obj_set_style_border_width(btn_luz_config_tiempo, 2, LV_PART_MAIN);     // Establece el grosor del borde
    lv_obj_set_style_border_color(btn_luz_config_tiempo, lv_color_white(), LV_PART_MAIN); // Establece el color del borde (Blanco)    
    
    lv_obj_t * label_luz_config_tiempo= lv_label_create(btn_luz_config_tiempo);
    lv_label_set_text(label_luz_config_tiempo,"CONFIG\nTIEMPO"); 
    lv_obj_add_style(label_luz_config_tiempo, &style_label_text, 0); 
    lv_obj_set_style_text_color(btn_luz_config_tiempo, lv_color_white(), LV_PART_MAIN);//cambiar
    lv_obj_center(label_luz_config_tiempo);
    
    
    // Bot�n VOLVER (Flecha hacia atr�s)
    static pantalla_estado_t destino_principal = PANTALLA_PRINCIPAL;
    lv_obj_t * btn_volver = lv_btn_create(scr);
    lv_obj_set_size(btn_volver, 40, 40);
    
    lv_obj_set_style_bg_color(btn_volver, lv_color_make(0x55, 0x55, 0x55), LV_PART_MAIN); 
    lv_obj_add_style(btn_volver, &style_decoration_base, 0);
    lv_obj_align(btn_volver, LV_ALIGN_TOP_LEFT, 3, 3);
    lv_obj_add_event_cb(btn_volver, callback_cambio_pantalla, LV_EVENT_CLICKED, &destino_principal);
    
    lv_obj_t * label_volver = lv_label_create(btn_volver);
    lv_label_set_text(label_volver, LV_SYMBOL_HOME);
    lv_obj_center(label_volver);
    
    lv_obj_set_style_text_color(label_volver, lv_color_make(0xFF, 0xEB, 0x3B), LV_PART_MAIN); 


    // 2. PANEL DE DATOS EN LA DERECHA (Hora/Fecha Din�mica)
    lv_obj_t * panel_datos = lv_obj_create(scr);
    lv_obj_set_size(panel_datos, 100, 210); 
    lv_obj_align(panel_datos, LV_ALIGN_TOP_RIGHT, -10, 35);

    lv_obj_add_style(panel_datos, &style_decoration_base, 0); 

    lv_obj_set_style_border_width(panel_datos, 3, LV_PART_MAIN); 
    lv_obj_set_style_border_color(panel_datos, lv_color_make(0x00, 0x00, 0xA0), LV_PART_MAIN); 
                                                         
    lv_obj_set_style_bg_color(panel_datos, lv_color_make(0x80, 0x80, 0x80), LV_PART_MAIN); 

    lv_obj_set_style_bg_opa(panel_datos, LV_OPA_COVER, LV_PART_MAIN);
    
    
    // HORA
    label_time_panel = lv_label_create(panel_datos);
    lv_label_set_text(label_time_panel, "HH:MM:SS");
    lv_obj_set_style_text_font(label_time_panel, &lv_font_montserrat_14, LV_PART_MAIN); 
    lv_obj_set_style_text_color(label_time_panel, lv_color_make(0x00, 0x00, 0xA0), LV_PART_MAIN); // Azul p�lido
    lv_obj_align(label_time_panel, LV_ALIGN_TOP_MID, 0, 10);

    // FECHA Y D�A
    label_date_panel = lv_label_create(panel_datos);
    lv_label_set_text(label_date_panel, "DD/MM");
    lv_obj_set_style_text_font(label_date_panel, &lv_font_montserrat_14, LV_PART_MAIN); 
    lv_obj_set_style_text_color(label_date_panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_date_panel, LV_ALIGN_TOP_MID, 0, 30);
    
    label_day_panel = lv_label_create(panel_datos); 
    lv_label_set_text(label_day_panel, "JUEVES");
    lv_obj_set_style_text_font(label_day_panel, &lv_font_montserrat_14, LV_PART_MAIN); 
    lv_obj_set_style_text_color(label_day_panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_day_panel, LV_ALIGN_TOP_MID, 0, 50); 



   lv_obj_t * label_status_luz = lv_label_create(panel_datos);
    
    // Texto inicial
    lv_label_set_text(label_status_luz, "SENSOR: --"); 
    
    // Estilo (Fuente peque�a o normal)
    lv_obj_set_style_text_font(label_status_luz, &lv_font_montserrat_14, LV_PART_MAIN); 
    lv_obj_set_style_text_color(label_status_luz, lv_color_white(), LV_PART_MAIN);
    
    // Posici�n: DEBAJO del d�a (Si el d�a est� en Y=50, este va en Y=80)
    lv_obj_align(label_status_luz, LV_ALIGN_TOP_MID, 0, 80); 

    // [CRUCIAL] GUARDAMOS LA ETIQUETA EN LA ESTRUCTURA GLOBAL
    data_luz_auto.label_status = label_status_luz;
    
    // Sincronizaci�n visual inicial (Si arranc� activado)
    if(lv_obj_has_state(btn_luz_automatica, LV_STATE_CHECKED)) {
         lv_label_set_text(label_status_luz, "SENSOR: ON");
         lv_obj_set_style_text_color(label_status_luz, lv_color_make(50, 255, 50), 0);
    }
}

void UI_ShowPantallaConfigTempHum(void) {
    lv_obj_t * scr = lv_disp_get_scr_act(NULL);
   
    lv_obj_clean(scr); 
    lv_obj_remove_style_all(scr);

    // 1. ESTILOS Y FONDO
    lv_obj_set_style_bg_color(scr, lv_color_make(0x18, 0x18, 0x5A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    // Estilos Locales
    static lv_style_t style_decoration_base;
    static lv_style_t style_label_text;
    static lv_style_t style_label_white;
    
    lv_style_init(&style_decoration_base);
    lv_style_set_radius(&style_decoration_base, 10);
    lv_style_set_shadow_width(&style_decoration_base, 10);
    lv_style_set_shadow_opa(&style_decoration_base, LV_OPA_40);

    lv_style_init(&style_label_text);
    lv_style_set_text_font(&style_label_text, &lv_font_montserrat_14);
    lv_style_set_text_color(&style_label_text, lv_color_white());

    lv_style_init(&style_label_white);
    lv_style_set_text_font(&style_label_white, &lv_font_montserrat_14);
    lv_style_set_text_color(&style_label_white, lv_color_white());

    // T�tulo
    lv_obj_t * label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "CONFIG CLIMATIZACION");
    lv_obj_add_style(label_title, &style_label_white, 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);
    

    // GRADIENTE VERTICAL PARA CALOR
    static lv_style_t style_gradient_calor;
    lv_style_init(&style_gradient_calor);

    // Colores del gradiente: Rojo oscuro a Rojo vivo (para el bot�n ARMAR)
    static const lv_color_t grad_colors_calor[2] = {
        LV_COLOR_MAKE(0x00, 0x50, 0x00), // Rojo MUY Oscuro (Base/Abajo)
        LV_COLOR_MAKE(0x33, 0xFF, 0x33)  // Rojo Claro (Resaltado/Arriba)
    };
    
    // 1. Aplicamos las propiedades de decoraci�n base
    lv_style_set_radius(&style_gradient_calor, 10); 
    lv_style_set_shadow_width(&style_gradient_calor, 10);
    lv_style_set_shadow_opa(&style_gradient_calor, LV_OPA_40);
    
    lv_style_set_bg_color(&style_gradient_calor, grad_colors_calor[1]);        // Color superior (Base si falla gradiente)
    lv_style_set_bg_grad_color(&style_gradient_calor, grad_colors_calor[0]);   // Color inferior
    lv_style_set_bg_grad_dir(&style_gradient_calor, LV_GRAD_DIR_VER);           // Direcci�n vertical
    lv_style_set_bg_opa(&style_gradient_calor, LV_OPA_COVER);                   // Opacidad completa
    
    // 3. Aplicamos las propiedades del borde (SIN el par�metro 'part' en style_set_*)
    lv_style_set_border_width(&style_gradient_calor, 2);
    lv_style_set_border_color(&style_gradient_calor, lv_color_white());
    
    // GRADIENTE VERTICAL PARA FRIO
    static lv_style_t style_gradient_frio;
    lv_style_init(&style_gradient_frio);

    // Colores del gradiente: Rojo oscuro a Rojo vivo (para el bot�n ALARMA)
    static const lv_color_t grad_colors_frio[2] = {
    LV_COLOR_MAKE(0x20, 0x20, 0x20), // Gris Muy Oscuro (Base/Abajo)
    LV_COLOR_MAKE(0x60, 0x60, 0x60)  // Gris Medio (Resaltado/Arriba)
   };
    
    // 1. Aplicamos las propiedades de decoraci�n base
    lv_style_set_radius(&style_gradient_frio, 10); 
    lv_style_set_shadow_width(&style_gradient_frio, 10);
    lv_style_set_shadow_opa(&style_gradient_frio, LV_OPA_40);
    
    lv_style_set_bg_color(&style_gradient_frio, grad_colors_frio[1]);        // Color superior (Base si falla gradiente)
    lv_style_set_bg_grad_color(&style_gradient_frio, grad_colors_frio[0]);   // Color inferior
    lv_style_set_bg_grad_dir(&style_gradient_frio, LV_GRAD_DIR_VER);           // Direcci�n vertical
    lv_style_set_bg_opa(&style_gradient_frio, LV_OPA_COVER);                   // Opacidad completa
    
    // 3. Aplicamos las propiedades del borde (SIN el par�metro 'part' en style_set_*)
    lv_style_set_border_width(&style_gradient_frio, 2);
    lv_style_set_border_color(&style_gradient_frio, lv_color_white());

    // Contenedor Izquierdo
    lv_obj_t * btn_container = lv_obj_create(scr);
    lv_obj_set_size(btn_container, 110, 250);
    lv_obj_set_style_border_width(btn_container, 0, 0);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_align(btn_container, LV_ALIGN_TOP_LEFT, 5, 50);
    
    

    // =======================================================
    // 1. BOT�N TOGGLE "MODO AUTOM�TICO"
    // =======================================================
    lv_obj_t * btn_auto = lv_btn_create(btn_container);
    lv_obj_set_size(btn_auto, 110, 75);
    lv_obj_add_style(btn_auto, &style_gradient_calor, 0); // Aplicamos el gradiente
    lv_obj_align(btn_auto, LV_ALIGN_TOP_LEFT, 0, 10);
    lv_obj_add_flag(btn_auto, LV_OBJ_FLAG_CHECKABLE); // Toggle

    // **SINCRONIZACI�N**: Si MEF est� en modo AUTO, mostrar apretado
    if (MEF_Caloventor_IsAuto()) {
        lv_obj_add_state(btn_auto, LV_STATE_CHECKED);
    }

    // **CALLBACK NUEVO**
    lv_obj_add_event_cb(btn_auto, callback_auto_mode_toggle, LV_EVENT_VALUE_CHANGED, &data_modo_auto);

    lv_obj_t * lbl_auto = lv_label_create(btn_auto);
    lv_label_set_text(lbl_auto, "MODO AUTO\nON/OFF");
    lv_obj_add_style(lbl_auto, &style_label_text, 0);
    lv_obj_center(lbl_auto);
    
    // =======================================================
    // 2. PANEL INFO UMBRALES (Est�tico)
    // =======================================================
    lv_obj_t * panel_info = lv_obj_create(btn_container);
    lv_obj_set_size(panel_info, 110, 90);
    lv_obj_add_style(panel_info, &style_gradient_frio, 0);
    lv_obj_align_to(panel_info, btn_auto, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    
    // T�tulo Info
    lv_obj_t * lbl_info_title = lv_label_create(panel_info);
    lv_label_set_text(lbl_info_title, "UMBRALES");
    lv_obj_add_style(lbl_info_title, &style_label_text, 0);
    lv_obj_align(lbl_info_title, LV_ALIGN_TOP_MID, 0, 5);

    // Valor Fr�o
    lv_obj_t * lbl_frio = lv_label_create(panel_info);
    lv_label_set_text_fmt(lbl_frio, "Calor < %d C", MEF_Caloventor_GetUmbralCaliente()); 
    lv_obj_add_style(lbl_frio, &style_label_text, 0);
    lv_obj_align(lbl_frio, LV_ALIGN_TOP_MID, 0, 30);

    // Valor Calor
    lv_obj_t * lbl_calor = lv_label_create(panel_info);
    lv_label_set_text_fmt(lbl_calor, "Vent > %d C", MEF_Caloventor_GetUmbralFrio()); 
    lv_obj_add_style(lbl_calor, &style_label_text, 0);
    lv_obj_align(lbl_calor, LV_ALIGN_TOP_MID, 0, 50);

    // --- PANEL DE DATOS (DERECHA) ---
    lv_obj_t * panel_datos = lv_obj_create(scr);
    lv_obj_set_size(panel_datos, 100, 210);
    lv_obj_align(panel_datos, LV_ALIGN_TOP_RIGHT, -10, 35);
    lv_obj_add_style(panel_datos, &style_decoration_base, 0);
    lv_obj_set_style_border_width(panel_datos, 3, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel_datos, lv_color_make(0x00, 0x00, 0xA0), LV_PART_MAIN);
    lv_obj_set_style_bg_color(panel_datos, lv_color_make(0x80, 0x80, 0x80), LV_PART_MAIN);

    // Datos Hora/Fecha (Igual que antes)
    label_time_panel = lv_label_create(panel_datos);
    lv_label_set_text(label_time_panel, "HH:MM:SS");
    lv_obj_set_style_text_font(label_time_panel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_time_panel, lv_color_make(0x00, 0x00, 0xA0), LV_PART_MAIN);
    lv_obj_align(label_time_panel, LV_ALIGN_TOP_MID, 0, 10);

    label_date_panel = lv_label_create(panel_datos);
    lv_label_set_text(label_date_panel, "DD/MM");
    lv_obj_set_style_text_font(label_date_panel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_date_panel, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_date_panel, LV_ALIGN_TOP_MID, 0, 30);

    label_day_panel = lv_label_create(panel_datos);
    lv_label_set_text(label_day_panel, "DIA");
    lv_obj_set_style_text_font(label_day_panel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(label_day_panel, LV_ALIGN_TOP_MID, 0, 50);

    // TEMPERATURA
    label_temp_txt = lv_label_create(panel_datos);
    lv_label_set_text(label_temp_txt, "Temp:");
    lv_obj_set_style_text_font(label_temp_txt, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_temp_txt, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_temp_txt, LV_ALIGN_TOP_MID, 0, 80);

    label_temp_value = lv_label_create(panel_datos);
    lv_label_set_text_fmt(label_temp_value, "%.1f C", get_current_temperature());
    lv_obj_align(label_temp_value, LV_ALIGN_TOP_MID, 0, 95);

    // HUMEDAD
    label_hum_txt = lv_label_create(panel_datos);
    lv_label_set_text(label_hum_txt, "Hum:");
    lv_obj_set_style_text_font(label_hum_txt, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_hum_txt, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_hum_txt, LV_ALIGN_TOP_MID, 0, 115);

    label_hum_value = lv_label_create(panel_datos);
    lv_label_set_text_fmt(label_hum_value, "%.1f %%", get_current_humidity());
    lv_obj_align(label_hum_value, LV_ALIGN_TOP_MID, 0, 130);
    
    //feedback auto on/off
    lv_obj_t * lbl_modo_sys = lv_label_create(panel_datos);
    
    lv_obj_set_style_text_font(lbl_modo_sys, &lv_font_montserrat_10, LV_PART_MAIN); 
    lv_obj_align(lbl_modo_sys, LV_ALIGN_BOTTOM_MID, 0, -20); 

    data_modo_auto.label_status = lbl_modo_sys;

    // --- SINCRONIZACI�N INICIAL DEL TEXTO ---
    // Verificamos c�mo est� la m�quina AHORA para pintar el texto correcto al entrar
    if (MEF_Caloventor_IsAuto()) {
        // Si ya estaba en auto, pintamos Verde y texto AUTO
        lv_label_set_text(lbl_modo_sys, "SISTEMA: AUTO");
        lv_obj_set_style_text_color(lbl_modo_sys, lv_color_make(50, 255, 255), 0); // Cyan/Verde
        
        // Aseguramos que el bot�n tambi�n se vea apretado (por si acaso)
        lv_obj_add_state(btn_auto, LV_STATE_CHECKED);
    } else {
        // Si est� en manual, pintamos Gris y texto MANUAL
        lv_label_set_text(lbl_modo_sys, "SISTEMA: MANUAL");
        lv_obj_set_style_text_color(lbl_modo_sys, lv_color_make(180, 180, 180), 0); // Gris
    }
    
    // Bot�n VOLVER (Flecha hacia atr�s)
    static pantalla_estado_t destino_principal = PANTALLA_TEMP_HUM;
    lv_obj_t * btn_volver = lv_btn_create(scr);
    lv_obj_set_size(btn_volver, 40, 40);
    
    lv_obj_set_style_bg_color(btn_volver, lv_color_make(0x55, 0x55, 0x55), LV_PART_MAIN); 
    lv_obj_add_style(btn_volver, &style_decoration_base, 0);
    lv_obj_align(btn_volver, LV_ALIGN_TOP_LEFT, 3, 3);
    lv_obj_add_event_cb(btn_volver, callback_cambio_pantalla, LV_EVENT_CLICKED, &destino_principal);
    
    lv_obj_t * label_volver = lv_label_create(btn_volver);
    lv_label_set_text(label_volver, LV_SYMBOL_HOME);
    lv_obj_center(label_volver);
    
    lv_obj_set_style_text_color(label_volver, lv_color_make(0xFF, 0xEB, 0x3B), LV_PART_MAIN); 

    UI_UpdateTempHum();
    
}


void MEF_UI_INIT(void) {
    estado_actual = PANTALLA_PRINCIPAL;
}

void UI_Update(void) {
    // Esta funci�n se puede usar para actualizar labels de la pantalla
}