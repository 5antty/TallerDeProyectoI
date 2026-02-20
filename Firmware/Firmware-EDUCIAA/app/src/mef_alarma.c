#include "mef_alarma.h"
#include <string.h>
#include <stdio.h>

static alarma_estado_t estado;
static uint32_t t_sistema;
static uint32_t t_estado;           // tiempo estado
static uint32_t t_gracia;         // tiempo fin armado (ARMING)-> t_estado + ARM_DELAY_GRACIA_MS. Tiempo que doy a la persona para que pueda salir de la casa
static uint32_t t_fin_gracia;       // fin de ventana de gracia
static int intentos_clave;

static char pin_guardado[PIN_MAX_LONGITUD + 1] = "2222"; // PIN por defecto (editable)
static char pin_buffer[PIN_MAX_LONGITUD + 1];
static uint8_t pin_idx;
static bool entrada_pin_activo;    //Estoy metiendo una clave si esta true
static uint32_t entrada_pin_expiro;
static code_action_t pin_funcion;    // si el buffer es para armar o desarmar

static bool_t sensor_pir_activo = FALSE;
static bool_t  sensor_magnetico_activo = FALSE;
static uint8_t sensores_activados = SENSOR_AMBOS; // default

static const char* estado_a_string(alarma_estado_t est) {
    switch (est) {
        case ALM_DESARMADO:      return "ALM_DESARMADO";
        case ALM_ARMANDO:        return "ALM_ARMANDO";
        case ALM_ARMADO:         return "ALM_ARMADO";
        case ALM_IDENTIFICACION: return "ALM_IDENTIFICACION";
        case ALM_ACTIVADO:       return "ALM_ACTIVADO";
        default:                 return "DESCONOCIDO";
    }
}


static bool TimerService_Elapsed(uint32_t start, uint32_t delay) {
   return ((int32_t)(tickRead() - start) >= (int32_t)delay);
}

/* ==== PIN ==== */
static void cancelar_pin_entrada(void) {
   entrada_pin_activo = false;
   pin_idx = 0;
   pin_buffer[0] = '\0';
}

/* helper: iniciar entrada de PIN */
static void pedido_entrada_PIN(code_action_t action, uint32_t timeout_ms) {
    entrada_pin_activo = true;
    pin_funcion = action;  //action es en funcion de lo que le pase la pagina o la pantalla, puede ser: CODE_ACTION_NONE = 0,CODE_ACTION_ARM,CODE_ACTION_DISARM
    pin_idx = 0;
    pin_buffer[0] = '\0';
    entrada_pin_expiro = tickRead() + timeout_ms; //tengo un tiempo para poner el pin
    if (action == CODE_ACTION_ARM) {//ambos casos se manejan en el MEF_UPDATE() donde se pregunta si hay entrada_pin_activo = true;
        uartWriteString(UART_USB, "Ingrese PIN para armar:\r\n");
    } else {
        uartWriteString(UART_USB, "Ingrese PIN para desarmar:\r\n");
    }
}


/* comparar PIN */
static bool check_pin(void) {
    return (strcmp(pin_buffer, pin_guardado) == 0); //compara el buffer que se va rellenando con el pin ingresado con el pin guardado
}

/* acciones a realizar cuando PIN OK seg�n el prop�sito */
static void pin_ingresado_correcto(void) { //PIN correcto
    if (pin_funcion == CODE_ACTION_ARM) {  //QUIERO ARMAR LA ALARMA
        estado = ALM_ARMANDO;
        t_estado = tickRead(); //obtengo el tiempo del sistema
        t_gracia = t_estado + ARM_DELAY_GRACIA_MS; //le sumo al tiempo del sistema el tiempo de gracia
        uartWriteString(UART_USB, "PIN correcto. Armando (30s)...\r\n");
    } 
    else if (pin_funcion == CODE_ACTION_DISARM) { //QUIERO DESARMAR LA ALARMA-> apagar sirena y volver a reposo
        estado = ALM_DESARMADO;
         t_estado = tickRead();
         gpioWrite(PIN_SIRENA, OFF); // Relay apagado
         uartWriteString(UART_USB, "PIN correcto. Sistema desarmado.\r\n");
    }
    cancelar_pin_entrada();
}

