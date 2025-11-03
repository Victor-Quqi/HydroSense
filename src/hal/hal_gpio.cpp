/**
 * @file hal_gpio.cpp
 * @brief GPIO控制硬件抽象层实现
 */

#include "hal_gpio.h"
#include <Arduino.h>

void hal_gpio_init() {
    // 目前无需特殊初始化操作
}

void hal_gpio_pin_mode(uint8_t pin_num, uint8_t mode) {
    pinMode(pin_num, mode);
}

void hal_gpio_write(uint8_t pin_num, uint8_t level) {
    digitalWrite(pin_num, level);
}