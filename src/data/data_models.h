/**
 * @file data_models.h
 * @brief 定义系统中使用的核心数据结构
 */

#ifndef DATA_MODELS_H
#define DATA_MODELS_H

#include <stdint.h>

/**
 * @brief 存储一次传感器读数的数据结构
 */
typedef struct {
    uint32_t timestamp;         ///< 读数的时间戳 (Unix time)
    float battery_voltage;      ///< 电池电压 (V)
    uint16_t soil_moisture;     ///< 土壤湿度原始读数 (ADC value)
} sensor_data_t;

/**
 * @brief 系统核心配置参数
 */
typedef struct {
    uint8_t soil_moisture_threshold; ///< 触发浇水的土壤湿度阈值
    uint16_t watering_duration_ms;   ///< 单次浇水持续时间 (ms)
    char plant_type;             ///< 植物类型 (用于AI分析)
} SystemConfig;

#endif // DATA_MODELS_H