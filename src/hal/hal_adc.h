/**
 * @file hal_adc.h
 * @brief ADC读取硬件抽象层
 * @details 提供ADC传感器读取的硬件抽象接口
 */

#ifndef HAL_ADC_H
#define HAL_ADC_H

#include <stdint.h>

/**
 * @brief 初始化ADC硬件抽象层
 */
void hal_adc_init();

/**
 * @brief 从指定的ADC引脚读取原始值
 * @param pin_num GPIO引脚号
 * @param p_success 指向成功标志的指针（可为NULL）
 * @return uint16_t ADC原始读数 (0-4095)
 */
uint16_t hal_adc_read(uint8_t pin_num, bool* p_success);

#endif // HAL_ADC_H