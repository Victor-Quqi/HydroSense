/**
 * @file time_manager.h
 * @brief 时间管理器
 * @details 专职处理NTP时间同步和系统时间管理
 */

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <time.h>
#include <stdbool.h>

// 初始化时间管理器
void time_manager_init(void);

// 触发NTP时间同步（需Wi-Fi连接）
bool time_sync_ntp(void);

// 获取当前时间（UTC）
bool time_get_utc(struct tm* timeinfo);

// 获取本地时间（带时区偏移）
bool time_get_local(struct tm* timeinfo, int32_t gmt_offset_sec);

// 获取当前时间戳（秒级）
time_t time_get_timestamp(void);

// 主循环中调用，处理时间同步逻辑
void time_loop(void);

#endif // TIME_MANAGER_H