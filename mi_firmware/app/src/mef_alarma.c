#include "mef_alarma.h"
#include <string.h>
#include <stdio.h>

static alarma_estado_t estado;
static uint32_t t_sistema;
static uint32_t t_estado;           // tiempo estado
static uint32_t t_gracia;         // timestamp fin armado (ARMING)-> t_estado + ARM_DELAY_GRACIA_MS. Tiempo que doy a la persona para que pueda salir de la casa
static uint32_t t_fin_gracia;       // fin de ventana de gracia
static int intentos_clave;

static char pin_guardado[PIN_MAX_LONGITUD + 1] = "1234"; // PIN por defecto (editable)
static char pin_buffer[PIN_MAX_LONGITUD + 1];
static uint8_t pin_idx;
static bool entrada_pin_activo;    //Estoy metiendo una clave si esta true
static uint32_t entrada_pin_expiro;
static code_action_t pin_funcion;    // si el buffer es para armar o desarmar

static uint8_t sensores_activados = SENSOR_AMBOS; // default

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
        // pasar a ARMING
        estado = ALM_ARMANDO;
        t_estado = tickRead(); //obtengo el tiempo del sistema
        t_gracia = t_estado + ARM_DELAY_GRACIA_MS; //le sumo al tiempo del sistema el tiempo de gracia
        uartWriteString(UART_USB, "PIN correcto. Armando (30s)...\r\n");
    } 
    else if (pin_funcion == CODE_ACTION_DISARM) { //QUIERO DESARMAR LA ALARMA-> apagar sirena y volver a reposo
        estado = ALM_DESARMADO;
         t_estado = tickRead();
         gpioWrite(GPIO0, OFF); // Relay apagado
         uartWriteString(UART_USB, "PIN correcto. Sistema desarmado.\r\n");
    }
    cancelar_pin_entrada();
}

/* pin incorrecto mientras estamos en GRACIA: contar intentos */
static void pin_ingresado_fallo(void) { //PIN incorrecto
    uartWriteString(UART_USB, "PIN incorrecto\r\n");
    if (estado == ALM_IDENTIFICACION && pin_funcion == CODE_ACTION_DISARM) {
        intentos_clave++;
        char buf[64];
        snprintf(buf, sizeof(buf), "Intentos: %d/%d\r\n", intentos_clave, MAX_INTENTOS_CLAVE);
        uartWriteString(UART_USB, buf);
        cancelar_pin_entrada();
        if (intentos_clave >= MAX_INTENTOS_CLAVE) {
            // activar alarma inmediatamente
            estado = ALM_ACTIVADO;
            t_estado = tickRead();
            gpioWrite(GPIO0, ON); // Sirena ON
            uartWriteString(UART_USB, "Max intentos - Alarma ACTIVADA\r\n");
        }
    } else {
        // otra situaci�n: solo cancelar entrada
        cancelar_pin_entrada();
    }
}

/* procesa caracteres entrantes para PIN (no bloqueante) */
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

/* funciones p�blicas */

void MEF_Alarm_Init(void) {
    estado = ALM_DESARMADO;
    t_estado = tickRead();
    entrada_pin_activo = false;
    intentos_clave = 0;
   
   /*
    gpioWrite(GPIO0, OFF); // Sirena OFF
      gpioWrite(LEDR, OFF);
      gpioWrite(LEDG, OFF);
   */
}

