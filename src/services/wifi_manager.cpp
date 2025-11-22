/**
 * @file wifi_manager.cpp
 * @brief WiFi连接管理器实现
 */

#include "wifi_manager.h"
#include "config_manager.h"
#include "../managers/log_manager.h"
#include <ArduinoJson.h>
#include "esp_wpa2.h"

WiFiManager* WiFiManager::s_instance = nullptr;

WiFiManager& WiFiManager::instance() {
    if (s_instance == nullptr) {
        s_instance = new WiFiManager();
    }
    return *s_instance;
}

WiFiManager::WiFiManager() :
    m_state(WifiState::IDLE),
    m_connect_start_time(0),
    m_connect_retry_count(0),
    m_next_retry_time(0),
    m_auto_reconnect_enabled(false),
    m_event_got_ip(false),
    m_event_disconnected(false),
    m_event_disconnect_reason(0),
    m_event_scan_done(false)
{
    memset(&m_current_config, 0, sizeof(m_current_config));
}

void WiFiManager::wifiEventHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (!s_instance) return;

    switch (event) {
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            s_instance->m_event_scan_done = true;
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            s_instance->m_event_got_ip = true;
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            s_instance->m_event_disconnect_reason = info.wifi_sta_disconnected.reason;
            s_instance->m_event_disconnected = true;
            break;

        default:
            break;
    }
}

bool WiFiManager::init() {
    m_state = WifiState::DISCONNECTED;

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    WiFi.onEvent(wifiEventHandler);

    LOG_INFO("WiFiManager", "WiFi manager initialized");
    return true;
}

void WiFiManager::update() {
    // 处理扫描完成事件
    if (m_event_scan_done) {
        m_event_scan_done = false;
        int16_t scanResultCount = WiFi.scanComplete();

        if (scanResultCount >= 0 && m_state == WifiState::SCANNING) {
            LOG_INFO("WiFiManager", "Scan complete, found %d networks", scanResultCount);
            m_scan_results.clear();
            m_scan_results.reserve(scanResultCount);

            for (int i = 0; i < scanResultCount; ++i) {
                WifiScanResult res;
                strncpy(res.ssid, WiFi.SSID(i).c_str(), sizeof(res.ssid) - 1);
                res.ssid[sizeof(res.ssid) - 1] = '\0';
                res.rssi = WiFi.RSSI(i);

                wifi_auth_mode_t esp_auth = WiFi.encryptionType(i);
                res.auth_mode = 0; // OPEN
                if (esp_auth == WIFI_AUTH_WPA2_PSK || esp_auth == WIFI_AUTH_WPA_PSK) {
                    res.auth_mode = 0; // WPA2_PSK
                } else if (esp_auth == WIFI_AUTH_WPA2_ENTERPRISE) {
                    res.auth_mode = 1; // WPA2_ENTERPRISE
                }

                m_scan_results.push_back(res);
            }

            WiFi.scanDelete();
            m_state = WifiState::SCAN_COMPLETE;
        }
    }

    // 状态机
    switch (m_state) {
        case WifiState::CONNECTING: {
            bool attemptFailed = false;

            // 检查是否获取IP
            if (m_event_got_ip) {
                m_event_got_ip = false;
                m_state = WifiState::CONNECTED;
                m_auto_reconnect_enabled = true;

                LOG_INFO("WiFiManager", "Connected! IP: %s (attempt %d/%d)",
                         WiFi.localIP().toString().c_str(),
                         m_connect_retry_count, WIFI_CONNECT_MAX_RETRIES);
                return;
            }

            // 检查是否连接失败
            if (m_event_disconnected) {
                m_event_disconnected = false;
                LOG_WARN("WiFiManager", "Connection attempt %d/%d failed, reason: %d",
                         m_connect_retry_count, WIFI_CONNECT_MAX_RETRIES,
                         (int)m_event_disconnect_reason);
                attemptFailed = true;
            }

            // 检查超时
            if (millis() - m_connect_start_time > WIFI_CONNECT_TIMEOUT_MS) {
                LOG_ERROR("WiFiManager", "Connection attempt %d/%d timed out",
                          m_connect_retry_count, WIFI_CONNECT_MAX_RETRIES);
                attemptFailed = true;
            }

            // 处理失败
            if (attemptFailed) {
                WiFi.disconnect();
                esp_wifi_sta_wpa2_ent_disable();

                if (m_connect_retry_count < WIFI_CONNECT_MAX_RETRIES) {
                    m_state = WifiState::WAITING_FOR_RETRY;
                    m_next_retry_time = millis() + WIFI_RETRY_DELAY_MS;
                } else {
                    m_state = WifiState::CONNECTION_FAILED;
                    m_auto_reconnect_enabled = false;
                    LOG_ERROR("WiFiManager", "All %d connection attempts failed",
                              (int)WIFI_CONNECT_MAX_RETRIES);
                }
            }
            break;
        }

        case WifiState::WAITING_FOR_RETRY: {
            if (millis() > m_next_retry_time) {
                attemptConnection();
            }
            break;
        }

        case WifiState::DISCONNECTED: {
            if (m_auto_reconnect_enabled) {
                LOG_INFO("WiFiManager", "Unexpected disconnect detected, auto-reconnecting...");
                connect(m_current_config);
            }
            break;
        }

        default:
            break;
    }

    // 处理已连接状态下的断连
    if (m_event_disconnected) {
        m_event_disconnected = false;
        if (m_state == WifiState::CONNECTED) {
            LOG_WARN("WiFiManager", "Connection lost, reason: %d",
                     (int)m_event_disconnect_reason);
            m_state = WifiState::DISCONNECTED;
            esp_wifi_sta_wpa2_ent_disable();
        }
    }
}

