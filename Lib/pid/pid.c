#include "pid.h"

#include <math.h>
#include <stddef.h>

static float pid_clamp(float value, float limit)
{
    if (limit > 0.0f) {
        if (value > limit) {
            return limit;
        }
        if (value < -limit) {
            return -limit;
        }
    }
    return value;
}

static bool pid_valid_config(const pid_config_t *config)
{
    return config != NULL &&
           (config->mode == PID_POSITIONAL ||
            config->mode == PID_DERIVATIVE_ON_MEASUREMENT ||
            config->mode == PID_INCREMENTAL ||
            config->mode == PID_EXTERNAL_VELOCITY) &&
           isfinite(config->kp) && isfinite(config->ki) && isfinite(config->kd) &&
           isfinite(config->sample_time) && config->sample_time > 0.0f &&
           isfinite(config->p_limit) && config->p_limit >= 0.0f &&
           isfinite(config->i_limit) && config->i_limit >= 0.0f &&
           isfinite(config->d_limit) && config->d_limit >= 0.0f &&
           isfinite(config->integral_limit) && config->integral_limit >= 0.0f &&
           isfinite(config->output_limit) && config->output_limit >= 0.0f;
}

bool pid_init(pid_t *pid, const pid_config_t *config)
{
    if (pid == NULL || !pid_valid_config(config)) {
        return false;
    }

    pid->config = *config;
    pid_reset(pid);
    return true;
}

void pid_reset(pid_t *pid)
{
    if (pid == NULL) {
        return;
    }

    pid->integral = 0.0f;
    pid->previous_error = 0.0f;
    pid->previous_measurement = 0.0f;
    pid->p_term = 0.0f;
    pid->i_term = 0.0f;
    pid->d_term = 0.0f;
    pid->output = 0.0f;
    pid->initialized = false;
}

static bool pid_update_internal(pid_t *pid, float setpoint, float measurement,
                                float estimated_velocity, bool has_velocity, float *output)
{
    if (pid == NULL || output == NULL || !isfinite(setpoint) || !isfinite(measurement) ||
        (has_velocity && !isfinite(estimated_velocity)) ||
        !pid_valid_config(&pid->config) ||
        ((pid->config.mode == PID_EXTERNAL_VELOCITY) != has_velocity)) {
        return false;
    }

    const pid_config_t *config = &pid->config;
    const float error = setpoint - measurement;
    const float integral = pid_clamp(pid->integral + error * config->sample_time,
                                     config->integral_limit);
    const float p_term = pid_clamp(config->kp * error, config->p_limit);
    const float i_term = pid_clamp(config->ki * integral, config->i_limit);
    float derivative = 0.0f;

    if (config->mode == PID_EXTERNAL_VELOCITY) {
        derivative = -estimated_velocity;
    } else if (pid->initialized) {
        if (config->mode == PID_DERIVATIVE_ON_MEASUREMENT) {
            derivative = -(measurement - pid->previous_measurement) / config->sample_time;
        } else {
            derivative = (error - pid->previous_error) / config->sample_time;
        }
    }

    const float d_term = pid_clamp(config->kd * derivative, config->d_limit);
    float next_output;
    if (config->mode == PID_INCREMENTAL) {
        next_output = pid->output + (p_term - pid->p_term) +
                      (i_term - pid->i_term) + (d_term - pid->d_term);
    } else {
        next_output = p_term + i_term + d_term;
    }
    next_output = pid_clamp(next_output, config->output_limit);
    if (!isfinite(integral) || !isfinite(p_term) || !isfinite(i_term) ||
        !isfinite(d_term) || !isfinite(next_output)) {
        return false;
    }

    pid->integral = integral;
    pid->previous_error = error;
    pid->previous_measurement = measurement;
    pid->p_term = p_term;
    pid->i_term = i_term;
    pid->d_term = d_term;
    pid->output = next_output;
    pid->initialized = true;
    *output = next_output;
    return true;
}

bool pid_update(pid_t *pid, float setpoint, float measurement, float *output)
{
    return pid_update_internal(pid, setpoint, measurement, 0.0f, false, output);
}

bool pid_update_with_velocity(pid_t *pid, float setpoint, float measurement,
                              float estimated_velocity, float *output)
{
    return pid_update_internal(pid, setpoint, measurement, estimated_velocity, true, output);
}
