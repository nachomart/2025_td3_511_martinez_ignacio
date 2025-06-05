#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "Semphr.h"
#include "Queue.h"

//  INCLUYO EL HELPER PARA PWM
#include "helper.h"


#define PIN_ENT_SIGNAL 2     //  UTILIZO GPIO2 COMO ENTRADA
#define PIN_SAL_SIGNAL 3     //  UTILIZO GPIO3 COMO SALIDA DEL PWM
#define PWM_FREQ 1000

  
SemaphoreHandle_t semphrCounting;  // UTILIZO SEMAFORO PARA BLOQUEAR TAREA

// TAREA PARA CONTAR FLANCOS MEDIANTE SEMAFORO
void task_CuentoFlancos(void *params)
{
    while (1)
    {
        if(gpio_get(PIN_SAL_SIGNAL))  // LEVANTO EL FLANCO Y ENTRO O NO AL IF

        {
            xSemaphoreGive(semphrCounting);     // LLAMO AL SEMAFORO
            while(gpio_get(PIN_SAL_SIGNAL));    //  SI CAMBIA EL FLANCO SALGO DEL IF
        }
    }
}

// TAREA PARA IMPRIMIR FRECUENCIA
void task_MuestroCantidadDeFlancos(void *params)
{
    //DEFINO VARIABLE PARA CONTADOR

    uint16_t contador = 0;
    TickType_t ultimo_tick = xTaskGetTickCount();       //  VARIABLE PARA DELAY
    const TickType_t espera_1seg = pdMS_TO_TICKS(1000);  // 1 SEGUNDO DE DELAY

    while(1)
    {
        contador = uxSemaphoreGetCount(semphrCounting);   // ALMACENO EN contador EL CONTEO ACTUAL DEL SEMAFORO
        printf("La frencuencia es %d Hz \n", contador); // ESCRIBO EN PANTALLA

        //  RESETEO EL CONTADOR Y SEMAFORO

        contador = 0;
        xQueueReset(semphrCounting);

        //  BLOQUEO POR 1 SEGUNDO
        vTaskDelayUntil(&ultimo_tick, espera_1seg);
    }
}



int main()
{
    stdio_init_all();

    //  INICIALIZO LOS GPIO

    gpio_init(PIN_ENT_SIGNAL);
    gpio_set_dir(PIN_ENT_SIGNAL, false);        //  SETEO COMO PIN DE ENTRADA
    gpio_pull_down(PIN_ENT_SIGNAL);          //  HABILITO RESISTENCIA PULL DOWN
    
    // GENERO PWM PARA CONTAR LOS FLANCOS
    pwm_user_init(PIN_SAL_SIGNAL,PWM_FREQ);

    //  CREO EL SEMAOFORO
    semphrCounting = xSemaphoreCreateCounting(1000,0);

    //  CREO LA TAREA DEL FRECUENCIMETRO
    xTaskCreate
    (
        task_CuentoFlancos,
        "Contador De Flancos",
        2*configMINIMAL_STACK_SIZE,
        NULL,
        1,
        NULL
    );

    //  CREO LA TAREA DE IMPRESION
    xTaskCreate
    (
        task_MuestroCantidadDeFlancos,
        "Imprimo Cantidad de Flancos",
        2*configMINIMAL_STACK_SIZE,
        NULL,
        2,
        NULL
    );

    // ARRANCO EL SCHEDULER

    vTaskStartScheduler();
    while(1);
}
