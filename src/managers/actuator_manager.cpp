/**
 * @file actuator_manager.cpp
 * @brief 执行器管理器实现
 */

#include "actuator_manager.h"
#include "power_manager.h"
#include "hal/hal_gpio.h"
#include "hal/hal_ledc.h"
#include "hal/hal_config.h"
#include "log_manager.h"
#include <Arduino.h>

#define PUMP_LEDC_CHANNEL 0

// 内部状态，用于智能电源管理
static bool is_pump_running = false;

// 私有辅助函数，用于管理12V电源
static void ensure_12v_power() {
    if (!power_pump_module_is_enabled()) {
        LOG_DEBUG("Actuator", "12V power is off. Turning on...");
        power_result_t result = power_pump_module_enable(true);
        if (result != POWER_OK) {
            LOG_ERROR("Actuator", "Failed to enable 12V boost module!");
        }
        delay(50); // 等待电源稳定
    }
}

static void shutdown_12v_if_idle() {
    if (!is_pump_running) {
        LOG_DEBUG("Actuator", "All actuators idle. Turning off 12V power.");
        power_result_t result = power_pump_module_enable(false);
        if (result != POWER_OK) {
            LOG_ERROR("Actuator", "Failed to disable 12V boost module.");
        }
    }
}

void actuator_manager_init()
{
    // 初始化水泵PWM通道，但不附加引脚
    hal_ledc_init(PIN_ACTUATOR_PUMP, PUMP_LEDC_CHANNEL);
    // 将水泵引脚初始化为普通GPIO输出，并设为高电平（安全状态）
    hal_gpio_pin_mode(PIN_ACTUATOR_PUMP, OUTPUT);
    hal_gpio_write(PIN_ACTUATOR_PUMP, HIGH);

    LOG_INFO("Actuator", "Actuator manager initialized.");
}

void actuator_manager_run_pump(uint8_t duty_cycle, uint32_t duration_ms)
{
    LOG_INFO("Actuator", "Running pump at %d/255 power for %d ms.", duty_cycle, duration_ms);

    ensure_12v_power();
    is_pump_running = true;

    // 1. 将引脚附加到LEDC外设
    ledcAttachPin(PIN_ACTUATOR_PUMP, PUMP_LEDC_CHANNEL);

    // 2. 运行PWM (注意：由于NPN三极管反相，占空比需要反转)
    uint8_t inverted_duty = 255 - duty_cycle;
    hal_ledc_set_duty(PUMP_LEDC_CHANNEL, inverted_duty);
    delay(duration_ms);
    hal_ledc_set_duty(PUMP_LEDC_CHANNEL, 255); // 确保PWM输出为0V，让三极管截止

    // 3. 将引脚与LEDC外设分离
    ledcDetachPin(PIN_ACTUATOR_PUMP);

    // 4. 恢复引脚为高电平安全状态
    hal_gpio_write(PIN_ACTUATOR_PUMP, HIGH);

    is_pump_running = false;
    LOG_INFO("Actuator", "Pump run finished.");
    shutdown_12v_if_idle();
}