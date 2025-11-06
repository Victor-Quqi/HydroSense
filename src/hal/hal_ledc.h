/**
 * @file hal_ledc.h
 * @brief LEDC (PWM) 控制硬件抽象层
 * @details 提供对ESP32 LEDC外设的封装，用于生成PWM信号
 */

#ifndef HAL_LEDC_H
#define HAL_LEDC_H

#include <stdint.h>

/**
 * @brief 初始化LEDC通道
 * @param pin_num 要输出PWM信号的GPIO引脚号
 * @param channel LEDC通道 (0-15)
 */
void hal_ledc_init(uint8_t pin_num, uint8_t channel);

/**
 * @brief 设置LEDC通道的占空比
 * @param channel LEDC通道 (0-15)
 * @param duty_cycle 占空比 (0-255)
 */
void hal_ledc_set_duty(uint8_t channel, uint8_t duty_cycle);

#endif // HAL_LEDC_H