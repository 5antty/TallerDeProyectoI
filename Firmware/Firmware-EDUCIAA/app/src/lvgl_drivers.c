#include "lvgl_drivers.h"
#include "sapi.h"
#include <string.h>

// --- DEFINICIONES Y VARIABLES DE HARDWARE ---
// las definiciones de pinesest�n en lvgl_drivers.h
#define ILI9341_CMD_SWRESET     0x01
#define ILI9341_CMD_SLEEPOUT    0x11
#define ILI9341_CMD_DISPON      0x29
#define ILI9341_CMD_CASET       0x2A 
#define ILI9341_CMD_PASET       0x2B 
#define ILI9341_CMD_RAMWR       0x2C 
#define ILI9341_CMD_PIXFMT      0x3A
#define ILI9341_CMD_MADCTL      0x36 
#define ILI9341_PIXEL_FORMAT_RGB565 0x55 

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

// Comandos de lectura XPT2046
#define CMD_X_READ 0xD0
#define CMD_Y_READ 0x90

// 3. Mapeo y Normalizaci�n (VALORES REALES MEDIDOS)
#define X_MIN 4371 // RAW Izquierda (0 px)
#define X_MAX 7955 // RAW Derecha (240 px) 
#define Y_MIN 4371 // RAW Abajo (320 px)
#define Y_MAX 7955// RAW Arriba (0 px)

#define TOUCH_STABILITY_THRESHOLD 4 
extern int touch_press_count;
static spiMap_t spi_bus_global; 
static int last_raw_x = 0;
static int last_raw_y = 0;

// --- FUNCI�N DE UTILIDAD: SWAP BYTES ---
static void swap_bytes(uint16_t* color_p, size_t len) {
    for (size_t i = 0; i < len; i++) {
        uint16_t val = color_p[i];
        color_p[i] = (val >> 8) | (val << 8); 
    }
}

// IMPLEMENTACIONES DE BAJO NIVEL -> esto anda medio medio pero funca

void ili9341_write_command(uint8_t cmd) {
    gpioWrite(PIN_LCD_CS, 0); 
    gpioWrite(PIN_LCD_DC, 0); 
    spiWrite(spi_bus_global, &cmd, 1);
    gpioWrite(PIN_LCD_CS, 1);
}

void ili9341_write_data(uint8_t *data, size_t len) {
    gpioWrite(PIN_LCD_CS, 0); 
    gpioWrite(PIN_LCD_DC, 1); 
    spiWrite(spi_bus_global, data, len);
    gpioWrite(PIN_LCD_CS, 1);
}


// --- FUNCIONES PARA PRENDER EL SPI ---

void lvgl_spi_init(spiMap_t spi_bus) {
    spi_bus_global = spi_bus;
    spiInit(spi_bus); 
   

    gpioConfig(PIN_LCD_CS, GPIO_OUTPUT);
    gpioConfig(PIN_LCD_DC, GPIO_OUTPUT);
    gpioConfig(PIN_LCD_RST, GPIO_OUTPUT);
    
    gpioWrite(PIN_LCD_CS, 1);
    gpioWrite(PIN_LCD_RST, 1);
}


void lvgl_display_init(uint8_t cs, uint8_t dc, uint8_t rst) {
    // 1. Reset
    gpioWrite(PIN_LCD_RST, 0);
    delay(10);
    gpioWrite(PIN_LCD_RST, 1);
    delay(120);

    // 2. Secuencia de Inicializaci�n ILI9341
    ili9341_write_command(ILI9341_CMD_SWRESET);
    delay(5);
    ili9341_write_command(ILI9341_CMD_SLEEPOUT);
    delay(120);

    ili9341_write_command(0xC0); ili9341_write_data((uint8_t[]){0x23}, 1);
    ili9341_write_command(0xC5); ili9341_write_data((uint8_t[]){0x3E, 0x28}, 2);

    // Memory Access Control (MADCTL) -> esto maneja si es vertical u horizontal
    ili9341_write_command(ILI9341_CMD_MADCTL);
    ili9341_write_data((uint8_t[]){0x48}, 1); 

    ili9341_write_command(ILI9341_CMD_PIXFMT);
    ili9341_write_data((uint8_t[]){ILI9341_PIXEL_FORMAT_RGB565}, 1);

    ili9341_write_command(ILI9341_CMD_DISPON);
}


