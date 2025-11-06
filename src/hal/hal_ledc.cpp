/**
 * @file hal_ledc.cpp
 * @brief LEDC (PWM) 控制硬件抽象层实现
 */

#include "hal_ledc.h"
#include <Arduino.h>

// PWM属性
#define LEDC_TIMER_BIT 8
#define LEDC_BASE_FREQ 1000

void hal_ledc_init(uint8_t pin_num, uint8_t channel)
{
    ledcSetup(channel, LEDC_BASE_FREQ, LEDC_TIMER_BIT);
    ledcAttachPin(pin_num, channel);
}

void hal_ledc_set_duty(uint8_t channel, uint8_t duty_cycle)
{
    ledcWrite(channel, duty_cycle);
}