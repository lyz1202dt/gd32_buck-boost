#include <stdint.h>
#include "systick.h"
#include "gd32f30x.h"

int main(void)
{
    systick_config();

    /* LED2 on the GD32307C-EVAL board is connected to PC0 (active low). */
    rcu_periph_clock_enable(RCU_GPIOC); 
    gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_0);
    gpio_bit_set(GPIOC, GPIO_PIN_0);

    while (1) {
        delay_1ms(500U);
        gpio_bit_write(GPIOC, GPIO_PIN_0,
                       (SET == gpio_output_bit_get(GPIOC, GPIO_PIN_0)) ? RESET : SET);
    }
}