void WiFiManager::attemptConnection() {
    m_connect_retry_count++;
    LOG_INFO("WiFiManager", "Starting connection attempt %d/%d...",
             m_connect_retry_count, WIFI_CONNECT_MAX_RETRIES);

    m_state = WifiState::CONNECTING;
    m_connect_start_time = millis();
    m_event_disconnect_reason = 0;
    m_event_got_ip = false;

    WiFi.disconnect();

    // WPA2-Enterprise认证 (PEAP+MSCHAPv2)
    if (m_current_config.auth_mode == 1) { // WPA2_ENTERPRISE
        // 设置identity和username（通常在PEAP中identity用于匿名外层，username用于内层认证）
        // 如果identity不为空才设置（某些网络不需要outer identity）
        if (strlen(m_current_config.identity) > 0) {
            esp_wifi_sta_wpa2_ent_set_identity(
                (uint8_t*)m_current_config.identity,
                strlen(m_current_config.identity)
            );
        }
        esp_wifi_sta_wpa2_ent_set_username(
            (uint8_t*)m_current_config.username,
            strlen(m_current_config.username)
        );
        esp_wifi_sta_wpa2_ent_set_password(
            (uint8_t*)m_current_config.password,
            strlen(m_current_config.password)
        );

        // 启用WPA2-Enterprise（ESP32自动使用PEAP+MSCHAPv2作为默认方法）
        esp_wifi_sta_wpa2_ent_enable();

        LOG_INFO("WiFiManager", "Connecting with WPA2-Enterprise (PEAP+MSCHAPv2)");
        LOG_INFO("WiFiManager", "  SSID: %s", m_current_config.ssid);
        if (strlen(m_current_config.identity) > 0) {
            LOG_INFO("WiFiManager", "  Identity: %s", m_current_config.identity);
        }
        LOG_INFO("WiFiManager", "  Username: %s", m_current_config.username);

        WiFi.begin(m_current_config.ssid);
    } else {
        // WPA2-PSK认证
        LOG_INFO("WiFiManager", "Connecting with WPA2-PSK");
        LOG_INFO("WiFiManager", "  SSID: %s", m_current_config.ssid);
        WiFi.begin(m_current_config.ssid, m_current_config.password);
    }
}

bool WiFiManager::connect() {
    // 从ConfigManager读取配置
    hydro_wifi_config_t& wifi_config = ConfigManager::instance().getConfig().wifi;
    return connect(wifi_config);
}

bool WiFiManager::connect(const hydro_wifi_config_t& config) {
    if (m_state == WifiState::CONNECTING || m_state == WifiState::WAITING_FOR_RETRY) {
        LOG_WARN("WiFiManager", "Already connecting, ignoring request");
        return false;
    }

    m_current_config = config;
    m_connect_retry_count = 0;
    attemptConnection();
    return true;
}

void WiFiManager::disconnect() {
    m_auto_reconnect_enabled = false;
    WiFi.disconnect(true);
    m_state = WifiState::DISCONNECTED;
    LOG_INFO("WiFiManager", "Disconnected manually");
}

bool WiFiManager::startScan() {
    if (m_state == WifiState::SCANNING) {
        return false;
    }

    m_state = WifiState::SCANNING;
    m_scan_results.clear();
    WiFi.scanNetworks(true); // 异步扫描
    LOG_INFO("WiFiManager", "Started WiFi scan");
    return true;
}

bool WiFiManager::isConnected() {
    return m_state == WifiState::CONNECTED && WiFi.status() == WL_CONNECTED;
}

WifiState WiFiManager::getState() {
    return m_state;
}

String WiFiManager::getStatusJson() {
    JsonDocument doc;

    switch (m_state) {
        case WifiState::CONNECTED:
            doc["status"] = "CONNECTED";
            doc["ip"] = WiFi.localIP().toString();
            doc["ssid"] = WiFi.SSID();
            doc["rssi"] = WiFi.RSSI();
            break;

        case WifiState::CONNECTING:
        case WifiState::WAITING_FOR_RETRY:
            doc["status"] = "CONNECTING";
            doc["attempt"] = m_connect_retry_count;
            doc["max_attempts"] = (int)WIFI_CONNECT_MAX_RETRIES;
            break;

        case WifiState::CONNECTION_FAILED:
            doc["status"] = "CONNECTION_FAILED";
            doc["reason"] = m_event_disconnect_reason;
            break;

        case WifiState::SCANNING:
            doc["status"] = "SCANNING";
            break;

        case WifiState::SCAN_COMPLETE:
            doc["status"] = "SCAN_COMPLETE";
            doc["count"] = m_scan_results.size();
            break;

        default:
            doc["status"] = "DISCONNECTED";
            break;
    }

    String output;
    serializeJson(doc, output);
    return output;
}

uint16_t WiFiManager::getScanResultCount() {
    return m_scan_results.size();
}

const std::vector<WifiScanResult>& WiFiManager::getScanResults() {
    return m_scan_results;
}
