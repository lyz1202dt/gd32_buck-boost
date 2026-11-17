#include "controller.h"
#include "pid.h"
#include <math.h>

#define MEASUREMENT_FILTER_ALPHA 0.5f

#define CTRL_MODE_DEAD_AREA 1.0 // 过渡区域电压范围为正负1V

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
    config.mode           = PID_POSITIONAL;
    config.output_limit   = 10.0f;
    pid_init(&controller->pid, &config);
    pid_reset(&controller->pid);
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
        pid_reset(&controller->pid);
        *ch1 = 0;
        *ch2 = 0;
        return true;
    }

    controller->state=CONTROLLER_STATE_RUN;
    float output = 0.0f;
    pid_update(&controller->pid, controller->vol_ref, controller->vol_out, &output);
    //float d1d2_rate=controller->vol_ref/controller->vol_in;  //开关管占空比比值
    //const float dc=0.5f;    //参考中心量
    // const float dff=0.5f*(controller->vol_ref-controller->vol_in)/(controller->vol_ref+controller->vol_in);
    // float d1=dc+dff+output;
    // float d2=dc-dff-output;
    // *ch1 = (uint16_t)(d1 * controller->max_pwm_ccr);
    // *ch2 = (uint16_t)(d2 * controller->max_pwm_ccr);
    const float max_paw_rate=0.98f;

    //TODO:电压差从-1~1，映射成w
    float diff_vol=controller->vol_in-controller->vol_out;
    float t = fminf(fabsf(diff_vol) / CTRL_MODE_DEAD_AREA, 1.0f);
    float w = t * t * (3.0f - 2.0f * t);

    float a=controller->vol_out/(controller->vol_in+controller->vol_out);
    float b=controller->vol_in/(controller->vol_in+controller->vol_out);

    float s_max=1.0f;
    if(controller->vol_out>controller->vol_in)
        s_max=max_paw_rate/a;
    else
        s_max=max_paw_rate/b;
    float s=1.0f+w*(s_max-1.0f);
    float d1ff=s*a+output;
    float d2ff=s*b-output;

    //TODO:限制数字的范围

    *ch1 = (uint16_t)(d1ff * controller->max_pwm_ccr);
    *ch2 = (uint16_t)(d2ff * controller->max_pwm_ccr);
    return true;
}
