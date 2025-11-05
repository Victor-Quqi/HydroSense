/**
 * @file actuator_manager.cpp
 * @brief 执行器管理器实现
 */

#include "actuator_manager.h"
#include "power_manager.h"
#include "hal/hal_gpio.h"
#include "hal/hal_config.h"
#include "log_manager.h"
#include <Arduino.h>

void actuator_manager_init()
{
    // 初始化时确保水泵控制引脚为输出模式且为低电平
    hal_gpio_pin_mode(PIN_ACTUATOR_PUMP, OUTPUT);
    hal_gpio_write(PIN_ACTUATOR_PUMP, LOW);
    LOG_INFO("Actuator", "Actuator manager initialized.");
}

void actuator_manager_run_pump(uint32_t duration_ms)
{
    LOG_INFO("Actuator", "Running pump for %d ms.", duration_ms);

    // 1. 打开12V升压模块电源
    power_result_t result = power_pump_module_enable(true);
    if (result != POWER_OK)
    {
        LOG_ERROR("Actuator", "Failed to enable 12V boost module. Aborting pump run.");
        return;
    }

    // 短暂延时，确保电源稳定
    delay(50);

    // 2. 打开水泵
    hal_gpio_write(PIN_ACTUATOR_PUMP, HIGH);

    // 3. 等待指定时长
    delay(duration_ms);

    // 4. 关闭水泵
    hal_gpio_write(PIN_ACTUATOR_PUMP, LOW);

    // 5. 关闭12V升压模块电源
    result = power_pump_module_enable(false);
    if (result != POWER_OK)
    {
        LOG_ERROR("Actuator", "Failed to disable 12V boost module.");
    }
    
    LOG_INFO("Actuator", "Pump run finished.");
}