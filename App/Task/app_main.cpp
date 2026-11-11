#include "app_main.hpp"

#include "FreeRTOS.h"
#include "screen.hpp"
#include "task.h"

int app_main(void* param)
{
    (void)param;
    return xTaskCreate(ScreenTask, "screen", 512, nullptr, tskIDLE_PRIORITY + 1U, nullptr) == pdPASS
               ? 0
               : -1;
}
