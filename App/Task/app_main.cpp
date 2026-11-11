#include "app_main.hpp"

#include <cstdint>

#include "FreeRTOS.h"
#include "OLED.h"
#include "task.h"

int app_main(void* param)
{
    (void)param;
    OLED_Init();
    OLED_ShowString(0, 0, "GD32 OLED", OLED_8X16);
    OLED_Update();
    return 0;
}
