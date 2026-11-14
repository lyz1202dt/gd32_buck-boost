/*!
    \file    gd32f30x_it.c
    \brief   interrupt service routines

    \version 2026-2-6, V3.0.3, firmware for GD32F30x
*/

/*
    Copyright (c) 2025, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#include "gd32f30x_it.h"

#include "controller.h"
#include "gd32f30x_adc.h"
#include "gd32f30x_timer.h"
#include "usart.h"

extern controller_t controller;
extern usart_handle_t huart;

/* 3.3 V ADC, 20k/1k voltage dividers, INA240A2 gain 50 with 1 mOhm shunts. */
#define ADC_VOLTS_PER_COUNT (3.3f / 4095.0f)
#define VOLTAGE_SENSE_GAIN 21.0f
#define CURRENT_SENSE_A_PER_V 20.0f

/*!
    \brief      this function handles NMI exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void NMI_Handler(void)
{
}

/*!
    \brief      this function handles HardFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void HardFault_Handler(void)
{
    /* if Hard Fault exception occurs, go to infinite loop */
    while (1){
    }
}

/*!
    \brief      this function handles MemManage exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void MemManage_Handler(void)
{
    /* if Memory Manage exception occurs, go to infinite loop */
    while (1){
    }
}

/*!
    \brief      this function handles BusFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void BusFault_Handler(void)
{
    /* if Bus Fault exception occurs, go to infinite loop */
    while (1){
    }
}

/*!
    \brief      this function handles UsageFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void UsageFault_Handler(void)
{
    /* if Usage Fault exception occurs, go to infinite loop */
    while (1){
    }
}

/*!
    \brief      this function handles DebugMon exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DebugMon_Handler(void)
{
}

/*!
    \brief      this function handles the USART2 interrupt
    \param[in]  none
    \param[out] none
    \retval     none
*/
void USART2_IRQHandler(void)
{
    usart_irq_handler(&huart);
}

void DMA0_Channel4_IRQHandler(void)
{
}

/*!
    \brief      this function handles the USART0 transmit DMA interrupt
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DMA0_Channel3_IRQHandler(void)
{
}



void ADC0_1_IRQHandler(void)
{
    static uint8_t completed = 0U;

    if (adc_interrupt_flag_get(ADC0, ADC_INT_FLAG_EOIC) == SET) {
        adc_interrupt_flag_clear(ADC0, ADC_INT_FLAG_EOIC);
        completed |= 0x01U;
    }
    if (adc_interrupt_flag_get(ADC1, ADC_INT_FLAG_EOIC) == SET) {
        adc_interrupt_flag_clear(ADC1, ADC_INT_FLAG_EOIC);
        completed |= 0x02U;
    }

    if (completed == 0x03U) {
        uint16_t ch1 = (uint16_t)TIMER_CH0CV(TIMER0);
        uint16_t ch2 = (uint16_t)TIMER_CH1CV(TIMER0);
        float cur_out;
        float vol_out;
        float vol_in;
        float cur_in;

        completed = 0U;
        cur_out = (float)adc_inserted_data_read(ADC0, ADC_INSERTED_CHANNEL_0) *
                  ADC_VOLTS_PER_COUNT * CURRENT_SENSE_A_PER_V;
        vol_out = (float)adc_inserted_data_read(ADC0, ADC_INSERTED_CHANNEL_1) *
                  ADC_VOLTS_PER_COUNT * VOLTAGE_SENSE_GAIN;
        vol_in = (float)adc_inserted_data_read(ADC1, ADC_INSERTED_CHANNEL_0) *
                 ADC_VOLTS_PER_COUNT * VOLTAGE_SENSE_GAIN;
        cur_in = (float)adc_inserted_data_read(ADC1, ADC_INSERTED_CHANNEL_1) *
                 ADC_VOLTS_PER_COUNT * CURRENT_SENSE_A_PER_V;

        set_controller_state(&controller, vol_in, cur_in, vol_out, cur_out);
        controller_update(&controller, &ch1, &ch2);
        timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, ch1);
        timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_1, ch2);
    }
}
