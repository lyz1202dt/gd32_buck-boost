#include "controller.h"

#include <assert.h>
#include <stdint.h>

int main(void)
{
    controller_t controller = {0};
    uint16_t ch1 = 0U;
    uint16_t ch2 = 0U;

    controller_init(&controller, 599U, 587U, 0U);
    controller.ctrl_mode = 1;
    controller.vol_ref = 12.0f;
    controller.vol_out = 12.0f;
    controller.measurements_initialized = true;

    controller.vol_in = 13.5f;
    assert(controller_update(&controller, &ch1, &ch2));
    assert(controller.state == CONTROLLER_STATE_BUCK);
    assert(ch2 >= 574U && ch2 <= 576U);
    assert(controller.buck_pid.initialized);

    controller.vol_in = 12.5f;
    assert(controller_update(&controller, &ch1, &ch2));
    assert(controller.state == CONTROLLER_STATE_MIXTURE1);
    assert(ch2 >= 514U && ch2 <= 516U);
    assert(controller.bb1_pid.initialized);
    const uint16_t mix_dw_ch1 = ch1;
    controller.vol_out = 11.9f;
    assert(controller_update(&controller, &ch1, &ch2));
    assert(controller.state == CONTROLLER_STATE_MIXTURE1);
    assert(ch1 > mix_dw_ch1 && ch2 >= 514U && ch2 <= 516U);
    controller.vol_out = 12.0f;

    controller.vol_in = 12.05f;
    assert(controller_update(&controller, &ch1, &ch2));
    assert(controller.state == CONTROLLER_STATE_MIXTURE1);
    controller.vol_in = 11.99f;
    assert(controller_update(&controller, &ch1, &ch2));
    assert(controller.state == CONTROLLER_STATE_MIXTURE2);
    assert(ch1 >= 508U && ch1 <= 510U);
    assert(controller.bb2_pid.initialized);
    const uint16_t mix_up_ch2 = ch2;
    controller.vol_out = 11.9f;
    assert(controller_update(&controller, &ch1, &ch2));
    assert(controller.state == CONTROLLER_STATE_MIXTURE2);
    assert(ch1 >= 508U && ch1 <= 510U && ch2 < mix_up_ch2);
    controller.vol_out = 12.0f;

    controller.vol_in = 12.19f;
    assert(controller_update(&controller, &ch1, &ch2));
    assert(controller.state == CONTROLLER_STATE_MIXTURE2);
    controller.vol_in = 12.21f;
    assert(controller_update(&controller, &ch1, &ch2));
    assert(controller.state == CONTROLLER_STATE_MIXTURE1);

    controller.vol_in = 10.5f;
    assert(controller_update(&controller, &ch1, &ch2));
    assert(controller.state == CONTROLLER_STATE_BOOST);
    assert(ch1 >= 574U && ch1 <= 576U);
    assert(controller.boost_pid.initialized);

    controller.vol_in = 10.95f;
    assert(controller_update(&controller, &ch1, &ch2));
    assert(controller.state == CONTROLLER_STATE_BOOST);
    controller.vol_in = 11.01f;
    assert(controller_update(&controller, &ch1, &ch2));
    assert(controller.state == CONTROLLER_STATE_MIXTURE2);

    controller.vol_in = 12.5f;
    controller.vol_out = 11.7f;
    assert(controller_update(&controller, &ch1, &ch2));
    assert(controller.state == CONTROLLER_STATE_MIXTURE1);
    assert(ch2 >= 514U && ch2 <= 516U);

    controller.vol_in = 11.5f;
    assert(controller_update(&controller, &ch1, &ch2));
    assert(controller.state == CONTROLLER_STATE_MIXTURE2);
    assert(ch1 >= 508U && ch1 <= 510U);

    controller.ctrl_mode = 0;
    assert(controller_update(&controller, &ch1, &ch2));
    assert(controller.state == CONTROLLER_STATE_IDLE);
    assert(ch1 == 0U && ch2 == 0U);

    controller.ctrl_mode = 1;
    controller.vol_ref = 0.0f;
    assert(!controller_update(&controller, &ch1, &ch2));
    assert(ch1 == 0U && ch2 == 0U);
    return 0;
}
