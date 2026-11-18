#include "buttons.h"

#include "gd32f30x_adc.h"
#include "gd32f30x_gpio.h"
#include "task.h"

#include <stdint.h>

#define BUTTON_SCAN_PERIOD_TICKS pdMS_TO_TICKS(10U)
#define BUTTON_DEBOUNCE_SAMPLES 3U
#define BUTTON_QUEUE_LENGTH 16U
#define ADC_MAX_COUNT 4095U

typedef enum {
    BUTTON_NONE = 4,
    BUTTON_INVALID
} button_sample_t;

static QueueHandle_t event_queue;

bool buttons_init(void)
{
    event_queue = xQueueCreate(BUTTON_QUEUE_LENGTH, sizeof(button_event_t));
    if (event_queue == NULL) {
        return false;
    }

    /* KEY_ADC is PA0 / ADC0 channel 0; injected PWM samples remain configured separately. */
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, GPIO_PIN_0);
    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 1U);
    adc_routine_channel_config(ADC0, 0U, ADC_CHANNEL_0, ADC_SAMPLETIME_41POINT5);
    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, ENABLE);
    return true;
}

QueueHandle_t buttons_event_queue(void)
{
    return event_queue;
}

static bool read_key_adc(uint16_t *value)
{
    TickType_t start = xTaskGetTickCount();

    adc_flag_clear(ADC0, ADC_FLAG_EOC);
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);
    while (adc_flag_get(ADC0, ADC_FLAG_EOC) == RESET) {
        if ((xTaskGetTickCount() - start) >= pdMS_TO_TICKS(2U)) {
            return false;
        }
    }

    *value = adc_routine_data_read(ADC0);
    return true;
}

static button_sample_t decode_key(uint16_t value)
{
    /* Equal R28-R32: SW9, SW8, SW7, SW6 yield VCC/5, /4, /3, /2. */
    if (value < ADC_MAX_COUNT / 10U) {
        return BUTTON_NONE;
    }
    if (value < ADC_MAX_COUNT * 9U / 40U) {
        return BUTTON_SW9;
    }
    if (value < ADC_MAX_COUNT * 7U / 24U) {
        return BUTTON_SW8;
    }
    if (value < ADC_MAX_COUNT * 5U / 12U) {
        return BUTTON_SW7;
    }
    if (value < ADC_MAX_COUNT * 3U / 4U) {
        return BUTTON_SW6;
    }
    return BUTTON_INVALID;
}

static void publish_event(button_id_t button, button_action_t action)
{
    const button_event_t event = {button, action};
    button_event_t discarded;

    if (xQueueSend(event_queue, &event, 0U) != pdPASS) {
        (void)xQueueReceive(event_queue, &discarded, 0U);
        (void)xQueueSend(event_queue, &event, 0U);
    }
}

void ButtonsTask(void *param)
{
    TickType_t previous_wake_time = xTaskGetTickCount();
    button_sample_t stable = BUTTON_NONE;
    button_sample_t candidate = BUTTON_INVALID;
    uint8_t count = 0U;

    (void)param;
    for (;;) {
        uint16_t adc_value;
        button_sample_t sample;

        vTaskDelayUntil(&previous_wake_time, BUTTON_SCAN_PERIOD_TICKS);
        if (!read_key_adc(&adc_value)) {
            count = 0U;
            candidate = BUTTON_INVALID;
            continue;
        }

        sample = decode_key(adc_value);
        if (sample == BUTTON_INVALID) {
            count = 0U;
            candidate = BUTTON_INVALID;
            continue;
        }
        if (sample != candidate) {
            candidate = sample;
            count = 1U;
        } else if (count < BUTTON_DEBOUNCE_SAMPLES) {
            ++count;
        }

        if (count == BUTTON_DEBOUNCE_SAMPLES && stable != candidate) {
            if (stable != BUTTON_NONE) {
                publish_event((button_id_t)stable, BUTTON_RELEASED);
            }
            if (candidate != BUTTON_NONE) {
                publish_event((button_id_t)candidate, BUTTON_PRESSED);
            }
            stable = candidate;
        }
    }
}
