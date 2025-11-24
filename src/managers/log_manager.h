/**
 * @file log_manager.h
 * @brief 日志管理器（带时间戳和SPIFFS持久化）
 * @details 统一管理系统日志输出，支持分级、模块化、时间戳和文件存储
 */

#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include "hal/hal_config.h" // For TEST_MODE macro
#include <Arduino.h>

// --- 配置常量 ---
#define LOG_FILE_PATH "/spiffs/system.log"
#define LOG_FILE_OLD_PATH "/spiffs/system.log.old"
#define LOG_MAX_FILE_SIZE (500 * 1024)  // 500KB
#define LOG_BUFFER_SIZE 256

// --- 公共 API ---

/**
 * @brief 初始化日志管理器
 * @details 挂载SPIFFS，准备日志文件
 */
void log_manager_init();

/**
 * @brief 核心日志记录函数 (不建议直接调用，请使用宏)
 * @param level 日志级别 ("INFO", "WARN", "ERROR", "DEBUG")
 * @param module 模块标签 (例如 "Sensor", "Power")
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void log_manager_log(const char* level, const char* module, const char* format, ...);

/**
 * @brief 获取最近N条日志
 * @param count 日志条数（默认20）
 * @return 日志字符串（每行一条，最多count行）
 */
String log_manager_get_recent_logs(int count = 20);

/**
 * @brief 立即将RAM缓冲区的日志写入SPIFFS
 * @details 用于关键时刻（如OFF模式）手动触发持久化
 */
void log_manager_flush_now();


// --- 日志宏定义 ---

// ERROR 级别日志 (始终启用)
#define LOG_ERROR(module, format, ...) log_manager_log("ERROR", module, format, ##__VA_ARGS__)

// WARN 级别日志 (始终启用)
#define LOG_WARN(module, format, ...) log_manager_log("WARN", module, format, ##__VA_ARGS__)

// INFO 级别日志 (始终启用)
#define LOG_INFO(module, format, ...) log_manager_log("INFO", module, format, ##__VA_ARGS__)

// DEBUG 级别日志 (仅在 TEST_MODE 下启用)
#ifdef TEST_MODE
    #define LOG_DEBUG(module, format, ...) log_manager_log("DEBUG", module, format, ##__VA_ARGS__)
#else
    #define LOG_DEBUG(module, format, ...) (void)0
#endif


#endif // LOG_MANAGER_H
