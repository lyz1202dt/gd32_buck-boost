#ifndef LIB_PID_H
#define LIB_PID_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PID_POSITIONAL,
    PID_DERIVATIVE_ON_MEASUREMENT,
    PID_INCREMENTAL,
    PID_EXTERNAL_VELOCITY
} pid_mode_t;

typedef struct {
    pid_mode_t mode;
    float kp;
    float ki;
    float kd;
    float sample_time;    /* Seconds between updates; must be positive. */
    float p_limit;        /* Symmetric P output limit; zero disables the limit. */
    float i_limit;        /* Symmetric I output limit; zero disables the limit. */
    float d_limit;        /* Symmetric D output limit; zero disables the limit. */
    float integral_limit; /* Symmetric limit on accumulated error * seconds. */
    float output_limit;   /* Symmetric total output limit; zero disables it. */
} pid_config_t;

typedef struct {
    pid_config_t config;
    float integral;
    float previous_error;
    float previous_measurement;
    float p_term;
    float i_term;
    float d_term;
    float output;
    bool initialized;
} pid_t;

/* Invalid configuration leaves the instance untouched. */
bool pid_init(pid_t *pid, const pid_config_t *config);
void pid_reset(pid_t *pid);

/* Returns false without changing the instance or output on invalid arguments. */
/* Use this for all modes except PID_EXTERNAL_VELOCITY. */
bool pid_update(pid_t *pid, float setpoint, float measurement, float *output);
/* Incremental mode: u += delta(P) + Ki*error*dt + delta(D), then clamp
 * to actuator limits. i_limit bounds each integral increment; integral_limit
 * is unused here because this path has no separate accumulated integral.
 * step_limit == 0 disables the per-update delta limit. */
bool pid_update_bounded(pid_t *pid, float setpoint, float measurement,
                        float lower, float upper, float step_limit, float *output);
/* Only for PID_EXTERNAL_VELOCITY; velocity is measurement rate in units/second. */
bool pid_update_with_velocity(pid_t *pid, float setpoint, float measurement,
                              float estimated_velocity, float *output);

#ifdef __cplusplus
}
#endif

#endif
