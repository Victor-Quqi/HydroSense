/**
 * @file hydro_config.h
 * @brief HydroSense完整配置结构定义（用于NVS持久化）
 */

#ifndef HYDRO_CONFIG_H
#define HYDRO_CONFIG_H

#include <stdint.h>

/**
 * @brief 浇水相关配置
 */
typedef struct {
    uint16_t threshold;         ///< 土壤湿度阈值 (ADC值, 0-4095)
    uint16_t duration_ms;       ///< 单次浇水持续时间 (毫秒)
    uint16_t min_interval_s;    ///< 最小浇水间隔 (秒)
    uint16_t power;             ///< 水泵功率/占空比 (0-255, PWM duty cycle)
    uint16_t humidity_wet;      ///< 湿润阈值 (ADC值, 用于百分比计算下限)
    uint16_t humidity_dry;      ///< 干燥阈值 (ADC值, 用于百分比计算上限)
    char plant_type[32];        ///< 植物类型 (如"绿萝", "多肉")
} hydro_watering_config_t;

/**
 * @brief WiFi连接配置
 */
typedef struct {
    char ssid[32];              ///< WiFi SSID
    char password[64];          ///< WiFi密码
    uint8_t auth_mode;          ///< 认证模式: 0=PSK, 1=WPA2-Enterprise
    char identity[64];          ///< WPA2-Enterprise身份标识
    char username[64];          ///< WPA2-Enterprise用户名
} hydro_wifi_config_t;

/**
 * @brief LLM API配置
 */
typedef struct {
    char base_url[128];         ///< API基础URL (如 https://api.openai.com/v1/chat/completions)
    char api_key[64];           ///< API密钥
    char model[32];             ///< 模型名称 (如 gpt-3.5-turbo)
} hydro_llm_config_t;

/**
 * @brief 系统配置
 */
typedef struct {
    bool ntp_enabled;           ///< 是否启用NTP时间同步
    char timezone[32];          ///< 时区 (如 "CST-8")
    char ntp_server[64];        ///< NTP服务器地址
} hydro_system_config_t;

/**
 * @brief HydroSense完整配置结构
 */
typedef struct {
    hydro_watering_config_t watering; ///< 浇水配置
    hydro_wifi_config_t wifi;         ///< WiFi配置
    hydro_llm_config_t llm;           ///< LLM配置
    hydro_system_config_t system;     ///< 系统配置
} hydro_config_t;

#endif // HYDRO_CONFIG_H
