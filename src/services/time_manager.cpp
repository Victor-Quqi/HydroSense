/**
 * @file time_manager.cpp
 * @brief 时间管理器实现
 */

#include "time_manager.h"
#include "config_manager.h"
#include "wifi_manager.h"
#include "../managers/log_manager.h"
#include <ArduinoJson.h>

TimeManager* TimeManager::s_instance = nullptr;

TimeManager& TimeManager::instance() {
    if (s_instance == nullptr) {
        s_instance = new TimeManager();
    }
    return *s_instance;
}

TimeManager::TimeManager() :
    m_state(TimeState::UNSYNCED),
    m_last_sync_time(0)
{
}

bool TimeManager::init() {
    LOG_INFO("TimeManager", "Time manager initializing...");

    // 从ConfigManager读取配置
    hydro_config_t& config = ConfigManager::instance().getConfig();

    if (config.system.ntp_enabled) {
        LOG_INFO("TimeManager", "NTP enabled, will sync when WiFi connected");
    } else {
        LOG_INFO("TimeManager", "NTP disabled in configuration");
    }

    return true;
}

void TimeManager::configureNTP() {
    hydro_config_t& config = ConfigManager::instance().getConfig();

    // 配置NTP服务器和时区
    // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer)
    // 由于使用POSIX时区字符串，gmtOffset设为0
    configTime(0, 0, config.system.ntp_server);

    // 设置时区（POSIX格式，例如"CST-8"表示UTC+8）
    setenv("TZ", config.system.timezone, 1);
    tzset();

    LOG_INFO("TimeManager", "NTP configured: server=%s, timezone=%s",
             config.system.ntp_server, config.system.timezone);
}

bool TimeManager::waitForSync(unsigned long timeout_ms) {
    unsigned long start = millis();

    while (millis() - start < timeout_ms) {
        time_t now = time(nullptr);
        // 检查时间是否有效（2020-01-01之后）
        if (now > 1577836800) {
            return true;
        }
        delay(100);
    }

    return false;
}

bool TimeManager::syncNTP() {
    // 检查WiFi连接
    if (!WiFiManager::instance().isConnected()) {
        LOG_ERROR("TimeManager", "WiFi not connected, cannot sync NTP");
        return false;
    }

    // 检查NTP是否启用
    hydro_config_t& config = ConfigManager::instance().getConfig();
    if (!config.system.ntp_enabled) {
        LOG_WARN("TimeManager", "NTP is disabled in configuration");
        return false;
    }

    LOG_INFO("TimeManager", "Starting NTP sync...");
    m_state = TimeState::SYNCING;

    // 配置NTP
    configureNTP();

    // 等待同步完成
    if (waitForSync(NTP_SYNC_TIMEOUT_MS)) {
        m_state = TimeState::SYNCED;
        m_last_sync_time = time(nullptr);

        char time_str[32];
        getTimeString(time_str, sizeof(time_str));
        LOG_INFO("TimeManager", "NTP sync successful: %s", time_str);
        return true;
    } else {
        m_state = TimeState::SYNC_FAILED;
        LOG_ERROR("TimeManager", "NTP sync timeout");
        return false;
    }
}

bool TimeManager::isTimeSynced() {
    if (m_state != TimeState::SYNCED) {
        return false;
    }

    // 检查时间是否有效
    time_t now = time(nullptr);
    return now > 1577836800; // 2020-01-01
}

time_t TimeManager::getTimestamp() {
    if (!isTimeSynced()) {
        return 0;
    }
    return time(nullptr);
}

bool TimeManager::getTimeString(char* buffer, size_t size, const char* format) {
    if (size == 0) return false;

    time_t now = time(nullptr);
    if (now < 1577836800) {
        snprintf(buffer, size, "NOT_SYNCED");
        return false;
    }

    struct tm timeinfo;
    if (!localtime_r(&now, &timeinfo)) {
        snprintf(buffer, size, "ERROR");
        return false;
    }

    size_t written = strftime(buffer, size, format, &timeinfo);
    return written > 0;
}

void TimeManager::setTime(time_t timestamp) {
    struct timeval tv;
    tv.tv_sec = timestamp;
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);

    m_state = TimeState::SYNCED;
    m_last_sync_time = timestamp;

    char time_str[32];
    getTimeString(time_str, sizeof(time_str));
    LOG_INFO("TimeManager", "Time manually set to: %s", time_str);
}

TimeState TimeManager::getState() {
    return m_state;
}

String TimeManager::getStatusJson() {
    JsonDocument doc;

    // 状态
    switch (m_state) {
        case TimeState::UNSYNCED:
            doc["status"] = "UNSYNCED";
            break;
        case TimeState::SYNCING:
            doc["status"] = "SYNCING";
            break;
        case TimeState::SYNCED:
            doc["status"] = "SYNCED";
            break;
        case TimeState::SYNC_FAILED:
            doc["status"] = "SYNC_FAILED";
            break;
    }

    // 当前时间
    time_t now = time(nullptr);
    if (now > 1577836800) {
        char time_str[32];
        getTimeString(time_str, sizeof(time_str));
        doc["time"] = time_str;
        doc["timestamp"] = now;
    } else {
        doc["time"] = "NOT_SYNCED";
        doc["timestamp"] = 0;
    }

    // 上次同步时间
    if (m_last_sync_time > 0) {
        doc["last_sync"] = m_last_sync_time;
    }

    // 配置信息
    hydro_config_t& config = ConfigManager::instance().getConfig();
    doc["ntp_enabled"] = config.system.ntp_enabled;
    doc["ntp_server"] = config.system.ntp_server;
    doc["timezone"] = config.system.timezone;

    String output;
    serializeJson(doc, output);
    return output;
}
