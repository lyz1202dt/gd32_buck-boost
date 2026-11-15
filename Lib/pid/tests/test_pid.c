#include "pid.h"

#include <assert.h>
#include <math.h>

static void expect_near(float actual, float expected)
{
    assert(fabsf(actual - expected) < 0.0001f);
}

int main(void)
{
    pid_t pid;
    float output;
    pid_config_t config = {
        .mode = PID_POSITIONAL,
        .kp = 2.0f,
        .ki = 1.0f,
        .kd = 0.5f,
        .sample_time = 0.1f
    };

    assert(pid_init(&pid, &config));
    assert(pid_update(&pid, 3.0f, 1.0f, &output));
    expect_near(output, 4.2f);
    assert(pid_update(&pid, 3.0f, 2.0f, &output));
    expect_near(output, -2.7f);

    config.p_limit = 2.0f;
    config.ki = 4.0f;
    config.i_limit = 1.0f;
    config.d_limit = 3.0f;
    config.integral_limit = 0.5f;
    config.output_limit = 4.0f;
    assert(pid_init(&pid, &config));
    assert(pid_update(&pid, 10.0f, 0.0f, &output));
    expect_near(output, 3.0f);
    expect_near(pid.integral, 0.5f);
    assert(pid_update(&pid, -10.0f, 0.0f, &output));
    expect_near(output, -4.0f);
    expect_near(pid.integral, -0.5f);

    config.mode = PID_DERIVATIVE_ON_MEASUREMENT;
    config.kp = 0.0f;
    config.ki = 0.0f;
    config.kd = 1.0f;
    config.p_limit = 0.0f;
    config.i_limit = 0.0f;
    config.d_limit = 0.0f;
    config.integral_limit = 0.0f;
    config.output_limit = 0.0f;
    assert(pid_init(&pid, &config));
    assert(pid_update(&pid, 0.0f, 5.0f, &output));
    expect_near(output, 0.0f);
    assert(pid_update(&pid, 100.0f, 5.0f, &output));
    expect_near(output, 0.0f);
    assert(pid_update(&pid, 100.0f, 6.0f, &output));
    expect_near(output, -10.0f);

    config.mode = PID_INCREMENTAL;
    config.kp = 2.0f;
    config.ki = 1.0f;
    config.kd = 0.5f;
    assert(pid_init(&pid, &config));
    assert(pid_update(&pid, 3.0f, 1.0f, &output));
    expect_near(output, 4.2f);
    assert(pid_update(&pid, 3.0f, 2.0f, &output));
    expect_near(output, -2.7f);
    assert(pid_update(&pid, 3.0f, 2.0f, &output));
    expect_near(output, 2.4f);

    config.output_limit = 4.0f;
    assert(pid_init(&pid, &config));
    assert(pid_update(&pid, 3.0f, 1.0f, &output));
    expect_near(output, 4.0f);
    assert(pid_update(&pid, 3.0f, 2.0f, &output));
    expect_near(output, -2.9f);

    config.kp = 0.0f;
    config.ki = 1.0f;
    config.kd = 0.0f;
    config.sample_time = 1.0f;
    config.output_limit = 0.0f;
    assert(pid_init(&pid, &config));
    assert(pid_update_bounded(&pid, 1.0f, 0.0f, 0.0f, 0.3f, 0.0f, &output));
    expect_near(output, 0.3f);
    assert(pid_update_bounded(&pid, 1.0f, 0.0f, 0.0f, 0.3f, 0.0f, &output));
    expect_near(output, 0.3f);
    expect_near(pid.integral, 0.0f);
    assert(pid_update_bounded(&pid, 0.0f, 1.0f, 0.0f, 0.3f, 0.0f, &output));
    expect_near(output, 0.0f);
    output = 123.0f;
    assert(!pid_update_bounded(&pid, 1.0f, 0.0f, 0.3f, 0.0f, 0.0f, &output));
    expect_near(output, 123.0f);

    config.kp = 1.0f;
    config.ki = 0.0f;
    assert(pid_init(&pid, &config));
    assert(pid_update_bounded(&pid, 1.0f, 0.0f, 0.0f, 1.0f, 0.08f, &output));
    expect_near(output, 0.08f);

    config.mode = PID_EXTERNAL_VELOCITY;
    config.sample_time = 0.1f;
    config.kp = 2.0f;
    config.ki = 1.0f;
    config.kd = 0.5f;
    config.output_limit = 0.0f;
    assert(pid_init(&pid, &config));
    output = 123.0f;
    assert(!pid_update(&pid, 3.0f, 1.0f, &output));
    expect_near(output, 123.0f);
    assert(!pid.initialized);
    assert(pid_update_with_velocity(&pid, 3.0f, 1.0f, 4.0f, &output));
    expect_near(output, 2.2f);
    assert(!pid_update_with_velocity(&pid, 3.0f, 1.0f, NAN, &output));
    expect_near(output, 2.2f);
    pid_reset(&pid);
    expect_near(pid.output, 0.0f);
    assert(!pid.initialized);

    config.sample_time = 0.0f;
    assert(!pid_init(&pid, &config));
    assert(pid.config.sample_time == 0.1f);
    return 0;
}