/* pin incorrecto mientras estamos en GRACIA: contar intentos */
static void pin_ingresado_fallo(void) { //PIN incorrecto
    uartWriteString(UART_USB, "PIN incorrecto\r\n");
    if (pin_funcion == CODE_ACTION_DISARM && 
       (estado == ALM_IDENTIFICACION || estado == ALM_ARMADO || estado == ALM_ARMANDO)) {
        
        intentos_clave++;
        
        char buf[64];
        snprintf(buf, sizeof(buf), "Intentos: %d/%d\r\n", intentos_clave, MAX_INTENTOS_CLAVE);
        uartWriteString(UART_USB, buf);
        
        cancelar_pin_entrada();
        
        if (intentos_clave >= MAX_INTENTOS_CLAVE) {
            // Se acabaron los intentos -> DISPARAR ALARMA
            estado = ALM_ACTIVADO;
            t_estado = tickRead();
            gpioWrite(PIN_SIRENA, ON); // Prender Sirena
            gpioWrite(PIN_BUZZER, ON); // Prender Buzzer
            uartWriteString(UART_USB, "Max intentos fallidos - Alarma ACTIVADA\r\n");
        }
    } else {
        // Si estaba desarmada o era otra acci�n, solo cancelamos
        cancelar_pin_entrada();
    }
}

void MEF_Alarm_ProcessPIN(const char *pin_ingresado) {
   char buffer[64];
  
   // --- DEBUG: IMPRIMIR PIN INGRESADO ---
    sprintf(buffer, "PIN ingresado (UI): [%s]\r\n", pin_ingresado);
    uartWriteString(UART_USB, buffer);
    // -------------------------------------
   
    // 1. Copiar el PIN del Keypad al buffer interno de la MEF
    strncpy(pin_buffer, pin_ingresado, PIN_MAX_LONGITUD);
    pin_buffer[PIN_MAX_LONGITUD] = '\0';

    // 2. Ejecutar la l�gica de verificaci�n (similar a presionar '#')
    if (check_pin()) {
        pin_ingresado_correcto();
       sprintf(buffer, "Buen pin.\r\n");
    } else {
        pin_ingresado_fallo();
       sprintf(buffer, "Mal pin.\r\n");
    }
    uartWriteString(UART_USB, buffer);
}

bool MEF_Alarm_VerificarPin(const char *pin_ingresado) {
    if (pin_ingresado == NULL) return false;
    // Compara el pin ingresado con el guardado
    if (strcmp(pin_ingresado, pin_guardado) == 0) {
        return true;
    }
    return false;
}

//todo esto al final no sirve xq es para el teclado fisico

/* 
static void process_input_chars(void) {
    char c;
    while (uartReadByte(UART_USB, &c)){ //saco un char de la cola
        // Normalizar ENTER (CR/LF) como # -> o sea confirmar PIN
        if (c == '\r' || c == '\n') c = '#';

        if (!entrada_pin_activo) { 
           //si no estoy en modo entrada PIN que solo lo solicitan las funciones MEF_Alarm_Pedido_Armar puedo ignorar
            // solo procesamos si entrada_pin_activo.
            continue; //no se si seria un break
        }

        // aceptamos s�lo '0'..'9', 'C' borrar, '#' confirmar, '*' cancelar
        if (c >= '0' && c <= '9') {
            if (pin_idx < PIN_MAX_LONGITUD) { //si pin_idx es menor que PIN_MAX_LONGITUD seguir metiendo caracteres
                pin_buffer[pin_idx++] = c;  //meto caracter
                pin_buffer[pin_idx] = '\0';
                uartWriteByte(UART_USB, '*'); // opcional: eco por UART (mostrar *) o char real
            }
            // si buffer ya lleno, ignorar siguientes d�gitos
        } else if (c == 'C' || c == 'c') { //con c o C hago un clear y borro todo el buffer
            // borrar
            pin_idx = 0;
           pin_buffer[0] = '\0';
           uartWriteString(UART_USB, "\r\nBorrado\r\n");
        } else if (c == '#') {   //esto era como un enter, para confirmar entrada
            uartWriteString(UART_USB, "\r\n");
            if (pin_idx == 0) { //si el pin_idx esta en 0 entonces no se escribio nada
                 uartWriteString(UART_USB, "PIN vacio\r\n");
                cancelar_pin_entrada();
                return;
            }
            if (check_pin()) {  //si el buffer tiene datos llamo a check pin que devuelve TRUE si es pin valido y FALSE si no
                pin_ingresado_correcto(); //el pin es correcto
            } else {
                pin_ingresado_fallo(); //el pin no es correcto
            }
        } else if (c == '*') { //* para cancelar el pin
            cancelar_pin_entrada();
            uartWriteString(UART_USB, "Entrada PIN cancelada\r\n");
        } else {
            // ignorar otros
        }
    }
}

*/

