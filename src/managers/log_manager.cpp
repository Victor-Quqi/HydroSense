/**
 * @file log_manager.cpp
 * @brief 日志管理器实现（带时间戳和SPIFFS持久化）
 */

#include "log_manager.h"
#include "../services/time_manager.h"
#include <SPIFFS.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static bool spiffs_initialized = false;

/**
 * @brief 格式化时间戳字符串
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 */
static void format_timestamp(char* buffer, size_t size) {
    TimeManager& time_mgr = TimeManager::instance();
    
    if (time_mgr.isTimeSynced()) {
        // 时间已同步，显示绝对时间 HH:MM:SS
        time_t now = time_mgr.getTimestamp();
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &timeinfo);
    } else {
        // 时间未同步，显示启动后相对时间 +HH:MM:SS
        unsigned long ms = millis();
        unsigned long seconds = ms / 1000;
        unsigned long minutes = seconds / 60;
        unsigned long hours = minutes / 60;
        
        seconds %= 60;
        minutes %= 60;
        
        snprintf(buffer, size, "+%02lu:%02lu:%02lu", hours, minutes, seconds);
    }
}

/**
 * @brief 检查日志文件大小，执行滚动策略
 */
static void check_and_rotate_log() {
    if (!spiffs_initialized) return;
    
    File file = SPIFFS.open(LOG_FILE_PATH, FILE_READ);
    if (!file) return;
    
    size_t fileSize = file.size();
    file.close();
    
    // 超过500KB，执行滚动
    if (fileSize >= LOG_MAX_FILE_SIZE) {
        // 删除旧备份
        if (SPIFFS.exists(LOG_FILE_OLD_PATH)) {
            SPIFFS.remove(LOG_FILE_OLD_PATH);
        }
        
        // 重命名当前日志为旧日志
        SPIFFS.rename(LOG_FILE_PATH, LOG_FILE_OLD_PATH);
        
        // 下次写入时会创建新文件
    }
}

void log_manager_init() {
    // 初始化SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("[ERROR][LogManager] SPIFFS mount failed");
        return;
    }
    
    spiffs_initialized = true;
    Serial.println("[INFO][LogManager] SPIFFS initialized");
    
    // 检查日志文件大小
    check_and_rotate_log();
}

void log_manager_log(const char* level, const char* module, const char* format, ...) {
    char timestamp[32];
    char buffer[LOG_BUFFER_SIZE];
    char log_message[LOG_BUFFER_SIZE];
    char final_log[LOG_BUFFER_SIZE + 64];

    // 格式化时间戳
    format_timestamp(timestamp, sizeof(timestamp));

    // 格式化可变参数
    va_list args;
    va_start(args, format);
    vsnprintf(log_message, sizeof(log_message), format, args);
    va_end(args);

    // 组合最终的日志字符串
    snprintf(final_log, sizeof(final_log), "[%s][%s][%s] %s",
             timestamp, level, module, log_message);

    // 输出到串口
    if (Serial) {
        Serial.println(final_log);
    }

    // DEBUG日志不写入SPIFFS，只在串口输出
    bool is_debug = (strcmp(level, "DEBUG") == 0);

    if (spiffs_initialized && !is_debug) {
        File file = SPIFFS.open(LOG_FILE_PATH, FILE_APPEND);
        if (file) {
            file.println(final_log);
            file.close();
        }

        // 定期检查滚动（每100条日志检查一次，降低性能开销）
        static int log_count = 0;
        if (++log_count >= 100) {
            check_and_rotate_log();
            log_count = 0;
        }
    }
}

String log_manager_get_recent_logs(int count) {
    if (!spiffs_initialized) {
        return "SPIFFS not initialized";
    }
    
    File file = SPIFFS.open(LOG_FILE_PATH, FILE_READ);
    if (!file) {
        return "Log file not found";
    }
    
    size_t fileSize = file.size();
    
    // 策略：读取最后2KB（估算约20-30行）
    const size_t READ_TAIL_SIZE = 2048;
    size_t startPos = (fileSize > READ_TAIL_SIZE) ? (fileSize - READ_TAIL_SIZE) : 0;
    
    file.seek(startPos);
    String tail = file.readString();
    file.close();
    
    // 按行分割
    int lineCount = 0;
    int lastNewline = tail.length();
    String result = "";
    
    // 从尾部向前找换行符
    for (int i = tail.length() - 1; i >= 0 && lineCount < count; i--) {
        if (tail[i] == '\n') {
            if (lineCount > 0) {  // 跳过最后一个空行
                String line = tail.substring(i + 1, lastNewline);
                line.trim();
                if (line.length() > 0) {
                    result = line + "\n" + result;
                }
            }
            lastNewline = i;
            lineCount++;
        }
    }
    
    // 处理第一行（可能不完整，跳过）
    if (startPos > 0 && lineCount < count) {
        // 第一行可能被截断，跳过
    } else if (lineCount < count) {
        // 文件小于2KB，包含第一行
        String line = tail.substring(0, lastNewline);
        line.trim();
        if (line.length() > 0) {
            result = line + "\n" + result;
        }
    }
    
    return result.length() > 0 ? result : "No logs found";
}
