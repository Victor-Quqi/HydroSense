/**
 * @file hal_gpio.h
 * @brief GPIO控制硬件抽象层
 * @details 提供GPIO引脚控制和电源门控的硬件抽象接口
 */

#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdint.h>

/**
 * @brief 初始化GPIO硬件抽象层
 * @details 该函数应在系统启动时调用一次
 */
void hal_gpio_init();

/**
 * @brief 设置GPIO引脚的模式
 * @param pin_num GPIO引脚号
 * @param mode 引脚模式 (e.g., OUTPUT, INPUT)
 */
void hal_gpio_pin_mode(uint8_t pin_num, uint8_t mode);

/**
 * @brief 向GPIO引脚写入电平
 * @param pin_num GPIO引脚号
 * @param level 要写入的电平 (e.g., HIGH, LOW)
 */
void hal_gpio_write(uint8_t pin_num, uint8_t level);

#endif // HAL_GPIO_H