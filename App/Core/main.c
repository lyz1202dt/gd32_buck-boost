#include "app_main.hpp"
#include "FreeRTOS.h"
#include "gd32f30x_rcu.h"
#include "task.h"

static void init_task(void* param);
static void halt(void);

int main(void)
{
    if (xTaskCreate(init_task,
                    "app_main",
                    configMINIMAL_STACK_SIZE,
                    NULL,
                    tskIDLE_PRIORITY + 1U,
                    NULL) != pdPASS) {
        halt();
    }

    vTaskStartScheduler();

    halt();
}

static void init_task(void* param)
{
    (void)param;
    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ,GPIO_PIN_1|GPIO_PIN_2);
    gpio_bit_write(GPIOA,GPIO_PIN_1,SET);
    app_main(NULL);

    for (;;) {
        gpio_bit_write(GPIOA,GPIO_PIN_1,RESET);
        gpio_bit_write(GPIOA,GPIO_PIN_2,SET);
        vTaskDelay(pdMS_TO_TICKS(300));
        gpio_bit_write(GPIOA,GPIO_PIN_1,SET);
        gpio_bit_write(GPIOA,GPIO_PIN_2,RESET);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

static void halt(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}
