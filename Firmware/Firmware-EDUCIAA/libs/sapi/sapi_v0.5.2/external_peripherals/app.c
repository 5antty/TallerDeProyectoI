#include "sapi.h"
#include "lvgl_drivers.h" // Incluye los drivers
#include <stdio.h> 
#include <string.h>

// Definiciones de Pines GPIO para la Pantalla (Necesario para el driver)
#define PIN_LCD_CS    GPIO0 
#define PIN_LCD_DC    GPIO2
#define PIN_LCD_RST   GPIO4
#define SPI_BUS       SPI0

// Colores de prueba RGB565
#define TEST_COLOR_RED    0xF800
#define TEST_COLOR_BLUE   0x001F

// Prototipo de la función de prueba
extern void tft_test_fill_screen(uint16_t color);


int main(void) {
    boardConfig();
    uartConfig(UART_USB, 115200);
    tickConfig(1);
    uartWriteString(UART_USB, "Iniciando Test Mínimo de Display...\r\n");
    
    // 1. Inicializa el Bus SPI y los Pines de Control
    lvgl_spi_init(SPI_BUS); 
    
    // 2. Inicializa el Display ILI9341 (Secuencia de comandos)
    lvgl_display_init(PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST);
    
    uartWriteString(UART_USB, "Inicialización completada. Probando colores...\r\n");

    while(true) {
        // TAREA 1: Pantalla AZUL
        uartWriteString(UART_USB, "Pantalla Azul...\r\n");
        tft_test_fill_screen(TEST_COLOR_BLUE);
        gpioWrite(LEDB, ON); // Indicar actividad con el LED Azul
        delay(2000); 

        // TAREA 2: Pantalla ROJO
        uartWriteString(UART_USB, "Pantalla Roja...\r\n");
        tft_test_fill_screen(TEST_COLOR_RED);
        gpioWrite(LEDR, ON); // Indicar actividad con el LED Rojo
        delay(2000);
    }
}