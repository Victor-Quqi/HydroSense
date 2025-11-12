/**
 * @file wifi_manager.cpp
 * @brief Wi-Fi连接管理器实现
 */

#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "managers/log_manager.h"

// 静态状态变量
static wifi_state_t s_wifi_state = WIFI_DISCONNECTED;
static char s_ssid[32] = {0};
static char s_password[64] = {0};
static int8_t s_rssi = 0;

// 事件处理回调
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                s_wifi_state = WIFI_CONNECTING;
                LOG_INFO("WiFi", "Starting station mode...");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                s_wifi_state = WIFI_DISCONNECTED;
                LOG_WARN("WiFi", "Disconnected. Reconnecting...");
                esp_wifi_connect();
                s_wifi_state = WIFI_CONNECTING;
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        LOG_INFO("WiFi", "Connected. IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_wifi_state = WIFI_CONNECTED;
        
        // 获取信号强度
        esp_wifi_sta_get_rssi(&s_rssi);
        LOG_DEBUG("WiFi", "RSSI: %d dBm", s_rssi);
    }
}

void wifi_manager_init(void) {
    // 初始化TCP/IP栈
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // 配置Wi-Fi参数
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // 注册事件处理
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    s_wifi_state = WIFI_DISCONNECTED;
    LOG_DEBUG("WiFi", "Manager initialized");
}

void wifi_connect(const char* ssid, const char* password) {
    if (!ssid || !password) {
        LOG_ERROR("WiFi", "Invalid SSID or password");
        return;
    }

    // 保存凭据
    strncpy(s_ssid, ssid, sizeof(s_ssid)-1);
    strncpy(s_password, password, sizeof(s_password)-1);

    // 配置连接参数
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
        },
    };
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid)-1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password)-1);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_wifi_connect();
    s_wifi_state = WIFI_CONNECTING;
}

void wifi_disconnect(void) {
    esp_wifi_disconnect();
    s_wifi_state = WIFI_DISCONNECTED;
    LOG_INFO("WiFi", "Disconnected manually");
}

wifi_state_t wifi_get_state(void) {
    return s_wifi_state;
}

void wifi_loop(void) {
    // 处理周期性任务（如信号强度更新）
    if (s_wifi_state == WIFI_CONNECTED) {
        static uint32_t last_rssi_update = 0;
        if (millis() - last_rssi_update > 5000) { // 每5秒更新一次
            esp_wifi_sta_get_rssi(&s_rssi);
            last_rssi_update = millis();
        }
    }
}

int8_t wifi_get_rssi(void) {
    return s_rssi;
}