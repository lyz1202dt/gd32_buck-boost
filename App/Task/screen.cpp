#include "screen.hpp"
#include "FreeRTOS.h"
#include "task.h"

void ScreenTask(void* param){
    TickType_t xPreviousWakeTime;
    xPreviousWakeTime=xTaskGetTickCount();
    while(1)
    {

        vTaskDelayUntil(&xPreviousWakeTime, pdMS_TO_TICKS(50));
    }
}
