#include "manejo_uart.h"


void procesar_mensaje(char *mensaje)
{
    static char buffer[30];
    sprintf(buffer, "Mensaje recibido: %s\r\n", mensaje);
    uartWriteString(UART_USB, buffer);

    // Verificar formato b�sico
    if (mensaje[0] != '#')
    {
        // ESP_LOGW(TAG_UART, "Mensaje sin inicio v�lido");
        uartWriteString(UART_USB, "Mensaje sin inicio v�lido\r\n");
        return;
    }

    char *fin = strchr(mensaje, '*'); // Buscar fin de mensaje
    if (fin == NULL)
    {
        // ESP_LOGW(TAG_UART, "Mensaje sin fin v�lido");
        uartWriteString(UART_USB, "Mensaje sin fin vlido\r\n");
        return;
    }

    // Extraer tipo de mensaje
    char tipo = mensaje[1];

    // Extraer datos (despu�s de tipo y coma, antes de *)
    char datos[MAX_MSG_SIZE];
    int datos_len = fin - (mensaje + 3);
    if (datos_len > 0 && datos_len < MAX_MSG_SIZE)
    {
        strncpy(datos, mensaje + 3, datos_len);
        datos[datos_len] = '\0';
    }
    else
    {
        datos[0] = '\0';
    }

    // Procesar seg�n tipo
    switch (tipo)
    {
        /*
       #T,25.5,60.2*\n          // Temperatura y humedad
       #A,ON*\n       // Acci�n: ENCENDER ALARMA
       #C,ON*\n       // Acci�n: ENCENDER CALOVENTOR
       #V,OFF*\n       // Acci�n: APAGAR VENTILADOR
       #L,1ON*\n             // Acci�n: ENCENDER LUZ 1
       #L,2OFF*\n            // Acci�n: APAGAR LUZ 2
       #S,1,89*\n            // Estado de del slider 1 y 2
       */
    case 'L':
        if (datos[0] == '1')
        {
            if (datos[2] == 'N')
                MEF_Luz_SetMode(0, LUZ_ON, 100);
            else if (datos[2] == 'F')
                MEF_Luz_SetMode(0, LUZ_OFF, 0);
        }
        else if (datos[0] == '2')
        {
            if (datos[2] == 'N')
                MEF_Luz_SetMode(1, LUZ_ON, 100);
            else if (datos[2] == 'F')
                MEF_Luz_SetMode(1, LUZ_OFF, 0);
        }
        break;
    case 'S':
        if (datos[0] == '1')
            MEF_Luz_SetMode(0, LUZ_DIMMING, atoi(datos + 2));
        else if (datos[0] == '2')
            MEF_Luz_SetMode(1, LUZ_DIMMING, atoi(datos + 2));
        break;
    case 'C':
        if (datos[1] == 'N')
            MEF_Caloventor_SetCalor();
        else if (datos[1] == 'F')
            MEF_Caloventor_SetOff();
        break;
    case 'V':
        if (datos[1] == 'N')
            MEF_Caloventor_SetVentilador();
        else if (datos[1] == 'F')
            MEF_Caloventor_SetOff();
        break;
    /* case 'A':
        if (datos[1] == 'N')
            mqtt_publish("smarthome/web/alarma", "ON");
        else if (datos[1] == 'F')
            mqtt_publish("smarthome/web/alarma", "OFF");
        break; */
    default:
        uartWriteString(UART_USB, "Tipo de mensaje desconocido\r\n");
    }
}

void leer_uart_esp32(void) {
    uint8_t data;
    char mensaje[MAX_MSG_SIZE];
    char c;
    int msg_idx = 0;
    bool recibiendo = false;
    
    
    // Leer todos los bytes disponibles
    while(uartReadByte(UART_232, &data)) {
        uartWriteString(UART_USB, "RECIBI ALGO\r\n");
        c = (char)data;
        
        // Detectar inicio de mensaje
        if (c == '#') {
            recibiendo = true;
            msg_idx = 0;
            mensaje[msg_idx++] = c;
        }
        // Recibir datos
        else if (recibiendo) {
            if (msg_idx < MAX_MSG_SIZE - 1) {
                mensaje[msg_idx++] = c;
                
                // Detectar fin de mensaje
                if ((c == '\n')||(c == '\r')) {
                    mensaje[msg_idx] = '\0';
                    procesar_mensaje(mensaje);
                    recibiendo = false;
                    msg_idx = 0;
                }
            }
            else {
                // Buffer overflow
                uartWriteString(UART_USB, "WARN: Mensaje demasiado largo\r\n");
                recibiendo = false;
                msg_idx = 0;
            }
        }
    }
}

void txCmd(char tipo, const char *datos)
{
    char mensaje[MAX_MSG_SIZE];
    switch (tipo)
    {
    case 'T':
        sprintf(mensaje, "#T,%s*\r", datos);
        uartWriteString(UART_232, mensaje);
        break;
    case 'L':
        snprintf(mensaje, MAX_MSG_SIZE, "#L,%s*\r", datos);
        uartWriteString(UART_232, mensaje);
        break;
    case 'C':
        snprintf(mensaje, MAX_MSG_SIZE, "#C,%s*\r", datos);
        uartWriteString(UART_232, mensaje);
        break;

    case 'V':
        snprintf(mensaje, MAX_MSG_SIZE, "#V,%s*\r", datos);
        uartWriteString(UART_232, mensaje);
        break;
    case 'S':
        snprintf(mensaje, MAX_MSG_SIZE, "#S,%s*\r", datos);
        uartWriteString(UART_232, mensaje);
        break;
    case 'A':
        snprintf(mensaje, MAX_MSG_SIZE, "#A,%s*\r", datos);
        uartWriteString(UART_232, mensaje);
        break;

    default:
        break;
    }
    uartWriteString(UART_USB, mensaje);//DEBUG
}