/* funciones p�blicas */

void MEF_Alarm_Init(void) {
    estado = ALM_DESARMADO;
    t_estado = tickRead();
    entrada_pin_activo = false;
    intentos_clave = 0;
   
    // --- INICIALIZACI�N---
    gpioInit(PIN_SIRENA, GPIO_OUTPUT); // Sirena
    gpioWrite(PIN_SIRENA, OFF);
   
    gpioInit(PIN_BUZZER, GPIO_OUTPUT);
    gpioWrite(PIN_BUZZER, OFF);

    // Configurar PIR como entrada
    gpioInit(PIN_PIR, GPIO_INPUT); 

    // Configurar Magn�tico
     gpioInit(PIN_MAGNETICO, GPIO_INPUT);
}

//ESTAS 3 FUNCIONES DE ACA ABAJO SON LAS LLAMADAS POR LA PANTALLA Y POR LA PAG WEB
//SE ENCARGAN DE ARMAR LA ALARMA, DESARMAR LA ALARMA Y CAMBIAR CONTRASE�A (PIN)
void MEF_Alarm_Pedido_Armar(void) {
    pedido_entrada_PIN(CODE_ACTION_ARM, 20000u); // dar 20s para teclear PIN
}

void MEF_Alarm_Pedido_Desarmar(void) {
    // inicia entrada de PIN para desarmar
    pedido_entrada_PIN(CODE_ACTION_DISARM, 20000u);
}

void MEF_Alarm_SetPIN(const char *pin) { //paso PIN por parametro, importante para actualizar
    if (pin == NULL) return;
    strncpy(pin_guardado, pin, PIN_MAX_LONGITUD);
    pin_guardado[PIN_MAX_LONGITUD] = '\0';
}

void MEF_Alarm_SetSensores(uint8_t sensores) { 
    sensores_activados = sensores;
    
    // debug
    char buf[64];
    uartWriteString(UART_USB, "--- DEBUG CONFIGURACION ---\r\n");
    sprintf(buf, "Valor recibido: %d\r\n", sensores);
    uartWriteString(UART_USB, buf);

    if (sensores_activados == SENSOR_MAGNETICO) {
        uartWriteString(UART_USB, "Modo Activo: SOLO MAGNETICO (Bit 0)\r\n");
    } 
    else if (sensores_activados == SENSOR_MOVIMIENTO) {
        uartWriteString(UART_USB, "Modo Activo: SOLO PIR (Bit 1)\r\n");
    } 
    else if (sensores_activados == SENSOR_AMBOS) {
        uartWriteString(UART_USB, "Modo Activo: AMBOS (Bits 0 y 1)\r\n");
    } 
    else {
        uartWriteString(UART_USB, "Modo Activo: DESCONOCIDO/ERROR\r\n");
    }
    uartWriteString(UART_USB, "---------------------------\r\n");
}

alarma_estado_t get_alarm_state(void) {
    return estado;
}

bool_t get_pir_active_status(void) {
    return sensor_pir_activo;
}

bool_t get_magnetico_active_status(void) {
    return sensor_magnetico_activo;
}

uint8_t MEF_Alarm_GetSensoresConfigurados(void) {
    return sensores_activados;
}

