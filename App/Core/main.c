#include "FreeRTOS.h"
#include "controller.h"
#include "gd32f30x_adc.h"
#include "gd32f30x_gpio.h"
#include "gd32f30x_rcu.h"
#include "task.h"
#include "usart.h"
#include "screen.h"


TaskHandle_t ScreenTaskHandle;
TaskHandle_t TestTaskHandle;


usart_handle_t huart;
controller_t controller;

static void TestTask(void* param);

void timer_config();
void adc_config();
void uart_config();
void nvic_config();
void gpio_config();
void gpio_remap();

int main(void) {
    /* 587/599 limits either upper switch to about 98% duty. */
    controller_init(&controller, 599U, 587U, 0U);
    gpio_config();
    gpio_remap();
    adc_config();
    timer_config();
    uart_config();
    nvic_config();
    xTaskCreate(ScreenTask, "screen", 512, NULL, tskIDLE_PRIORITY + 1U, &ScreenTaskHandle);
    xTaskCreate(TestTask, "test", 256, NULL, tskIDLE_PRIORITY + 1U, &TestTaskHandle);

    //启动调度器
    vTaskStartScheduler();
    while(1);
}

void TestTask(void* param){
    (void)param;
    TickType_t previous_wake_time = xTaskGetTickCount();
    for(;;){
        vTaskDelayUntil(&previous_wake_time, pdMS_TO_TICKS(1000U));
    }
}

void uart_config(void){
    usart_config_t config;
    config.baud_rate=115200;
    config.data_bits=8;
    config.parity=USART_PARITY_NONE;
    config.stop_bits=USART_STOP_BITS_1;
    config.instance=USART2;
    usart_init(&huart, &config);
}

void nvic_config(void){
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    /* usart_irq_handler() uses FreeRTOS ISR APIs, so keep this at the
       FreeRTOS syscall priority or below (numerically 5 or higher). */
    nvic_irq_enable(USART2_IRQn, 5, 0);
    nvic_irq_enable(ADC0_1_IRQn, 1, 0);
    nvic_irq_enable(TIMER0_UP_TIMER9_IRQn, 1, 0);
}

void gpio_remap(void){
    rcu_periph_clock_enable(RCU_AF);
}

void gpio_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_AF);

    /*configure PA8 PA9(TIMER0 CH0 CH1 CH2) as alternate function*/
    gpio_init(GPIOA,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_8);
    gpio_init(GPIOA,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_9);

    /* ADC0 IN2/IN3: PA2/PA3 */
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, GPIO_PIN_2 | GPIO_PIN_3);

    /* ADC1 IN8/IN9: PB0/PB1 */
    gpio_init(GPIOB, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, GPIO_PIN_0 | GPIO_PIN_1);

    /* USART2 default pins: PB10 (TX) and PB11 (RX). */
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
    gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_11);

    /*configure PB13 PB14(TIMER0 CH0N CH1N CH2N) as alternate function*/
    gpio_init(GPIOB,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_13);
    gpio_init(GPIOB,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,GPIO_PIN_14);
}

