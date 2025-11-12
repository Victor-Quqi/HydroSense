/**
 * @file cloud_connector.h
 * @brief 云端连接器
 * @details 专职处理与云端API(LLM)的通信
 */

#ifndef CLOUD_CONNECTOR_H
#define CLOUD_CONNECTOR_H

#include <stdbool.h>
#include <stddef.h>

// 云端连接状态
typedef enum {
    CLOUD_DISCONNECTED = 0,
    CLOUD_CONNECTING,
    CLOUD_CONNECTED
} cloud_state_t;

// 传感器数据结构体（与云端交互格式）
typedef struct {
    time_t timestamp;      // 时间戳
    float soil_moisture;   // 土壤湿度 (%)
    float temperature;     // 温度 (°C)
    float battery_voltage; // 电池电压 (V)
} sensor_data_t;

// 初始化云端连接器
void cloud_connector_init(void);

// 连接到云端服务
void cloud_connect(const char* api_endpoint, const char* api_key);

// 断开云端连接
void cloud_disconnect(void);

// 获取当前连接状态
cloud_state_t cloud_get_state(void);

// 上报传感器数据到云端
bool cloud_send_sensor_data(const sensor_data_t* data);

// 从云端获取AI健康报告（LLM集成）
bool cloud_get_ai_report(char* buffer, size_t buffer_size);

// 主循环中调用，处理云端通信逻辑
void cloud_loop(void);

#endif // CLOUD_CONNECTOR_H