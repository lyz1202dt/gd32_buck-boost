#include "controller.h"
#include "pid.h"
#include <math.h>

#define MEASUREMENT_FILTER_ALPHA  0.5f
#define OUTPUT_HIGH_SIDE_MIN_DUTY 0.05f
#define PI_OUTPUT_LIMIT           2.0f
#define PI_STEP_LIMIT             0.08f
#define MODE_CROSSOVER_BAND        0.02f

static float clamp(float value, float lower, float upper)
{
    if (value < lower) {
        return lower;
    }
    if (value > upper) {
        return upper;
    }
    return value;
}

static uint16_t controller_compare_from_duty(float duty, uint16_t max_pwm_ccr)
{
    const float bounded = clamp(duty, 0.0f, 1.0f);
    return (uint16_t)(bounded * (float)max_pwm_ccr + 0.5f);
}

void controller_init(
    controller_t* controller, uint16_t max_pwm_ccr, uint16_t max_higher_ccr_value,
    uint16_t min_higher_period_cnt) {
    if (controller == 0) {
        return;
    }

    controller->error_code               = 0U;
    controller->state                    = CONTROLLER_STATE_IDLE;
    controller->max_pwm_ccr              = max_pwm_ccr;
    controller->max_pwm_higher_ccr_value = max_higher_ccr_value;
    controller->max_higher_period_cnt    = min_higher_period_cnt;
    controller->ctrl_mode                = 0;
    controller->vol_ref                  = 0.0f;
    controller->cur_ref                  = 0.0f;
    controller->vol_in                   = 0.0f;
    controller->cur_in                   = 0.0f;
    controller->vol_out                  = 0.0f;
    controller->cur_out                  = 0.0f;
    controller->measurements_initialized = false;
    controller->update_cnt                = 0U;

    controller->state = CONTROLLER_STATE_IDLE;

    /* Voltage error is in V; PI gains convert it to dimensionless duty. */
    pid_config_t config;
    config.d_limit        = 0.0f;
    config.i_limit        = 0.9f;
    config.p_limit        = 0.9f;
    config.integral_limit = 0.9f;
    config.kp             = 0.5f;
    config.ki             = 0.5f;
    config.kd             = 0.0f;
    config.sample_time    = 0.0001f;
    config.mode           = PID_INCREMENTAL;
    config.output_limit   = PI_OUTPUT_LIMIT;
    pid_init(&controller->pid, &config);
    pid_reset(&controller->pid);
}

bool set_controller_state(
    controller_t* controller, float vol_in, float cur_in, float vol_out, float cur_out) {
    if (controller == 0 || !isfinite(vol_in) || !isfinite(cur_in) ||
        !isfinite(vol_out) || !isfinite(cur_out) || vol_in < 0.0f || vol_out < 0.0f) {
        return false;
    }

    if (!controller->measurements_initialized) {
        controller->vol_in                   = vol_in;
        controller->cur_in                   = cur_in;
        controller->vol_out                  = vol_out;
        controller->cur_out                  = cur_out;
        controller->measurements_initialized = true;
        return true;
    }

    controller->vol_in += MEASUREMENT_FILTER_ALPHA * (vol_in - controller->vol_in);
    controller->cur_in += MEASUREMENT_FILTER_ALPHA * (cur_in - controller->cur_in);
    controller->vol_out += MEASUREMENT_FILTER_ALPHA * (vol_out - controller->vol_out);
    controller->cur_out += MEASUREMENT_FILTER_ALPHA * (cur_out - controller->cur_out);
    return true;
}

bool controller_update(controller_t* controller, uint16_t* ch1, uint16_t* ch2) {
    if (controller == 0 || ch1 == 0 || ch2 == 0) {
        return false;
    }

    controller->update_cnt++;
    *ch1 = 0U;
    *ch2 = 0U;
    if (controller->state == CONTROLLER_STATE_ERROR) {
        return false;
    }
    if (controller->ctrl_mode == 0) {
        controller->state = CONTROLLER_STATE_IDLE;
        pid_reset(&controller->pid);
        return true;
    }
    if (controller->ctrl_mode != 1 || controller->max_pwm_ccr == 0U ||
        controller->max_pwm_higher_ccr_value == 0U ||
        controller->max_pwm_higher_ccr_value > controller->max_pwm_ccr) {
        controller->state = CONTROLLER_STATE_ERROR;
        return false;
    }
    if (!controller->measurements_initialized || !isfinite(controller->vol_ref) ||
        controller->vol_ref <= 0.0f) {
        controller->state = CONTROLLER_STATE_IDLE;
        pid_reset(&controller->pid);
        return false;
    }

    const float high_duty = (float)controller->max_pwm_higher_ccr_value /
                            (float)controller->max_pwm_ccr;
    if (high_duty <= OUTPUT_HIGH_SIDE_MIN_DUTY) {
        controller->state = CONTROLLER_STATE_ERROR;
        return false;
    }

    /* u is a dimensionless actuator command shared by both power stages. */
    const float max_command = 2.0f * high_duty - OUTPUT_HIGH_SIDE_MIN_DUTY;
    float output;
    if (!pid_update_bounded(&controller->pid, controller->vol_ref, controller->vol_out,
                            0.0f, max_command, PI_STEP_LIMIT, &output)) {
        controller->state = CONTROLLER_STATE_ERROR;
        return false;
    }

    /* State reports the crossover region; it does not select a PWM formula. */
    if (output < high_duty - MODE_CROSSOVER_BAND) {
        controller->state = CONTROLLER_STATE_BUCK;
    } else if (output > high_duty + MODE_CROSSOVER_BAND) {
        controller->state = CONTROLLER_STATE_BOOST;
    } else {
        controller->state = CONTROLLER_STATE_MIXTURE;
    }

    /* CH1 is the input high side; CH2 is the output high side. */
    const float input_high_duty = fminf(output, high_duty);
    const float output_high_duty = clamp(2.0f * high_duty - output,
                                         OUTPUT_HIGH_SIDE_MIN_DUTY, high_duty);
    *ch1 = controller_compare_from_duty(input_high_duty, controller->max_pwm_ccr);
    *ch2 = controller_compare_from_duty(output_high_duty, controller->max_pwm_ccr);
    return true;
}

uint32_t controller_get_error(const controller_t* controller) {
    return controller != 0 ? controller->error_code : 0U;
}
