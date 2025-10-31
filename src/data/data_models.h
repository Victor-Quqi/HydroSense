/**
 * @file data_models.h
 * @brief 全局数据模型
 * @details 定义系统中使用的所有共享数据结构、枚举和类型
 */

#ifndef DATA_MODELS_H
#define DATA_MODELS_H

#include <stdint.h>

// ============================================================================
// 系统状态枚举（对应状态机设计）
// ============================================================================
typedef enum {
    SYSTEM_STATE_IDLE_SLEEPING,    // 深度睡眠状态（常态）
    SYSTEM_STATE_SENSING,          // 传感器数据采集状态
    SYSTEM_STATE_WATERING,         // 浇水执行状态
    SYSTEM_STATE_CONFIGURING,      // 配置调整状态（交互模式）
    SYSTEM_STATE_ERROR             // 错误状态
} SystemState;

// ============================================================================
// 传感器数据结构
// ============================================================================
typedef struct {
    uint16_t soil_moisture;        // 土壤湿度值（0-1023，ADC原始值）
    float battery_voltage;         // 电池电压（单位：V）
    uint32_t timestamp;            // 数据采集时间戳（秒级）
} SensorData;

// ============================================================================
// 系统配置参数结构（存储于NVS）
// ============================================================================
typedef struct {
    uint16_t watering_threshold;   // 浇水触发阈值（土壤湿度低于此值）
    uint16_t watering_duration;    // 单次浇水时长（单位：ms）
    uint32_t sampling_interval;    // 采样间隔（单位：秒，默认1800s=30分钟）
    bool auto_watering_enabled;    // 自动浇水使能标志
    uint8_t device_mode;           // 设备运行模式（0=RUN, 1=INTERACTIVE）
} SystemConfig;

// ============================================================================
// 系统事件标志（用于事件驱动机制）
// ============================================================================
typedef struct {
    bool sensor_data_ready;        // 传感器数据就绪标志
    bool watering_complete;        // 浇水完成标志
    bool config_updated;           // 配置更新标志
    bool manual_wakeup;            // 手动唤醒标志（编码器触发）
} SystemEvents;
#endif // DATA_MODELS_H