//ESTAS 3 FUNCIONES DE ACA ABAJO SON LAS LLAMADAS POR LA PANTALLA Y POR LA PAG WEB
//SE ENCARGAN DE ARMAR LA ALARMA, DESARMAR LA ALARMA Y CAMBIAR CONTRASE�A (PIN)
void MEF_Alarm_Pedido_Armar(void) {
    // inicia entrada de PIN para armar
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

void MEF_Alarm_SetSensores(uint8_t sensores) { //la pagina o la pantalla eligen que sensores se usan, magnetico, movimiento o los 2
    sensores_activados = sensores;
}

/* estado de la MEF: llamada en bucle principal */
void MEF_Alarm_Update(void) {
    uint32_t t_sistema = tickRead();

    /* procesar entrada de caracteres si existe */
    process_input_chars();

    /* si hay entrada de PIN activa y expir� -> reacci�n seg�n acci�n */
    //TODA ESTA LOGICA ES PARA MANEJAR AL MOMENTO EN QUE ENTRAS A TU CASA Y PASAS DE ESTADO ACTIVADO A IDENTIFICACION. SI PASAN LOS 60SEG PASAS A SONANDO
    if(entrada_pin_activo && (int32_t)(t_sistema - entrada_pin_expiro) >= 0) {
      uartWriteString(UART_USB, "Tiempo para ingresar PIN expirado\r\n");
      if(pin_funcion == CODE_ACTION_DISARM && estado == ALM_IDENTIFICACION) {
         estado = ALM_ACTIVADO;
         t_estado = t_sistema;
         gpioWrite(GPIO0, ON);
         uartWriteString(UART_USB, "Ventana expiro - Alarma ACTIVADA\r\n");
      }
         cancelar_pin_entrada();
      }
 

    switch (estado) {
    case ALM_DESARMADO:
        // indicadores: LED1 apagado
        gpioWrite(LEDR, OFF);
        // nada mas; MEF_Alarm_Pedido_Armar() inicia PIN entry para armar
        break;

    case ALM_ARMANDO:
        // Mostrar LED intermitente o fijo (indicamos armado pr�ximo)
        // al finalizar el tiempo pasamos a ARMED
        if (TimerService_Elapsed(t_gracia - ARM_DELAY_GRACIA_MS, ARM_DELAY_GRACIA_MS)) { // check t_sistema >= t_gracia
            if (TimerService_Elapsed(t_estado, ARM_DELAY_GRACIA_MS)) {
                estado = ALM_ARMADO;
                 t_estado = t_sistema;
                 uartWriteString(UART_USB, "Sistema ARMADO\r\n");
                 gpioWrite(LEDR, ON);//prendo sensores magneticos y de movimiento
            }
        }
        break;

     case ALM_ARMADO: {
         gpioWrite(LEDR, ON);
         bool intruso = false;
        if((sensores_activados & SENSOR_MAGNETICO) &&
           (gpioRead(TEC1) || gpioRead(TEC2))) {
           intruso = true;
        }
        if((sensores_activados & SENSOR_MOVIMIENTO) &&
           gpioRead(TEC3)) {
           intruso = true;
        }
        if(intruso) {
           estado = ALM_IDENTIFICACION;
           t_estado = t_sistema;
           t_fin_gracia = t_sistema + INGRESANDO_PIN_DELAY;
           intentos_clave = 0;
           pedido_entrada_PIN(CODE_ACTION_DISARM, INGRESANDO_PIN_DELAY);
           uartWriteString(UART_USB, "Intrusion detectada: 60s para desarmar\r\n");
           gpioWrite(LEDG, ON);
        }
     } break;

    case ALM_IDENTIFICACION:
        // Si PIN correct� se manej� ya en pin_ingresado_correcto -> IDLE
        // Si tiempo expir� -> TRIGGERED
        if (TimerService_Elapsed(t_estado, INGRESANDO_PIN_DELAY)) { //veo si se venci� INGRESANDO_PIN_DELAY
            estado = ALM_ACTIVADO;
           t_estado = t_sistema;
           gpioWrite(GPIO0, ON);
           uartWriteString(UART_USB, "Tiempo expiro - ALARMA ACTIVADA\r\n");
           gpioWrite(LEDG, OFF);
        }
        break;

    case ALM_ACTIVADO:
        // Sirena encendida, parpadeo de LED
        // implemento un parpadeo sencillo usando ms tick
        if ((t_sistema / 500) & 1) {
            gpioWrite(LEDG, ON);
        } else {
            gpioWrite(LEDG, OFF);
        }
        // si el PIN fue ingresado correctamente en process_input_chars() -> pasa a IDLE. 
        //Esta logica esta implementada en MEF_Alarm_Pedido_Desarmar
        break;
    }
}

/*pedido_entrada_PIN() lanza la ventana para ingresar PIN para ARM o DISARM. La entrada de d�gitos la gestiona process_input_chars() (no bloqueante).

InputService_GetChar() lee desde UART (o keypad si lo implement�s). Si no te interesa UART para PIN, reemplaz� por lectura de keypad.

GPIOService_Set(RELAY1, true) activa la sirena ? adapta a tu pin real.

Las comprobaciones de temporizador usan TimerService_GetTick() y TimerService_Elapsed() para evitar bloqueos.*/