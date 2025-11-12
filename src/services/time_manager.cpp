/**
 * @file time_manager.cpp
 * @brief 时间管理器实现
 */

#include "time_manager.h"
#include "wifi_manager.h"
#include "log_manager.h"
#include "esp_sntp.h"
#include <string.h>

static bool s_ntp_initialized = false;
static time_t s_last_sync = 0;
static const char* NTP_SERVERS[] = {
    "pool.ntp.org",
    "time.nist.gov",
    NULL // 终止符
};

// NTP同步完成回调
static void sntp_sync_callback(struct timeval* tv) {
    s_last_sync = tv->tv_sec;
    struct tm timeinfo;
    if (time_get_utc(&timeinfo)) {
        LOG_INFO("Time", "NTP sync completed: %04d-%02d-%02d %02d:%02d:%02d",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }
}

void time_manager_init(void) {
    // 初始化SNTP
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, (char*)NTP_SERVERS[0]);
    sntp_setservername(1, (char*)NTP_SERVERS[1]);
    sntp_set_time_sync_notification_cb(sntp_sync_callback);
    
    s_ntp_initialized = true;
    LOG_DEBUG("Time", "Manager initialized");
}

bool time_sync_ntp(void) {
    if (!s_ntp_initialized) {
        LOG_ERROR("Time", "Not initialized");
        return false;
    }

    if (wifi_get_state() != WIFI_CONNECTED) {
        LOG_WARN("Time", "Cannot sync - Wi-Fi not connected");
        return false;
    }

    // 启动SNTP客户端
    sntp_init();
    
    // 等待同步（超时5秒）
    uint32_t timeout = 5000;
    uint32_t start = millis();
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && 
           (millis() - start) < timeout) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    if (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
        LOG_ERROR("Time", "NTP sync timed out");
        return false;
    }
    return true;
}

bool time_get_utc(struct tm* timeinfo) {
    if (!timeinfo) return false;
    
    time_t now;
    time(&now);
    return gmtime_r(&now, timeinfo) != NULL;
}

bool time_get_local(struct tm* timeinfo, int32_t gmt_offset_sec) {
    if (!timeinfo) return false;
    
    time_t now;
    time(&now);
    now += gmt_offset_sec; // 应用时区偏移
    return gmtime_r(&now, timeinfo) != NULL;
}

time_t time_get_timestamp(void) {
    time_t now;
    time(&now);
    return now;
}

void time_loop(void) {
    // 周期性同步（每24小时）
    if (s_ntp_initialized && 
        wifi_get_state() == WIFI_CONNECTED &&
        (time_get_timestamp() - s_last_sync) > 86400) { // 24h = 86400s
        LOG_INFO("Time", "Performing daily NTP sync");
        time_sync_ntp();
    }
}