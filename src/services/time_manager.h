/**
 * @file time_manager.h
 * @brief 时间管理器 - 基于ESP32内部时间和NTP同步
 */

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <time.h>

/**
 * @brief 时间同步状态
 */
enum class TimeState {
    UNSYNCED,      // 未同步
    SYNCING,       // 同步中
    SYNCED,        // 已同步
    SYNC_FAILED    // 同步失败
};

/**
 * @brief 时间管理器单例类
 *
 * 功能特性:
 * - 使用ESP32内部时间管理（无外部RTC）
 * - NTP自动同步
 * - 时区支持
 * - 时间戳和格式化时间查询
 */
class TimeManager {
public:
    /**
     * @brief 获取单例实例
     */
    static TimeManager& instance();

    /**
     * @brief 初始化时间管理器
     * @return true 成功, false 失败
     */
    bool init();

    /**
     * @brief 触发NTP同步（需要WiFi已连接）
     * @return true 开始同步, false 无法同步
     */
    bool syncNTP();

    /**
     * @brief 检查时间是否已同步
     * @return true 已同步, false 未同步
     */
    bool isTimeSynced();

    /**
     * @brief 获取当前Unix时间戳
     * @return Unix时间戳（秒），如果未同步返回0
     */
    time_t getTimestamp();

    /**
     * @brief 获取格式化时间字符串
     * @param buffer 输出缓冲区
     * @param size 缓冲区大小
     * @param format 时间格式（默认ISO8601）
     * @return true 成功, false 失败
     */
    bool getTimeString(char* buffer, size_t size, const char* format = "%Y-%m-%d %H:%M:%S");

    /**
     * @brief 手动设置时间（用于无网络环境）
     * @param timestamp Unix时间戳（秒）
     */
    void setTime(time_t timestamp);

    /**
     * @brief 获取时间状态
     * @return 状态枚举
     */
    TimeState getState();

    /**
     * @brief 返回状态JSON字符串
     * @return JSON格式的状态信息
     */
    String getStatusJson();

private:
    TimeManager();
    TimeManager(const TimeManager&) = delete;
    TimeManager& operator=(const TimeManager&) = delete;

    /**
     * @brief 配置NTP和时区
     */
    void configureNTP();

    /**
     * @brief 等待时间同步完成
     * @param timeout_ms 超时时间（毫秒）
     * @return true 同步成功, false 超时
     */
    bool waitForSync(unsigned long timeout_ms);

    // 状态变量
    TimeState m_state;
    time_t m_last_sync_time;

    // 常量
    static const unsigned long NTP_SYNC_TIMEOUT_MS = 10000;
    static TimeManager* s_instance;
};

#endif // TIME_MANAGER_H
