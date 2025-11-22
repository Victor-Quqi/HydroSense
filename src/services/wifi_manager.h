/**
 * @file wifi_manager.h
 * @brief WiFi连接管理器 - 支持WPA2-PSK和WPA2-Enterprise
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "../data/hydro_config.h"

/**
 * @brief WiFi状态枚举
 */
enum class WifiState {
    IDLE,
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    WAITING_FOR_RETRY,
    CONNECTION_FAILED,
    SCANNING,
    SCAN_COMPLETE
};

/**
 * @brief WiFi扫描结果
 */
struct WifiScanResult {
    char ssid[33];
    int8_t rssi;
    uint8_t auth_mode;
};

/**
 * @brief WiFi连接管理器单例类
 *
 * 功能特性:
 * - 支持WPA2-PSK和WPA2-Enterprise认证
 * - 自动重连机制
 * - WiFi网络扫描
 * - 连接状态管理
 */
class WiFiManager {
public:
    /**
     * @brief 获取单例实例
     */
    static WiFiManager& instance();

    /**
     * @brief 初始化WiFi管理器
     * @return true 成功, false 失败
     */
    bool init();

    /**
     * @brief 更新WiFi状态(在loop()中调用)
     */
    void update();

    /**
     * @brief 连接到WiFi(使用ConfigManager中的配置)
     * @return true 开始连接, false 已在连接中
     */
    bool connect();

    /**
     * @brief 使用指定配置连接WiFi
     * @param config WiFi配置
     * @return true 开始连接, false 已在连接中
     */
    bool connect(const hydro_wifi_config_t& config);

    /**
     * @brief 断开WiFi连接
     */
    void disconnect();

    /**
     * @brief 开始扫描WiFi网络
     * @return true 开始扫描, false 已在扫描中
     */
    bool startScan();

    /**
     * @brief 检查是否已连接
     * @return true 已连接, false 未连接
     */
    bool isConnected();

    /**
     * @brief 获取WiFi状态
     * @return 状态枚举
     */
    WifiState getState();

    /**
     * @brief 返回状态JSON字符串
     * @return JSON格式的状态信息
     */
    String getStatusJson();

    /**
     * @brief 获取扫描结果数量
     * @return 扫描到的网络数量
     */
    uint16_t getScanResultCount();

    /**
     * @brief 获取扫描结果
     * @return 扫描结果列表
     */
    const std::vector<WifiScanResult>& getScanResults();

private:
    WiFiManager();
    WiFiManager(const WiFiManager&) = delete;
    WiFiManager& operator=(const WiFiManager&) = delete;

    /**
     * @brief WiFi事件处理函数
     */
    static void wifiEventHandler(WiFiEvent_t event, WiFiEventInfo_t info);

    /**
     * @brief 尝试连接WiFi
     */
    void attemptConnection();

    // 状态变量
    WifiState m_state;
    hydro_wifi_config_t m_current_config;

    // 连接管理
    unsigned long m_connect_start_time;
    uint8_t m_connect_retry_count;
    unsigned long m_next_retry_time;
    bool m_auto_reconnect_enabled;

    // 事件标志
    volatile bool m_event_got_ip;
    volatile bool m_event_disconnected;
    volatile uint8_t m_event_disconnect_reason;
    volatile bool m_event_scan_done;

    // 扫描结果
    std::vector<WifiScanResult> m_scan_results;

    // 常量
    static const unsigned long WIFI_CONNECT_TIMEOUT_MS = 20000;
    static const uint8_t WIFI_CONNECT_MAX_RETRIES = 10;
    static const unsigned long WIFI_RETRY_DELAY_MS = 5000;

    static WiFiManager* s_instance;
};

#endif // WIFI_MANAGER_H