/* estado de la MEF: llamada en bucle principal */
void MEF_Alarm_Update(void) {
    uint32_t t_sistema = tickRead();
   
   //esto no afecta en nada, solo imprime el estado por uart
   static alarma_estado_t estado_anterior = -1; // Iniciamos en un valor inv�lido para que imprima la primera vez
    if (estado != estado_anterior) {
        char buffer_debug[64];
        snprintf(buffer_debug, sizeof(buffer_debug), ">> ESTADO ALARMA: %s\r\n", estado_a_string(estado));
        uartWriteString(UART_USB, buffer_debug);
        
        // Actualizamos el anterior para no volver a imprimir hasta que cambie de nuevo
        estado_anterior = estado;
    }


    switch (estado) {
    case ALM_DESARMADO:
        // indicadores: LED1 apagado
        gpioWrite(LEDR, OFF);
        gpioWrite(PIN_BUZZER, OFF);
        break;

    case ALM_ARMANDO:
       // Hacer sonar el Buzzer intermitente
        if ((t_sistema / 500) % 2) {
            gpioWrite(PIN_BUZZER, ON);
        } else {
            gpioWrite(PIN_BUZZER, OFF);
        }
        
      
        if (TimerService_Elapsed(t_gracia - ARM_DELAY_GRACIA_MS, ARM_DELAY_GRACIA_MS)) { // check t_sistema >= t_gracia
            if (TimerService_Elapsed(t_estado, ARM_DELAY_GRACIA_MS)) {
                estado = ALM_ARMADO;
                 t_estado = t_sistema;
                 uartWriteString(UART_USB, "Sistema ARMADO\r\n");
                 gpioWrite(PIN_BUZZER, OFF); //apago buzzer
                 gpioWrite(LEDR, ON);       //prendo led rojo
            }
        }
        break;

     case ALM_ARMADO: {
         gpioWrite(LEDR, ON);
         bool intruso = false;
        
        // 1. REVISION MAGNETICO
        if ((sensores_activados & SENSOR_MAGNETICO)) {
            // Solo entramos aqui si el magnetico esta habilitado
            if (gpioRead(PIN_MAGNETICO) == LOW) { 
                 uartWriteString(UART_USB, "DISPARO: Codigo detecto PIN_MAGNETICO en HIGH\r\n");
                 intruso = true;
            }
        }
         
        // 2. REVISION PIR
        if ((sensores_activados & SENSOR_MOVIMIENTO)) {
            // Solo entramos aqui si el PIR esta habilitado
            if (gpioRead(PIN_PIR) == HIGH) {
                 uartWriteString(UART_USB, "DISPARO: Codigo detecto PIN_PIR en HIGH\r\n");
                 intruso = true;
            }
        }

            if (intruso) {
                // Pasamos al estado de gracia para pedir PIN
                estado = ALM_IDENTIFICACION;
                t_estado = t_sistema;
                t_fin_gracia = t_sistema + INGRESANDO_PIN_DELAY;
                intentos_clave = 0;
                
                // Pedimos PIN para DESARMAR
                pedido_entrada_PIN(CODE_ACTION_DISARM, INGRESANDO_PIN_DELAY);
                
                uartWriteString(UART_USB, "Intrusion detectada: Ingrese PIN para cancelar.\r\n");
                gpioWrite(LEDG, ON); // Led Amarillo/Verde alerta
            }
        } break;

    case ALM_IDENTIFICACION:
       if (t_sistema % 1000 == 0) { 
        char buf[64];
        snprintf(buf, sizeof(buf), "Time: %d, Start: %d, Diff: %d\r\n", 
                 t_sistema, t_estado, (t_sistema - t_estado));
        uartWriteString(UART_USB, buf);
    }
        // Si tiempo expir� -> TRIGGERED
        if (TimerService_Elapsed(t_estado, INGRESANDO_PIN_DELAY)) { //veo si se venci� INGRESANDO_PIN_DELAY
            estado = ALM_ACTIVADO;
           t_estado = t_sistema;
           gpioWrite(PIN_SIRENA, ON);
           
           uartWriteString(UART_USB, "�TIEMPO TERMIN�! SIRENA ACTIVADA\r\n");
           gpioWrite(LEDG, OFF);
        }
        break;

    case ALM_ACTIVADO:
        // Sirena encendida, parpadeo de LED
        gpioWrite(PIN_SIRENA, ON);
    
        // parpadeo facil de led
        if ((t_sistema / 250) % 2) {
            gpioWrite(LEDG, ON);
            gpioWrite(PIN_BUZZER, ON); // Buzzer tambi�n suena
        } else {
            gpioWrite(LEDG, OFF);
            gpioWrite(PIN_BUZZER, OFF);
        }
        break;
    }
}

/*pedido_entrada_PIN() lanza la ventana para ingresar PIN para ARM o DISARM. La entrada de d�gitos la gestiona process_input_chars() (no bloqueante).

InputService_GetChar() lee desde UART (o keypad si lo implement�s). Si no te interesa UART para PIN, reemplaz� por lectura de keypad.

GPIOService_Set(RELAY1, true) activa la sirena ? adapta a tu pin real.

Las comprobaciones de temporizador usan TimerService_GetTick() y TimerService_Elapsed() para evitar bloqueos.*/