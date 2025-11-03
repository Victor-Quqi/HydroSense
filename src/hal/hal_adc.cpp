/**
 * @file hal_adc.cpp
 * @brief ADC读取硬件抽象层实现
 */

#include "hal_adc.h"
#include <Arduino.h>

void hal_adc_init() {
    // ESP32-S3的ADC特性是自动校准的，
    // 并且analogRead函数会自动处理初始化。
    // 因此，目前无需在此处添加特定代码。
}

uint16_t hal_adc_read(uint8_t pin_num, bool* p_success) {
    uint16_t value = analogRead(pin_num);

    // 检查ADC读取是否成功
    bool success = true;
    if (value == 0 || value == 4095) {
        success = false;  // 可能的异常值（短路或断路）
    }

    // 如果调用者提供了成功标志指针，则设置它
    if (p_success != nullptr) {
        *p_success = success;
    }

    return value;
}