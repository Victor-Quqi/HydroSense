/**
 * @file log_manager.cpp
 * @brief 日志管理器实现（RAM缓冲 + FreeRTOS异步写入）
 */

#include "log_manager.h"
#include "../services/time_manager.h"
#include <SPIFFS.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static bool spiffs_initialized = false;

// --- RAM环形缓冲区 ---
#define LOG_RAM_BUFFER_SIZE 100
static String log_ram_buffer[LOG_RAM_BUFFER_SIZE];
static uint16_t log_ram_head = 0;
static uint16_t log_ram_count = 0;
static portMUX_TYPE log_ram_mux = portMUX_INITIALIZER_UNLOCKED;

// --- FreeRTOS后台写入任务 ---
static TaskHandle_t s_log_write_task_handle = NULL;
static volatile bool s_flush_requested = false;

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

/**
 * @brief 将RAM缓冲区批量写入SPIFFS（后台任务调用）
 */
static void flush_ram_to_spiffs() {
    if (!spiffs_initialized) return;
    if (log_ram_count == 0) return;

    // 临界区：复制待写入的数据到临时缓冲区
    String temp_buffer[LOG_RAM_BUFFER_SIZE];
    uint16_t count_to_write = 0;

    portENTER_CRITICAL(&log_ram_mux);
    count_to_write = log_ram_count;
    uint16_t start_idx = (log_ram_head - log_ram_count + LOG_RAM_BUFFER_SIZE) % LOG_RAM_BUFFER_SIZE;

    // 复制String（在临界区内，但不做文件操作）
    for (uint16_t i = 0; i < count_to_write; i++) {
        uint16_t idx = (start_idx + i) % LOG_RAM_BUFFER_SIZE;
        temp_buffer[i] = log_ram_buffer[idx];
    }

    log_ram_count = 0; // 清空已复制的
    portEXIT_CRITICAL(&log_ram_mux);

    // 临界区外：批量写入文件（不持有锁）
    File file = SPIFFS.open(LOG_FILE_PATH, FILE_APPEND);
    if (!file) return;

    for (uint16_t i = 0; i < count_to_write; i++) {
        file.println(temp_buffer[i]);
    }

    file.close();

    // 检查文件大小，必要时滚动
    check_and_rotate_log();
}

/**
 * @brief FreeRTOS后台日志写入任务
 */
static void log_write_task(void* parameter) {
    TickType_t last_flush = xTaskGetTickCount();
    const TickType_t flush_interval = pdMS_TO_TICKS(10000); // 10秒自动写入

    while (true) {
        TickType_t now = xTaskGetTickCount();

        // 触发条件：手动请求 OR 10秒超时 OR RAM缓冲区>80%满
        bool should_flush = s_flush_requested ||
                           (now - last_flush >= flush_interval) ||
                           (log_ram_count >= LOG_RAM_BUFFER_SIZE * 0.8);

        if (should_flush) {
            flush_ram_to_spiffs();
            s_flush_requested = false;
            last_flush = now;
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // 每秒检查一次
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

    // 创建FreeRTOS后台日志写入任务
    BaseType_t task_created = xTaskCreate(
        log_write_task,
        "LogWrite",
        8192,  // Stack size (增加到8KB防止栈溢出)
        NULL,
        1,     // 低优先级（不影响主循环）
        &s_log_write_task_handle
    );

    if (task_created != pdPASS) {
        Serial.println("[ERROR][LogManager] Failed to create log write task");
    } else {
        Serial.println("[INFO][LogManager] Async log write task created");
    }
}

void log_manager_log(const char* level, const char* module, const char* format, ...) {
    char timestamp[32];
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

    // 输出到串口（立即）
    if (Serial) {
        Serial.println(final_log);
    }

    // DEBUG日志不写入RAM/SPIFFS，只在串口输出
    if (strcmp(level, "DEBUG") == 0) {
        return;
    }

    // 写入RAM缓冲区（快速，临界区保护）
    portENTER_CRITICAL(&log_ram_mux);
    log_ram_buffer[log_ram_head] = String(final_log);
    log_ram_head = (log_ram_head + 1) % LOG_RAM_BUFFER_SIZE;
    if (log_ram_count < LOG_RAM_BUFFER_SIZE) {
        log_ram_count++;
    }
    // 缓冲区满时，最老的日志会被覆盖（丢弃）
    portEXIT_CRITICAL(&log_ram_mux);
}

String log_manager_get_recent_logs(int count) {
    String result = "";

    portENTER_CRITICAL(&log_ram_mux);
    int available = min(count, (int)log_ram_count);

    // 从最新到最旧读取
    for (int i = 0; i < available; i++) {
        int idx = (log_ram_head - 1 - i + LOG_RAM_BUFFER_SIZE) % LOG_RAM_BUFFER_SIZE;
        result = log_ram_buffer[idx] + "\n" + result;
    }
    portEXIT_CRITICAL(&log_ram_mux);

    return result.length() > 0 ? result : "No logs in RAM";
}

void log_manager_flush_now() {
    s_flush_requested = true;
    // 可选：等待写入完成（最多1秒）
    unsigned long start = millis();
    while (s_flush_requested && (millis() - start < 1000)) {
        delay(10);
    }
}
