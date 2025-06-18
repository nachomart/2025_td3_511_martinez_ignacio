#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "Semphr.h"
#include "Queue.h"

#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "lcd.h"

//  INCLUYO EL HELPER PARA PWM
#include "helper.h"


#define PIN_ENT_SIGNAL 2        //  UTILIZO GPIO2 COMO ENTRADA
#define PIN_SAL_SIGNAL 3        //  UTILIZO GPIO3 COMO SALIDA DEL PWM
#define PWM_FREQ 1000

#define PIN_I2C_SDA 4           // UTILIZO GP4 COMO I2C SDA
#define PIN_I2C_SCL 5           //  UTILIZO GP5 COMO I2C SCL

  
SemaphoreHandle_t semphrCounting;  // UTILIZO SEMAFORO PARA BLOQUEAR TAREA

// IRQ DEL CONTADOR GPIO
void contador_irq()
{
    static BaseType_t wake_higher_task = pdFALSE;
    xSemaphoreGiveFromISR(semphrCounting, &wake_higher_task);
    portYIELD_FROM_ISR(wake_higher_task);
}

// TAREA PARA IMPRIMIR FRECUENCIA
void task_MuestroCantidadDeFlancos(void *params)
{
    //DEFINO VARIABLE PARA CONTADOR

    uint16_t contador = 0;
    TickType_t ultimo_tick = xTaskGetTickCount();       //  VARIABLE PARA DELAY
    const TickType_t espera_1seg = pdMS_TO_TICKS(1000);  // 1 SEGUNDO DE DELAY

    char txt_text[17];
    char txt_num[17];

    while(1)
    {
        contador = uxSemaphoreGetCount(semphrCounting);   // ALMACENO EN contador EL CONTEO ACTUAL DEL SEMAFORO
        printf("La frencuencia es %d Hz \n", contador); // ESCRIBO EN PANTALLA

        sprintf(txt_text, "FRECUENCIA:");
        sprintf(txt_num, "%4d HZ", contador);

        // ENVIO LA TEXTO AL DISPLAY
        lcd_set_cursor(0, 0);
        lcd_string(txt_text);

        // ENVIO LA VALOR AL DISPLAY
        lcd_set_cursor(1, 0);
        lcd_string(txt_num);
   
        //  RESETEO EL CONTADOR Y SEMAFORO

        //  BLOQUEO POR 1 SEGUNDO
        vTaskDelayUntil(&ultimo_tick, espera_1seg);

        contador = 0;
        xQueueReset(semphrCounting);
    }
}



int main()
{
    stdio_init_all();

    //  INICIALIZO LOS GPIO

    gpio_init(PIN_ENT_SIGNAL);
    gpio_set_dir(PIN_ENT_SIGNAL, false);        //  SETEO COMO PIN DE ENTRADA
    gpio_pull_down(PIN_ENT_SIGNAL);          //  HABILITO RESISTENCIA PULL DOWN
    gpio_set_irq_enabled_with_callback(PIN_ENT_SIGNAL, GPIO_IRQ_EDGE_RISE, true, &contador_irq);      // HABILITO EL IRQ DEL PIN
    
    // GENERO PWM PARA CONTAR LOS FLANCOS
    pwm_user_init(PIN_SAL_SIGNAL,PWM_FREQ);

    // INICIO DE I2C
    i2c_init(i2c0, 100000);    // I2C0 (DEFAULT) a 100khz

    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);     // I2C0_SDA en GPIO4
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);     // I2C0_SCL en GPIO5

    gpio_pull_up(PIN_I2C_SDA);         // PONGO PIN SDA A PULLUP
    gpio_pull_up(PIN_I2C_SCL);         // PONGO PIN SCL A PULLUP

    // Inicializo LCD
    lcd_init(i2c0, 0x27);
    lcd_clear();

    //  CREO EL SEMAOFORO
    semphrCounting = xSemaphoreCreateCounting(1000,0);

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
