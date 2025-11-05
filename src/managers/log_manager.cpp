/**
 * @file log_manager.cpp
 * @brief 日志管理器实现
 */

#include "log_manager.h"
#include <stdio.h>
#include <stdarg.h>

#define LOG_BUFFER_SIZE 256

void log_manager_init() {
    // 目前初始化是空的，因为串口由main.cpp或test_cli.cpp初始化
    // 如果未来日志需要写入文件或网络，初始化代码将在这里添加
}

void log_manager_log(const char* level, const char* module, const char* format, ...) {
    // 仅在串口可用时输出 (避免在非测试模式下无串口时产生问题)
    if (Serial) {
        char buffer[LOG_BUFFER_SIZE];
        char log_message[LOG_BUFFER_SIZE];

        // 格式化可变参数
        va_list args;
        va_start(args, format);
        vsnprintf(log_message, sizeof(log_message), format, args);
        va_end(args);

        // 组合最终的日志字符串
        snprintf(buffer, sizeof(buffer), "[%s][%s] %s", level, module, log_message);

        // 通过串口输出
        Serial.println(buffer);
    }
}