void timer_config(void)
{
    timer_oc_parameter_struct timer_ocintpara;
    timer_oc_parameter_struct adc_trigger_oc;
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER0);

    timer_deinit(TIMER0);

    /* TIMER0 configuration */
    /* TIMER0 clock = 120 MHz; center-aligned PWM: f = 120 MHz / (1 * 600 * 2) = 100 kHz */
    timer_initpara.prescaler         = 0;
    /* CENTER_UP makes the CH3 compare event occur once per PWM period. */
    timer_initpara.alignedmode       = TIMER_COUNTER_CENTER_UP;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 599;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    /* One control interrupt for every 20 center-aligned update events. */
    timer_initpara.repetitioncounter = 19;
    timer_init(TIMER0,&timer_initpara);

     /* CH1,CH2 and CH3 configuration in PWM mode */
    timer_ocintpara.outputstate  = TIMER_CCX_ENABLE;
    timer_ocintpara.outputnstate = TIMER_CCXN_ENABLE;
    timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
    timer_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;

    timer_channel_output_config(TIMER0,TIMER_CH_0,&timer_ocintpara);
    timer_channel_output_config(TIMER0,TIMER_CH_1,&timer_ocintpara);

    /* CH3 is an internal ADC trigger at the PWM center (CNT == ARR). */
    adc_trigger_oc.outputstate  = TIMER_CCX_DISABLE;
    adc_trigger_oc.outputnstate = TIMER_CCXN_DISABLE;
    adc_trigger_oc.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    adc_trigger_oc.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
    adc_trigger_oc.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    adc_trigger_oc.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
    timer_channel_output_config(TIMER0, TIMER_CH_3, &adc_trigger_oc);
    timer_channel_output_mode_config(TIMER0, TIMER_CH_3, TIMER_OC_MODE_TIMING);
    timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_3, 599);

    timer_channel_output_pulse_value_config(TIMER0,TIMER_CH_0,0);
    timer_channel_output_mode_config(TIMER0,TIMER_CH_0,TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER0,TIMER_CH_0,TIMER_OC_SHADOW_DISABLE);

    timer_channel_output_pulse_value_config(TIMER0,TIMER_CH_1,0);
    timer_channel_output_mode_config(TIMER0,TIMER_CH_1,TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER0,TIMER_CH_1,TIMER_OC_SHADOW_DISABLE);

    timer_primary_output_config(TIMER0,ENABLE);

    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER0);
    timer_interrupt_flag_clear(TIMER0, TIMER_INT_FLAG_UP);
    timer_interrupt_enable(TIMER0, TIMER_INT_UP);
    timer_enable(TIMER0);
}

void adc_config(void)
{
    rcu_periph_clock_enable(RCU_ADC0);
    rcu_periph_clock_enable(RCU_ADC1);
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV12); /* 120 MHz / 12 = 10 MHz */

    adc_deinit(ADC0);
    adc_deinit(ADC1);

    adc_mode_config(ADC_MODE_FREE);
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    adc_data_alignment_config(ADC1, ADC_DATAALIGN_RIGHT);

    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
    adc_special_function_config(ADC1, ADC_SCAN_MODE, ENABLE);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);
    adc_special_function_config(ADC1, ADC_CONTINUOUS_MODE, DISABLE);

    /* Two injected conversions are started by TIMER0 CH3 at 100 kHz. */
    adc_channel_length_config(ADC0, ADC_INSERTED_CHANNEL, 2);
    adc_inserted_channel_config(ADC0, ADC_INSERTED_CHANNEL_0,
                                ADC_CHANNEL_2, ADC_SAMPLETIME_1POINT5);
    adc_inserted_channel_config(ADC0, ADC_INSERTED_CHANNEL_1,
                                ADC_CHANNEL_3, ADC_SAMPLETIME_1POINT5);
    adc_external_trigger_source_config(ADC0, ADC_INSERTED_CHANNEL,
                                        ADC0_1_EXTTRIG_INSERTED_T0_CH3);
    adc_external_trigger_config(ADC0, ADC_INSERTED_CHANNEL, ENABLE);

    adc_channel_length_config(ADC1, ADC_INSERTED_CHANNEL, 2);
    adc_inserted_channel_config(ADC1, ADC_INSERTED_CHANNEL_0,
                                ADC_CHANNEL_8, ADC_SAMPLETIME_1POINT5);
    adc_inserted_channel_config(ADC1, ADC_INSERTED_CHANNEL_1,
                                ADC_CHANNEL_9, ADC_SAMPLETIME_1POINT5);
    adc_external_trigger_source_config(ADC1, ADC_INSERTED_CHANNEL,
                                        ADC0_1_EXTTRIG_INSERTED_T0_CH3);
    adc_external_trigger_config(ADC1, ADC_INSERTED_CHANNEL, ENABLE);

    adc_enable(ADC0);
    adc_calibration_enable(ADC0);
    adc_enable(ADC1);
    adc_calibration_enable(ADC1);

    /* ADC0 and ADC1 share ADC0_1_IRQn; update after both inserted sequences complete. */
    adc_interrupt_enable(ADC0, ADC_INT_EOIC);
    adc_interrupt_enable(ADC1, ADC_INT_EOIC);
}
