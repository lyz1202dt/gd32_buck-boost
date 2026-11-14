#include "controller.h"

#define SETUP_OUTPUT_MIN_V 1.0f
#define MODE_ENTER_MARGIN_V 0.5f
#define MODE_EXIT_MARGIN_V 0.25f

void controller_init(controller_t *controller, uint16_t max_pwm_ccr,
                     uint16_t min_higher_ccr_value, uint16_t min_higher_period_cnt)
{
    if (controller == 0) {
        return;
    }

    controller->error_code = 0U;
    controller->state = CONTROLLER_STATE_SETUP;
    controller->max_pwm_ccr = max_pwm_ccr;
    controller->min_higher_ccr_value = min_higher_ccr_value;
    controller->min_higher_period_cnt = min_higher_period_cnt;
    controller->ctrl_mode = 0;
    controller->vol_ref = 0.0f;
    controller->cur_ref = 0.0f;
}

bool set_controller_state(controller_t *controller, float vol_in, float cur_in,
                          float vol_out, float cur_out)
{
    controller->vol_in = vol_in;
    controller->cur_in = cur_in;
    controller->vol_out = vol_out;
    controller->cur_out = cur_out;
    return true;
}

bool controller_update(controller_t *controller, uint16_t *ch1, uint16_t *ch2)
{
    controller_state_t state=controller->state;

    if (state == CONTROLLER_STATE_ERROR) {
        *ch1=0;
        *ch2=0;
        return false;
    }

    if (state == CONTROLLER_STATE_INIT) {
        *ch1=0;
        *ch2=0;
    }
    if (state == CONTROLLER_STATE_SETUP) {
        
    }

    return true;
}

uint32_t controller_get_error(const controller_t *controller)
{
    return controller != 0 ? controller->error_code : 0U;
}
