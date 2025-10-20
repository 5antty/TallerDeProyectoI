#ifndef MEF_UI_H
#define MEF_UI_H

#include <stdint.h>
#include <stdbool.h>



typedef enum {
    PANTALLA_PRINCIPAL,
    PANTALLA_ALARMA,
    PANTALLA_CONFIG_ALARMA,
    PANTALLA_LUCES,
    PANTALLA_CONFIG_LUCES,
    PANTALLA_TEMP_HUM,
    PANTALLA_CONFIG_TEMP_HUM
} pantalla_estado_t;

void MEF_UI_INIT(void);
void UI_Update(void);

void UI_ShowPantallaPrincipal(void);
void UI_ShowPantallaAlarma(void);
void UI_ShowPantallaConfigAlarma(void);
void UI_ShowPantallaLuces(void);
void UI_ShowPantallaConfigLuces(void);
void UI_ShowPantallaTempHum(void);
void UI_ShowPantallaConfigTempHum(void);


#endif
