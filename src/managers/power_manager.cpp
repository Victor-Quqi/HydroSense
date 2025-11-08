/**
 * @file power_manager.cpp
 * @brief 电源管理器实现
 */

#include "power_manager.h"
#include "hal/hal_config.h"
#include "hal/hal_gpio.h"
#include "managers/log_manager.h"

// --- 私有状态变量 ---
static bool is_sensor_powered = false;
static bool is_pump_module_powered = false;
static bool is_screen_powered = false;
static bool is_initialized = false;  // 初始化标志

// --- 私有辅助函数 ---
static const char* power_result_to_string(power_result_t result) {
    switch (result) {
        case POWER_OK: return "OK";
        case POWER_ERROR_GPIO_FAILED: return "GPIO_FAILED";
        case POWER_ERROR_INVALID_PARAM: return "INVALID_PARAM";
        case POWER_ERROR_NOT_INIT: return "NOT_INIT";
        default: return "UNKNOWN";
    }
}

power_result_t power_manager_init() {
    // 1. 初始化所有电源门控引脚为输出模式
    hal_gpio_pin_mode(PIN_POWER_GATE_PUMP, OUTPUT);
    hal_gpio_pin_mode(PIN_POWER_GATE_SENSOR, OUTPUT);
    hal_gpio_pin_mode(PIN_POWER_GATE_DISPLAY, OUTPUT);

    // 2. 强制将所有外设电源设置为关闭状态 (绕过状态检查)
    hal_gpio_write(PIN_POWER_GATE_PUMP, POWER_OFF);
    hal_gpio_write(PIN_POWER_GATE_SENSOR, POWER_OFF);
    hal_gpio_write(PIN_POWER_GATE_DISPLAY, POWER_OFF);

    // 3. 同步软件状态变量以匹配硬件的已知状态
    is_pump_module_powered = false;
    is_sensor_powered = false;
    is_screen_powered = false;

    // 4. 标记为已初始化
    is_initialized = true;

    LOG_DEBUG("Power", "Power manager initialized successfully");

    return POWER_OK;
}

power_result_t power_sensor_enable(bool enable) {
    if (!is_initialized) {
        return POWER_ERROR_NOT_INIT;
    }

    if (is_sensor_powered == enable) {
        // 状态未改变，不执行任何操作
        return POWER_OK;
    }

    // 执行GPIO操作 (HAL层不提供错误检查，假设操作成功)
    hal_gpio_write(PIN_POWER_GATE_SENSOR, enable ? POWER_ON : POWER_OFF);

    // 更新软件状态
    is_sensor_powered = enable;

    LOG_DEBUG("Power", "Sensor power %s", enable ? "ON" : "OFF");

    return POWER_OK;
}

power_result_t power_pump_module_enable(bool enable) {
    if (!is_initialized) {
        return POWER_ERROR_NOT_INIT;
    }

    if (is_pump_module_powered == enable) {
        return POWER_OK;
    }

    hal_gpio_write(PIN_POWER_GATE_PUMP, enable ? POWER_ON : POWER_OFF);
    is_pump_module_powered = enable;

    LOG_DEBUG("Power", "Pump module power %s", enable ? "ON" : "OFF");

    return POWER_OK;
}

power_result_t power_screen_enable(bool enable) {
    if (!is_initialized) {
        return POWER_ERROR_NOT_INIT;
    }

    if (is_screen_powered == enable) {
        return POWER_OK;
    }

    hal_gpio_write(PIN_POWER_GATE_DISPLAY, enable ? POWER_ON : POWER_OFF);
    is_screen_powered = enable;

    LOG_DEBUG("Power", "Screen power %s", enable ? "ON" : "OFF");

    return POWER_OK;
}

/* ========== 新命名函数 ========== */
bool power_sensor_is_enabled() {
    return is_sensor_powered;
}

bool power_pump_module_is_enabled() {
    return is_pump_module_powered;
}

bool power_screen_is_enabled() {
    return is_screen_powered;
}

/* 兼容性函数已移除 */