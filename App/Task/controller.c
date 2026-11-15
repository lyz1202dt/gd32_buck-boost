#include "controller.h"
#include "pid.h"

#define SETUP_OUTPUT_MIN_V       1.0f
#define MODE_ENTER_MARGIN_V      0.5f
#define MODE_EXIT_MARGIN_V       0.25f
#define MEASUREMENT_FILTER_ALPHA 0.5f

void controller_init(
    controller_t* controller, uint16_t max_pwm_ccr, uint16_t min_higher_ccr_value,
    uint16_t min_higher_period_cnt) {
    if (controller == 0) {
        return;
    }

    controller->error_code               = 0U;
    controller->state                    = CONTROLLER_STATE_IDLE;
    controller->max_pwm_ccr              = max_pwm_ccr;
    controller->max_pwm_higher_ccr_value = min_higher_ccr_value;
    controller->max_higher_period_cnt    = min_higher_period_cnt;
    controller->ctrl_mode                = 0;
    controller->vol_ref                  = 0.0f;
    controller->cur_ref                  = 0.0f;
    controller->measurements_initialized = false;

    controller->state = CONTROLLER_STATE_IDLE;

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
    config.output_limit   = 0.98f;
    pid_init(&controller->pid, &config);
    pid_reset(&controller->pid);
}

bool set_controller_state(
    controller_t* controller, float vol_in, float cur_in, float vol_out, float cur_out) {
    if (controller == 0) {
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
    controller->update_cnt++;
    controller_state_t state = controller->state;
    int ctrl_mode            = controller->ctrl_mode;

    if (state == CONTROLLER_STATE_ERROR) {
        *ch1 = 0;
        *ch2 = 0;
        return false;
    }

    //是否使能控制
    if (ctrl_mode == 0)
    {
        state = CONTROLLER_STATE_IDLE;
        *ch1  = 0;
        *ch2  = 0;
    } else if (ctrl_mode == 1) // 不管如何先进入BUCK模式使输出端的栅极驱动芯片触发启动
    {
        if (state == CONTROLLER_STATE_IDLE || state == CONTROLLER_STATE_INIT ||
            !controller->measurements_initialized || controller->vol_out < SETUP_OUTPUT_MIN_V) {
            state = CONTROLLER_STATE_BUCK;
        } else {
            float voltage_margin = controller->vol_in - controller->vol_ref;
            if (state == CONTROLLER_STATE_BUCK && voltage_margin > MODE_EXIT_MARGIN_V) {
                state = CONTROLLER_STATE_BUCK;
            } else if (state == CONTROLLER_STATE_BOOST &&
                       voltage_margin < -MODE_EXIT_MARGIN_V) {
                state = CONTROLLER_STATE_BOOST;
            } else if (voltage_margin >= MODE_ENTER_MARGIN_V) {
                state = CONTROLLER_STATE_BUCK;
            } else if (voltage_margin <= -MODE_ENTER_MARGIN_V) {
                state = CONTROLLER_STATE_BOOST;
            } else {
                state = CONTROLLER_STATE_MIXTURE;
            }
        }
    }

    controller->state = state;
    if (state == CONTROLLER_STATE_BUCK) {
        *ch1         = controller->max_pwm_higher_ccr_value;
        float output = 0;
        pid_update(&controller->pid, controller->vol_ref, controller->vol_out, &output);
        if (output < 0.0f)
            output = 0.0f;
        *ch2 = (uint16_t)(output * controller->max_pwm_ccr);
    } else if (state == CONTROLLER_STATE_BOOST) {
        *ch2         = controller->max_pwm_higher_ccr_value;
        float output = 0;
        pid_update(&controller->pid, controller->vol_ref, controller->vol_out, &output);
        if (output < 0.0f)
            output = 0.0f;
        *ch1 = (uint16_t)(output * controller->max_pwm_ccr);
    } else if (state == CONTROLLER_STATE_MIXTURE) {
        float boost_weight = (controller->vol_ref - controller->vol_in +
                              MODE_EXIT_MARGIN_V) / (2.0f * MODE_EXIT_MARGIN_V);
        float output = 0.0f;

        if (boost_weight < 0.0f)
            boost_weight = 0.0f;
        else if (boost_weight > 1.0f)
            boost_weight = 1.0f;

        // Match the pure-mode CCRs at the hysteresis entry points.
        boost_weight = boost_weight * boost_weight * (3.0f - 2.0f * boost_weight);

        if (!pid_update(&controller->pid, controller->vol_ref, controller->vol_out, &output)) {
            *ch1 = 0;
            *ch2 = 0;
            return false;
        }
        if (output < 0.0f)
            output = 0.0f;
        else if (output > 1.0f)
            output = 1.0f;

        float active_ccr = output * controller->max_pwm_ccr;
        float idle_ccr = controller->max_pwm_higher_ccr_value;
        *ch1 = (uint16_t)(idle_ccr + boost_weight * (active_ccr - idle_ccr));
        *ch2 = (uint16_t)(active_ccr + boost_weight * (idle_ccr - active_ccr));
    } else if (state == CONTROLLER_STATE_IDLE) {
        *ch1 = 0;
        *ch2 = 0;
    }
    return true;
}

uint32_t controller_get_error(const controller_t* controller) {
    return controller != 0 ? controller->error_code : 0U;
}
