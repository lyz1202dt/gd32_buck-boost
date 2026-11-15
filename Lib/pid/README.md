# PID library

`pid_init()` sets gains, sample period (seconds), mode and limits. Call an update
function once per sample period. `pid_reset()` clears the accumulated error,
history and output without changing configuration.

- `PID_POSITIONAL`: `P + I + D`, with D calculated from error change.
- `PID_DERIVATIVE_ON_MEASUREMENT`: `P + I - Kd * measurement_rate`.
- `PID_INCREMENTAL`: adds changes in the bounded P, I and D terms to the
  previous output; without limits, this gives the usual incremental PID formula.
  `pid_update_bounded()` uses the reference-style incremental PI formula with
  actuator limits and a per-update step limit. It stores only the saturated
  output, so there is no separate integral to wind up. Do not mix it with
  `pid_update()` on the same instance without resetting the PID. In this path,
  `i_limit` bounds each integral increment and `integral_limit` is unused.
- `PID_EXTERNAL_VELOCITY`: `P + I - Kd * estimated_velocity`. Pass the measured
  variable's estimated rate in units per second to `pid_update_with_velocity()`.

Limits are symmetric magnitudes; zero disables a limit. `p_limit`, `i_limit`
and `d_limit` bound each term, `integral_limit` bounds accumulated error in
error-seconds, and `output_limit` bounds the final output. All configuration
values must be finite, limits must be nonnegative, and the sample period must
be positive. The first update uses zero D for modes that estimate a derivative
from samples. The external velocity mode uses its supplied velocity immediately.

```c
pid_t controller;
pid_config_t config = {
    .mode = PID_EXTERNAL_VELOCITY,
    .kp = 1.0f, .ki = 0.2f, .kd = 0.05f,
    .sample_time = 0.001f,
    .integral_limit = 10.0f,
    .output_limit = 100.0f
};
float command;

pid_init(&controller, &config);
pid_update_with_velocity(&controller, target, measured, estimated_speed, &command);
```
