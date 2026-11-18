#ifndef APP_BUTTONS_H
#define APP_BUTTONS_H

#include "FreeRTOS.h"
#include "queue.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BUTTON_SW6,
    BUTTON_SW7,
    BUTTON_SW8,
    BUTTON_SW9
} button_id_t;

typedef enum {
    BUTTON_PRESSED,
    BUTTON_RELEASED
} button_action_t;

typedef struct {
    button_id_t button;
    button_action_t action;
} button_event_t;

bool buttons_init(void);
QueueHandle_t buttons_event_queue(void);
void ButtonsTask(void *param);

#ifdef __cplusplus
}
#endif

#endif /* APP_BUTTONS_H */
