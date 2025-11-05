/**
 * @file log_manager.h
 * @brief 日志管理器
 * @details 统一管理系统日志输出，支持分级和模块化
 */

#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include "hal/hal_config.h" // For TEST_MODE macro
#include <Arduino.h>

// --- 公共 API ---

/**
 * @brief 初始化日志管理器
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