void lvgl_display_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    uint8_t data[4];
    
    // 1. Configurar ventana (Esto dejalo igual, estaba bien)
    data[0] = (uint8_t)(area->x1 >> 8); data[1] = (uint8_t)area->x1; 
    data[2] = (uint8_t)(area->x2 >> 8); data[3] = (uint8_t)area->x2; 
    ili9341_write_command(ILI9341_CMD_CASET);
    ili9341_write_data(data, 4);

    data[0] = (uint8_t)(area->y1 >> 8); data[1] = (uint8_t)area->y1; 
    data[2] = (uint8_t)(area->y2 >> 8); data[3] = (uint8_t)area->y2; 
    ili9341_write_command(ILI9341_CMD_PASET);
    ili9341_write_data(data, 4);

    ili9341_write_command(ILI9341_CMD_RAMWR);

    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    uint32_t len = w * h;

    
    // Enviamos por SPI
    gpioWrite(PIN_LCD_CS, 0); 
    gpioWrite(PIN_LCD_DC, 1); 
    
    // Nota: len * 2 porque son bytes
    spiWrite(spi_bus_global, (uint8_t *)color_p, len * 2);
    
    gpioWrite(PIN_LCD_CS, 1); 

    lv_disp_flush_ready(disp_drv);
}

void lvgl_touch_init(uint8_t cs, uint8_t irq) {
    gpioConfig(cs, GPIO_OUTPUT);
    gpioConfig(irq, GPIO_INPUT); 
    
    gpioWrite(cs, 1); 
    
 
}

static uint16_t read_touch_coord(uint8_t command) {
    uint8_t data_out = command;
    uint8_t data_in[2] = {0x00, 0x00};
    
    
    spiWrite(spi_bus_global, &data_out, 1);
    delay(1); //sin este delay el tick del lvgl no funciona
    spiRead(spi_bus_global, data_in, 2); 
    
    uint16_t coord = (data_in[0] << 8 | data_in[1]) >> 3; 
    
    return coord;
}

bool lvgl_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    
    gpioWrite(PIN_TOUCH_CS, 0); 
    int raw_x = read_touch_coord(CMD_X_READ);
    int raw_y = read_touch_coord(CMD_Y_READ);
    gpioWrite(PIN_TOUCH_CS, 1); // Desactiva el chip inmediatamente
    
    // ----------------------------------------------------
    // 1. FILTRADO DE RUIDO 
    // ----------------------------------------------------
    
    if (raw_x < 400 || raw_x > 7900 || raw_y < 400 || raw_y > 7900) {
        touch_press_count = 0; // Reinicia el contador de estabilidad
        data->state = LV_INDEV_STATE_RELEASED;
        return false;
    }
    
    // Filtro 2: Deteccion de Movimiento Brusco 
    // Si el toque actual se movio m�s de 100 unidades RAW desde la ultima muestra
    #define MOVE_THRESHOLD 100 
    
    if (
        (abs(raw_x - last_raw_x) > MOVE_THRESHOLD) || 
        (abs(raw_y - last_raw_y) > MOVE_THRESHOLD)
    ) {
        touch_press_count = 0; // Si hay movimiento brusco, reiniciar la cuenta
        last_raw_x = raw_x;    // Actualiza la posici�n inicial del movimiento
        last_raw_y = raw_y;
        data->state = LV_INDEV_STATE_RELEASED;
        return false;
    }

    // ----------------------------------------------------
    // 3. FILTRO DE ESTABILIDAD (Genera el evento PRESSED)
    // ----------------------------------------------------
    
    if (touch_press_count < TOUCH_STABILITY_THRESHOLD) {
        touch_press_count++;
        last_raw_x = raw_x; 
        last_raw_y = raw_y;
        
        data->state = LV_INDEV_STATE_RELEASED;
        return false;
    }

    // ----------------------------------------------------
    // 4. REPORTE FINAL (Toque V�lido y Estable)
    // ----------------------------------------------------

    // Mapeo X (0-240) - Directo
    data->point.x = (raw_x - X_MIN) * 240 / (X_MAX - X_MIN);
    
    // Mapeo Y (0-320) - Inverso (Y_MAX es 0)
    data->point.y = (Y_MAX - raw_y) * 320 / (Y_MAX - Y_MIN);

    // Reportar el estado a LVGL
    data->state = LV_INDEV_STATE_PRESSED;
    
    //DEBUG
    char buffer[64];
    sprintf(buffer, "MAP: (%d, %d) | PRESSED (Stable)\r\n", 
            (int)data->point.x, (int)data->point.y);
    uartWriteString(UART_USB, buffer);
   

    return true; 
}