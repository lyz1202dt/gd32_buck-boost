#include "app_main.hpp"

#include <cstdint>

#include "FreeRTOS.h"
#include "task.h"

extern "C" {

volatile std::uint32_t g_freertos_task1_count = 0U;
volatile std::uint32_t g_freertos_task2_count = 0U;
volatile std::uint32_t g_freertos_stack_overflow = 0U;

void vApplicationStackOverflowHook(TaskHandle_t task, char* task_name)
{
    (void)task;
    (void)task_name;

    g_freertos_stack_overflow = 1U;
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}

}

namespace {

void task1_entry(void* param)
{
    (void)param;

    for (;;) {
        ++g_freertos_task1_count;
        vTaskDelay(pdMS_TO_TICKS(100U));
    }
}

void task2_entry(void* param)
{
    (void)param;

    for (;;) {
        ++g_freertos_task2_count;
        vTaskDelay(pdMS_TO_TICKS(250U));
    }
}

}  // namespace

int app_main(void* param)
{
    (void)param;

    TaskHandle_t task1_handle = nullptr;
    if (xTaskCreate(task1_entry,
                    "task1",
                    configMINIMAL_STACK_SIZE,
                    nullptr,
                    tskIDLE_PRIORITY + 1U,
                    &task1_handle) != pdPASS) {
        return -1;
    }

    if (xTaskCreate(task2_entry,
                    "task2",
                    configMINIMAL_STACK_SIZE,
                    nullptr,
                    tskIDLE_PRIORITY + 1U,
                    nullptr) != pdPASS) {
        vTaskDelete(task1_handle);
        return -2;
    }

    vTaskStartScheduler();

    return -3;
}
