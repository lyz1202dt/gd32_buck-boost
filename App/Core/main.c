#include "app_main.hpp"
#include "FreeRTOS.h"
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

    app_main(NULL);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void halt(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}
