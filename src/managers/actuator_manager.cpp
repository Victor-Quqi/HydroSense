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

// 内部状态
static bool is_pump_running = false;
static uint32_t pump_start_time = 0;
static uint32_t pump_duration_ms = 0;

// 私有辅助函数
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
    // 初始化水泵PWM通道
    hal_ledc_init(PUMP_LEDC_CHANNEL);
    // 将水泵引脚初始化为普通GPIO输出，并设为高电平（安全状态）
    hal_gpio_pin_mode(PIN_ACTUATOR_PUMP, OUTPUT);
    hal_gpio_write(PIN_ACTUATOR_PUMP, HIGH);

    LOG_INFO("Actuator", "Actuator manager initialized.");
}

void actuator_manager_start_pump(uint8_t duty_cycle)
{
    if (is_pump_running) {
        LOG_WARN("Actuator", "Pump is already running.");
        return;
    }
    LOG_INFO("Actuator", "Starting pump at %d/255 power.", duty_cycle);

    ensure_12v_power();
    is_pump_running = true;

    hal_ledc_attach_pin(PIN_ACTUATOR_PUMP, PUMP_LEDC_CHANNEL);
    
    // 注意：由于NPN三极管反相，占空比需要反转
    uint8_t inverted_duty = 255 - duty_cycle;
    hal_ledc_set_duty(PUMP_LEDC_CHANNEL, inverted_duty);
}

void actuator_manager_stop_pump()
{
    if (!is_pump_running) {
        return;
    }
    LOG_INFO("Actuator", "Stopping pump.");

    hal_ledc_set_duty(PUMP_LEDC_CHANNEL, 255); // 确保PWM输出为0V
    hal_ledc_detach_pin(PIN_ACTUATOR_PUMP);
    hal_gpio_write(PIN_ACTUATOR_PUMP, HIGH); // 恢复安全状态

    is_pump_running = false;
    pump_duration_ms = 0; // 清除计时
    shutdown_12v_if_idle();
}

void actuator_manager_run_pump_for(uint8_t duty_cycle, uint32_t duration_ms)
{
    if (is_pump_running) {
        LOG_WARN("Actuator", "Pump is already running. Ignoring new timed run request.");
        return;
    }
    pump_duration_ms = duration_ms;
    pump_start_time = millis();
    actuator_manager_start_pump(duty_cycle);
}

void actuator_manager_loop()
{
    if (is_pump_running && pump_duration_ms > 0) {
        if (millis() - pump_start_time >= pump_duration_ms) {
            LOG_INFO("Actuator", "Timed run finished.");
            actuator_manager_stop_pump();
        }
    }
}