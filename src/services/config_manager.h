/**
 * @file config_manager.h
 * @brief 配置管理器 - 使用NVS持久化存储HydroSense配置
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include "../data/hydro_config.h"

/**
 * @brief 配置管理器类（单例模式）
 *
 * 使用ESP32 NVS（非易失性存储）保存配置，支持：
 * - 浇水参数持久化
 * - WiFi连接信息（含WPA2-Enterprise）
 * - LLM API配置
 * - 系统配置（NTP等）
 */
class ConfigManager {
public:
    /**
     * @brief 获取单例实例
     */
    static ConfigManager& instance();

    /**
     * @brief 初始化配置管理器
     * @return true 成功, false 失败
     */
    bool init();

    /**
     * @brief 从NVS加载配置（如果不存在则使用默认值）
     * @return true 成功, false 失败
     */
    bool loadConfig();

    /**
     * @brief 保存当前配置到NVS
     * @return true 成功, false 失败
     */
    bool saveConfig();

    /**
     * @brief 重置为默认配置并保存
     * @return true 成功, false 失败
     */
    bool resetToDefault();

    /**
     * @brief 获取当前配置（可修改）
     * @return 配置结构引用
     */
    hydro_config_t& getConfig();

    /**
     * @brief 获取配置的JSON字符串表示（用于调试）
     * @return JSON格式的配置字符串
     */
    String getConfigJson();

private:
    ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    /**
     * @brief 获取默认配置
     * @return 默认配置结构
     */
    hydro_config_t getDefaultConfig();

    hydro_config_t m_config;  ///< 当前配置
    bool m_initialized;       ///< 是否已初始化
};

#endif // CONFIG_MANAGER_H
