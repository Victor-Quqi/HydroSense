/**
 * @file test_commands_config.cpp
 * @brief 配置管理测试命令实现
 */

#include "test_commands_config.h"
#include "test_command_registry.h"
#include "../services/config_manager.h"
#include <Arduino.h>

#ifdef TEST_MODE

// --- 子命令处理函数 ---

/**
 * @brief 显示当前配置（JSON格式）
 */
static void handle_show(const char* args) {
    Serial.println("{");
    Serial.println("  \"status\": \"success\",");
    Serial.println("  \"config\": ");
    String config_json = ConfigManager::instance().getConfigJson();
    Serial.print(config_json);
    Serial.println("}");
}

/**
 * @brief 保存当前配置到NVS
 */
static void handle_save(const char* args) {
    bool success = ConfigManager::instance().saveConfig();

    Serial.print("{\"status\": \"");
    Serial.print(success ? "success" : "error");
    Serial.print("\", \"message\": \"");
    Serial.print(success ? "Config saved to NVS" : "Failed to save config");
    Serial.println("\"}");
}

/**
 * @brief 重置为默认配置
 */
static void handle_reset(const char* args) {
    bool success = ConfigManager::instance().resetToDefault();

    Serial.print("{\"status\": \"");
    Serial.print(success ? "success" : "error");
    Serial.print("\", \"message\": \"");
    Serial.print(success ? "Config reset to default" : "Failed to reset config");
    Serial.println("\"}");
}

/**
 * @brief 设置配置项
 * 用法: config set <key> <value>
 */
static void handle_set(const char* args) {
    if (strlen(args) == 0) {
        Serial.println("{\"status\": \"error\", \"message\": \"Usage: config set <key> <value>\"}");
        return;
    }

    // 解析参数：key value
    char key[64] = {0};
    char value[256] = {0};

    // 查找第一个空格
    const char* space_pos = strchr(args, ' ');
    if (space_pos == nullptr) {
        Serial.println("{\"status\": \"error\", \"message\": \"Missing value argument\"}");
        return;
    }

    // 提取key
    size_t key_len = space_pos - args;
    if (key_len >= sizeof(key)) {
        Serial.println("{\"status\": \"error\", \"message\": \"Key too long\"}");
        return;
    }
    strncpy(key, args, key_len);
    key[key_len] = '\0';

    // 提取value（跳过空格）
    const char* value_start = space_pos + 1;
    // 去除首尾引号（如果有）
    if (*value_start == '"') {
        value_start++;
        const char* quote_end = strchr(value_start, '"');
        if (quote_end != nullptr) {
            size_t value_len = quote_end - value_start;
            strncpy(value, value_start, value_len);
            value[value_len] = '\0';
        } else {
            strncpy(value, value_start, sizeof(value) - 1);
        }
    } else {
        strncpy(value, value_start, sizeof(value) - 1);
    }

    // 获取配置引用
    hydro_config_t& config = ConfigManager::instance().getConfig();
    bool found = false;

    // 根据key设置值
    if (strcmp(key, "watering.threshold") == 0) {
        config.watering.threshold = atoi(value);
        found = true;
    }
    else if (strcmp(key, "watering.duration_ms") == 0) {
        config.watering.duration_ms = atoi(value);
        found = true;
    }
    else if (strcmp(key, "watering.min_interval_s") == 0) {
        config.watering.min_interval_s = atoi(value);
        found = true;
    }
    else if (strcmp(key, "watering.plant_type") == 0) {
        strncpy(config.watering.plant_type, value, sizeof(config.watering.plant_type) - 1);
        config.watering.plant_type[sizeof(config.watering.plant_type) - 1] = '\0';
        found = true;
    }
    else if (strcmp(key, "wifi.ssid") == 0) {
        strncpy(config.wifi.ssid, value, sizeof(config.wifi.ssid) - 1);
        config.wifi.ssid[sizeof(config.wifi.ssid) - 1] = '\0';
        found = true;
    }
    else if (strcmp(key, "wifi.password") == 0) {
        strncpy(config.wifi.password, value, sizeof(config.wifi.password) - 1);
        config.wifi.password[sizeof(config.wifi.password) - 1] = '\0';
        found = true;
    }
    else if (strcmp(key, "wifi.auth_mode") == 0) {
        config.wifi.auth_mode = atoi(value);
        found = true;
    }
    else if (strcmp(key, "wifi.identity") == 0) {
        strncpy(config.wifi.identity, value, sizeof(config.wifi.identity) - 1);
        config.wifi.identity[sizeof(config.wifi.identity) - 1] = '\0';
        found = true;
    }
    else if (strcmp(key, "wifi.username") == 0) {
        strncpy(config.wifi.username, value, sizeof(config.wifi.username) - 1);
        config.wifi.username[sizeof(config.wifi.username) - 1] = '\0';
        found = true;
    }
    else if (strcmp(key, "llm.base_url") == 0) {
        strncpy(config.llm.base_url, value, sizeof(config.llm.base_url) - 1);
        config.llm.base_url[sizeof(config.llm.base_url) - 1] = '\0';
        found = true;
    }
    else if (strcmp(key, "llm.api_key") == 0) {
        strncpy(config.llm.api_key, value, sizeof(config.llm.api_key) - 1);
        config.llm.api_key[sizeof(config.llm.api_key) - 1] = '\0';
        found = true;
    }
    else if (strcmp(key, "llm.model") == 0) {
        strncpy(config.llm.model, value, sizeof(config.llm.model) - 1);
        config.llm.model[sizeof(config.llm.model) - 1] = '\0';
        found = true;
    }
    else if (strcmp(key, "system.ntp_enabled") == 0) {
        config.system.ntp_enabled = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        found = true;
    }
    else if (strcmp(key, "system.timezone") == 0) {
        strncpy(config.system.timezone, value, sizeof(config.system.timezone) - 1);
        config.system.timezone[sizeof(config.system.timezone) - 1] = '\0';
        found = true;
    }
    else if (strcmp(key, "system.ntp_server") == 0) {
        strncpy(config.system.ntp_server, value, sizeof(config.system.ntp_server) - 1);
        config.system.ntp_server[sizeof(config.system.ntp_server) - 1] = '\0';
        found = true;
    }

    if (found) {
        Serial.print("{\"status\": \"success\", \"message\": \"Set ");
        Serial.print(key);
        Serial.print(" = ");
        Serial.print(value);
        Serial.println("\", \"note\": \"Remember to run 'config save' to persist to NVS\"}");
    } else {
        Serial.print("{\"status\": \"error\", \"message\": \"Unknown config key: ");
        Serial.print(key);
        Serial.println("\"}");
    }
}

// --- 主命令处理函数 ---

/**
 * @brief 处理 "config" 命令（内部分发子命令）
 * @param args 子命令及其参数
 */
void handle_config(const char* args) {
    // 解析子命令
    if (strlen(args) == 0) {
        Serial.println("{\"status\": \"error\", \"message\": \"Usage: config <show|save|reset|set>\"}");
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
    else if (subcmd == "save") {
        handle_save(subcmd_args.c_str());
    }
    else if (subcmd == "reset") {
        handle_reset(subcmd_args.c_str());
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

static const CommandRegistryEntry config_commands[] = {
    {"config", handle_config, "Manages system configuration. Usage: config <show|save|reset|set>"}
};

// --- 公共 API ---

void test_commands_config_init() {
    test_registry_register_commands(config_commands, sizeof(config_commands) / sizeof(config_commands[0]));
}

#endif // TEST_MODE
