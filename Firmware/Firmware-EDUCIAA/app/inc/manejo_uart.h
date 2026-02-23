#ifndef MANEJO_UART_H_
#define MANEJO_UART_H_

#include "sapi.h"
#include "mef_alarma.h"
#include "mef_luces.h"
#include "mef_caloventor.h"
#include "mef_ui.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_MSG_SIZE 30

void procesar_mensaje(char *mensaje);
void leer_uart_esp32(void);
void txCmd(char tipo, const char *datos);
#endif