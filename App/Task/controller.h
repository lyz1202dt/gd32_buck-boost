#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include "pid.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CONTROLLER_STATE_INIT,
    CONTROLLER_STATE_IDLE,
    CONTROLLER_STATE_BUCK,
    CONTROLLER_STATE_BOOST,
    CONTROLLER_STATE_MIXTURE,
    CONTROLLER_STATE_ERROR
} controller_state_t;


typedef struct {
    controller_state_t state;
    uint32_t error_code;
    uint16_t max_pwm_ccr;
    uint16_t max_pwm_higher_ccr_value;  //单周期最大高边导通占空
    uint16_t max_higher_period_cnt;     //最大高边导通触发间隔的周期数

    float vol_in;
    float cur_in;
    float vol_out;
    float cur_out;
    bool measurements_initialized;

    int ctrl_mode;
    float vol_ref;
    float cur_ref;

    uint32_t update_cnt;

    pid_t pid;
} controller_t;

/* Initializes the controller and enters the setup state. */
void controller_init(controller_t *controller, uint16_t max_pwm_ccr,
                     uint16_t min_higher_ccr_value, uint16_t min_higher_period_cnt);

/* Measurements are in volts and amperes; subsequent samples use alpha=0.5 filtering. */
bool set_controller_state(controller_t *controller, float vol_in, float cur_in,
                          float vol_out, float cur_out);
/* Updates the two PWM compare values. */
bool controller_update(controller_t *controller,uint16_t *ch1, uint16_t *ch2);

uint32_t controller_get_error(const controller_t *controller);

#ifdef __cplusplus
}
#endif

#endif /* APP_CONTROLLER_H */
