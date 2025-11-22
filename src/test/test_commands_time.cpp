/**
 * @file test_commands_time.cpp
 * @brief 时间管理测试命令实现
 */

#include "test_commands_time.h"
#include "test_command_registry.h"
#include "../services/time_manager.h"
#include <Arduino.h>

#ifdef TEST_MODE

// --- 子命令处理函数 ---

/**
 * @brief 显示时间状态
 */
static void handle_show(const char* args) {
    String status_json = TimeManager::instance().getStatusJson();
    Serial.println(status_json);
}

/**
 * @brief 触发NTP同步
 */
static void handle_sync(const char* args) {
    bool success = TimeManager::instance().syncNTP();
    if (success) {
        Serial.println("{\"status\": \"success\", \"message\": \"NTP sync successful\"}");
    } else {
        Serial.println("{\"status\": \"error\", \"message\": \"NTP sync failed\"}");
    }
}

/**
 * @brief 手动设置时间
 * 用法: time set <timestamp>
 */
static void handle_set(const char* args) {
    if (strlen(args) == 0) {
        Serial.println("{\"status\": \"error\", \"message\": \"Usage: time set <timestamp>\"}");
        return;
    }

    // 解析时间戳
    time_t timestamp = atol(args);
    if (timestamp < 1577836800) { // 2020-01-01
        Serial.println("{\"status\": \"error\", \"message\": \"Invalid timestamp (must be >= 1577836800)\"}");
        return;
    }

    TimeManager::instance().setTime(timestamp);
    Serial.println("{\"status\": \"success\", \"message\": \"Time set successfully\"}");
}

// --- 主命令处理函数 ---

/**
 * @brief 处理 "time" 命令（内部分发子命令）
 * @param args 子命令及其参数
 */
void handle_time(const char* args) {
    // 解析子命令
    if (strlen(args) == 0) {
        Serial.println("{\"status\": \"error\", \"message\": \"Usage: time <show|sync|set>\"}");
        return;
    }

    // 提取子命令
    String args_str = String(args);
    int first_space = args_str.indexOf(' ');
    String subcmd;
    String subcmd_args;

    if (first_space == -1) {
        subcmd = args_str;
        subcmd_args = "";
    } else {
        subcmd = args_str.substring(0, first_space);
        subcmd_args = args_str.substring(first_space + 1);
        subcmd_args.trim();
    }

    // 分发到子命令处理函数
    if (subcmd == "show") {
        handle_show(subcmd_args.c_str());
    }
    else if (subcmd == "sync") {
        handle_sync(subcmd_args.c_str());
    }
    else if (subcmd == "set") {
        handle_set(subcmd_args.c_str());
    }
    else {
        Serial.print("{\"status\": \"error\", \"message\": \"Unknown subcommand: ");
        Serial.print(subcmd);
        Serial.println("\"}");
    }
}

// --- 命令定义 ---

static const CommandRegistryEntry time_commands[] = {
    {"time", handle_time, "Manages system time. Usage: time <show|sync|set>"}
};

// --- 公共 API ---

void test_commands_time_init() {
    test_registry_register_commands(time_commands, sizeof(time_commands) / sizeof(time_commands[0]));
}

#endif // TEST_MODE
