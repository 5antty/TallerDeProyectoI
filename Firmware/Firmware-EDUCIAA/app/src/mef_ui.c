#include "lvgl.h"
#include "sapi.h"   // si necesitás UART/logs, sino podés quitarlo


typedef enum {
    PANTALLA_PRINCIPAL = 0,
    PANTALLA_SECUNDARIA
} pantalla_estado_t;

static pantalla_estado_t pantallaActual = PANTALLA_PRINCIPAL;


void MEF_UI_INIT(void);
void UI_Update(void);

static void UI_ShowPantallaPrincipal(void);
static void UI_ShowPantallaSecundaria(void);


static void Boton_CambiarPantalla(lv_event_t * e);


void MEF_UI_INIT(void) {
    pantallaActual = PANTALLA_PRINCIPAL;
    UI_ShowPantallaPrincipal();
}


void UI_Update(void) {
    switch(pantallaActual) {
        case PANTALLA_PRINCIPAL:
            UI_ShowPantallaPrincipal();
            break;
        case PANTALLA_SECUNDARIA:
            UI_ShowPantallaSecundaria();
            break;
    }
}


static void UI_ShowPantallaPrincipal(void) {
    lv_obj_clean(lv_scr_act());

    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn, Boton_CambiarPantalla, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, "Ir a SECUNDARIA");
}

static void UI_ShowPantallaSecundaria(void) {
    lv_obj_clean(lv_scr_act());

    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label, "Pantalla SECUNDARIA");

    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(btn, Boton_CambiarPantalla, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(btn), "VOLVER");
}


static void Boton_CambiarPantalla(lv_event_t * e) {
    (void)e;
    if(pantallaActual == PANTALLA_PRINCIPAL) {
        pantallaActual = PANTALLA_SECUNDARIA;
        UI_ShowPantallaSecundaria();
    } else {
        pantallaActual = PANTALLA_PRINCIPAL;
        UI_ShowPantallaPrincipal();
    }
}

