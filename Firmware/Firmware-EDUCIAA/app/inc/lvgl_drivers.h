#ifndef _LVGL_DRIVERS_H_
#define _LVGL_DRIVERS_H_

#include "sapi.h"
#include "lvgl.h"
//ultima
#define PIN_LCD_CS    LCD1  

#define PIN_LCD_RST   LCD2  

#define PIN_LCD_DC    LCD3  

#define PIN_TOUCH_CS  LCDRS 

#define PIN_TOUCH_IRQ LCD4 

#define SPI_BUS       SPI0

// Pin 16 (P2): TXD1 -> Reservado para UART1_TXD

// --- PROTOTIPOS DE FUNCIONES DE INICIALIZACIÓN ---
void lvgl_spi_init(spiMap_t spi_bus);
void lvgl_display_init(uint8_t cs, uint8_t dc, uint8_t rst);
void lvgl_touch_init(uint8_t cs, uint8_t irq);

// --- PROTOTIPOS LVGL REQUERIDOS ---
void lvgl_display_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);
bool lvgl_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
void lvgl_display_init_driver(void);
void lvgl_touch_init(uint8_t cs, uint8_t irq);
bool lvgl_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

#endif /* _LVGL_DRIVERS_H_ */