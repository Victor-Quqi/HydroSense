/**
 * @file wifi_manager.h
 * @brief Wi-Fi连接管理器
 * @details 专职处理Wi-Fi的连接、断开和状态监控
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

// Wi-Fi连接状态
typedef enum {
    WIFI_DISCONNECTED = 0,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_ERROR
} wifi_state_t;

// 初始化Wi-Fi管理器
void wifi_manager_init(void);

// 连接到指定SSID
void wifi_connect(const char* ssid, const char* password);

// 断开Wi-Fi连接
void wifi_disconnect(void);

// 检查当前连接状态
wifi_state_t wifi_get_state(void);

// 主循环中调用，处理连接逻辑
void wifi_loop(void);

// 获取当前RSSI（信号强度）
int8_t wifi_get_rssi(void);

#endif // WIFI_MANAGER_H