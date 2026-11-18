#include "controller.h"
#include "pid.h"
#include <math.h>

#define MEASUREMENT_FILTER_ALPHA 0.5f

#define CTRL_MODE_DEAD_AREA 1.0f // Buck 边界为 1V，Boost 边界基于此再偏移 0.1V

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
    controller->update_cnt               = 0U;

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
    config.mode           = PID_POSITIONAL;
    config.output_limit   = 10.0f;
    pid_init(&controller->buck_pid, &config);
    pid_init(&controller->boost_pid, &config);
    pid_init(&controller->bb1_pid, &config);
    pid_init(&controller->bb2_pid, &config);
}

bool set_controller_state(
    controller_t* controller, float vol_in, float cur_in, float vol_out, float cur_out) {

    if (!controller->measurements_initialized) {
        controller->vol_in                   = vol_in;
        controller->cur_in                   = cur_in;
        controller->vol_out                  = vol_out;
        controller->cur_out                  = cur_out;
        controller->measurements_initialized = true;
        return true;
    }

    controller->vol_in  += MEASUREMENT_FILTER_ALPHA * (vol_in - controller->vol_in);
    controller->cur_in  += MEASUREMENT_FILTER_ALPHA * (cur_in - controller->cur_in);
    controller->vol_out += MEASUREMENT_FILTER_ALPHA * (vol_out - controller->vol_out);
    controller->cur_out += MEASUREMENT_FILTER_ALPHA * (cur_out - controller->cur_out);
    return true;
}

bool controller_update(controller_t* controller, uint16_t* ch1, uint16_t* ch2) {
    controller->update_cnt++;
    if (controller->ctrl_mode == 0) {
        controller->state = CONTROLLER_STATE_IDLE;
        pid_reset(&controller->bb1_pid);
        pid_reset(&controller->bb2_pid);
        pid_reset(&controller->buck_pid);
        pid_reset(&controller->boost_pid);
        *ch1 = 0;
        *ch2 = 0;
        return true;
    }

    const controller_state_t previous_state = controller->state;
    const float hysteresis = 0.1f;
    const float voltage_diff = controller->vol_in - controller->vol_out;
    const float buck_boundary = CTRL_MODE_DEAD_AREA +
        (controller->state == CONTROLLER_STATE_BUCK ? -hysteresis :
         controller->state == CONTROLLER_STATE_IDLE ? 0.0f : hysteresis);
    const float middle_boundary = 0.1f +
        (controller->state == CONTROLLER_STATE_BUCK ||
         controller->state == CONTROLLER_STATE_MIXTURE1 ? -hysteresis :
         controller->state == CONTROLLER_STATE_IDLE ? 0.0f : hysteresis);
    const float boost_boundary = -CTRL_MODE_DEAD_AREA - 0.1f +
        (controller->state == CONTROLLER_STATE_BOOST ? hysteresis :
         controller->state == CONTROLLER_STATE_IDLE ? 0.0f : -hysteresis);

    if (voltage_diff >= buck_boundary) {
        controller->state = CONTROLLER_STATE_BUCK;
    } else if (voltage_diff >= middle_boundary) {
        controller->state = CONTROLLER_STATE_MIXTURE1;
    } else if (voltage_diff >= boost_boundary) {
        controller->state = CONTROLLER_STATE_MIXTURE2;
    } else {
        controller->state = CONTROLLER_STATE_BOOST;
    }

    pid_t* active_pid;
    if (controller->state == CONTROLLER_STATE_BUCK) {
        active_pid = &controller->buck_pid;
    } else if (controller->state == CONTROLLER_STATE_BOOST) {
        active_pid = &controller->boost_pid;
    } else if (controller->state == CONTROLLER_STATE_MIXTURE1) {
        active_pid = &controller->bb1_pid;
    } else {
        active_pid = &controller->bb2_pid;
    }
    if (controller->state != previous_state) {
        pid_reset(active_pid);
    }

    float correction;
    if (!pid_update(active_pid, controller->vol_ref, controller->vol_out, &correction)) {
        controller->state = CONTROLLER_STATE_ERROR;
        *ch1 = 0U;
        *ch2 = 0U;
        return false;
    }

    const uint16_t max_ccr = controller->max_pwm_higher_ccr_value < controller->max_pwm_ccr ?
        controller->max_pwm_higher_ccr_value : controller->max_pwm_ccr;
    const float max_duty = (float)max_ccr / (float)controller->max_pwm_ccr;
    float duty1;
    float duty2;
    if (controller->state == CONTROLLER_STATE_BUCK) {
        /* Article's 4% boost-side low-side pulse means 96% on CH2 high side. */
        duty2 = max_duty;
        duty1 = duty2 * controller->vol_ref / controller->vol_in + correction;
    } else if (controller->state == CONTROLLER_STATE_BOOST) {
        duty1 = max_duty;
        duty2 = duty1 * controller->vol_in / controller->vol_ref - correction;
    } else if (controller->state == CONTROLLER_STATE_MIXTURE1) {
        duty2 = 0.8f;
        duty1 = duty2 * controller->vol_ref / controller->vol_in + correction;
    } else {
        duty1 = 0.8f;
        duty2 = duty1 * controller->vol_in / controller->vol_ref - correction;
    }

    duty1 = fminf(fmaxf(duty1, 0.0f), max_duty);
    duty2 = fminf(fmaxf(duty2, 0.0f), max_duty);
    *ch1 = (uint16_t)(duty1 * (float)controller->max_pwm_ccr);
    *ch2 = (uint16_t)(duty2 * (float)controller->max_pwm_ccr);
    return true;
}
