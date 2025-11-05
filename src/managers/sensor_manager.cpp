/**
 * @file sensor_manager.cpp
 * @brief 传感器管理器实现
 */

#include "sensor_manager.h"
#include "power_manager.h"
#include "managers/log_manager.h"
#include "hal/hal_config.h"
#include "hal/hal_adc.h"
#include <Arduino.h>

static bool is_initialized = false;

/* ========== 私有辅助函数 ========== */

// 检查管理器是否已初始化
static inline sensor_result_t ensure_initialized() {
    return is_initialized ? SENSOR_OK : SENSOR_ERROR_NOT_INIT;
}

// 验证指针有效性
static inline sensor_result_t validate_pointer(void* ptr) {
    return ptr != nullptr ? SENSOR_OK : SENSOR_ERROR_READ_FAILED;
}

sensor_result_t sensor_manager_init() {
    hal_adc_init();
    is_initialized = true;
    LOG_INFO("Sensor", "Sensor manager initialized");
    return SENSOR_OK;
}

sensor_result_t sensor_manager_read_all(sensor_data_t* p_sensor_data) {
    sensor_result_t result = ensure_initialized();
    if (result != SENSOR_OK) return result;

    // 复用独立函数读取各项数据

    // 读取土壤湿度
    float humidity_value;
    result = sensor_manager_get_humidity(&humidity_value);
    if (result != SENSOR_OK) {
        return result;
    }
    p_sensor_data->soil_moisture = (uint16_t)humidity_value;

    // 读取电池电压
    result = sensor_manager_get_battery_voltage(&p_sensor_data->battery_voltage);
    if (result != SENSOR_OK) {
        return result;
    }

    // 填充时间戳 (暂时为0)
    p_sensor_data->timestamp = 0;

    return SENSOR_OK;
}

sensor_result_t sensor_manager_get_humidity(float* p_humidity) {
    sensor_result_t result = ensure_initialized();
    if (result != SENSOR_OK) return result;

    result = validate_pointer(p_humidity);
    if (result != SENSOR_OK) return result;

    // 1. 打开传感器电源
    if (power_sensor_enable(true) != POWER_OK) {
        LOG_ERROR("Sensor", "Failed to enable sensor power");
        return SENSOR_ERROR_POWER_FAILED;
    }

    // 2. 延时等待传感器稳定
    delay(200); // 等待200ms

    // 3. 读取湿度传感器ADC值
    bool adc_success;
    uint16_t adc_value = hal_adc_read(PIN_SENSOR_HUMIDITY, &adc_success);

    // 检查ADC读取是否成功
    if (!adc_success) {
        LOG_ERROR("Sensor", "ADC read failed for humidity sensor");
        // 尝试关闭电源，即使ADC读取失败
        power_sensor_enable(false);
        return SENSOR_ERROR_READ_FAILED;
    }

    // 4. 关闭传感器电源
    if (power_sensor_enable(false) != POWER_OK) {
        // 即使关闭失败，也返回成功，因为数据已经读到
        LOG_WARN("Sensor", "Failed to disable sensor power after reading");
    }

    // 5. 转换为湿度值（直接返回ADC值，待后续标定）
    *p_humidity = (float)adc_value;

    return SENSOR_OK;
}

sensor_result_t sensor_manager_get_battery_voltage(float* p_voltage) {
    sensor_result_t result = ensure_initialized();
    if (result != SENSOR_OK) return result;

    result = validate_pointer(p_voltage);
    if (result != SENSOR_OK) return result;

    // 直接读取电池电压ADC值，无需控制传感器电源
    bool adc_success;
    uint16_t adc_value = hal_adc_read(PIN_SENSOR_BATTERY_ADC, &adc_success);

    // 检查ADC读取是否成功
    if (!adc_success) {
        LOG_ERROR("Sensor", "ADC read failed for battery voltage");
        return SENSOR_ERROR_READ_FAILED;
    }

    // 转换为实际电压
    float v_out = adc_value * (ADC_REFERENCE_VOLTAGE / 4095.0);
    *p_voltage = v_out * VOLTAGE_DIVIDER_RATIO;

    // 电压范围合理性检查（锂电池正常范围：2.7V - 4.3V）
    if (*p_voltage < 2.0f || *p_voltage > 5.0f) {
        LOG_WARN("Sensor", "Battery voltage reading (%.2fV) is out of reasonable range", *p_voltage);
        return SENSOR_ERROR_READ_FAILED;
    }

    return SENSOR_OK;
}