/**
 * @file test_commands_wifi.cpp
 * @brief WiFi管理测试命令实现
 */

#include "test_commands_wifi.h"
#include "test_command_registry.h"
#include "../services/wifi_manager.h"
#include "../services/config_manager.h"
#include <Arduino.h>

#ifdef TEST_MODE

// --- 子命令处理函数 ---

/**
 * @brief 显示WiFi状态
 */
static void handle_status(const char* args) {
    String status_json = WiFiManager::instance().getStatusJson();
    Serial.println(status_json);
}

/**
 * @brief 连接WiFi
 * 用法: wifi connect [ssid] [password]
 * 如果不提供参数，使用ConfigManager中的配置
 */
static void handle_connect(const char* args) {
    if (strlen(args) == 0) {
        // 使用ConfigManager中的配置
        bool success = WiFiManager::instance().connect();
        if (success) {
            Serial.println("{\"status\": \"success\", \"message\": \"Connecting using saved config\"}");
        } else {
            Serial.println("{\"status\": \"error\", \"message\": \"Already connecting\"}");
        }
        return;
    }

    // 解析参数：ssid password
    char ssid[32] = {0};
    char password[64] = {0};

    const char* space_pos = strchr(args, ' ');
    if (space_pos == nullptr) {
        Serial.println("{\"status\": \"error\", \"message\": \"Usage: wifi connect [ssid] [password]\"}");
        return;
    }

    // 提取SSID
    size_t ssid_len = space_pos - args;
    if (ssid_len >= sizeof(ssid)) {
        Serial.println("{\"status\": \"error\", \"message\": \"SSID too long\"}");
        return;
    }
    strncpy(ssid, args, ssid_len);
    ssid[ssid_len] = '\0';

    // 提取密码
    const char* password_start = space_pos + 1;
    // 去除引号（如果有）
    if (*password_start == '"') {
        password_start++;
        const char* quote_end = strchr(password_start, '"');
        if (quote_end != nullptr) {
            size_t pass_len = quote_end - password_start;
            strncpy(password, password_start, pass_len);
            password[pass_len] = '\0';
        } else {
            strncpy(password, password_start, sizeof(password) - 1);
        }
    } else {
        strncpy(password, password_start, sizeof(password) - 1);
    }

    // 创建临时配置并连接
    hydro_wifi_config_t temp_config;
    memset(&temp_config, 0, sizeof(temp_config));
    strncpy(temp_config.ssid, ssid, sizeof(temp_config.ssid) - 1);
    strncpy(temp_config.password, password, sizeof(temp_config.password) - 1);
    temp_config.auth_mode = 0; // WPA2-PSK

    bool success = WiFiManager::instance().connect(temp_config);
    if (success) {
        Serial.print("{\"status\": \"success\", \"message\": \"Connecting to ");
        Serial.print(ssid);
        Serial.println("\"}");
    } else {
        Serial.println("{\"status\": \"error\", \"message\": \"Already connecting\"}");
    }
}

/**
 * @brief 断开WiFi连接
 */
static void handle_disconnect(const char* args) {
    WiFiManager::instance().disconnect();
    Serial.println("{\"status\": \"success\", \"message\": \"Disconnected\"}");
}

/**
 * @brief 扫描WiFi网络
 */
static void handle_scan(const char* args) {
    bool success = WiFiManager::instance().startScan();
    if (success) {
        Serial.println("{\"status\": \"success\", \"message\": \"Scanning...\"}");
    } else {
        Serial.println("{\"status\": \"error\", \"message\": \"Already scanning\"}");
    }
}

/**
 * @brief 显示扫描结果
 */
static void handle_scan_results(const char* args) {
    const auto& results = WiFiManager::instance().getScanResults();

    Serial.println("{");
    Serial.println("  \"status\": \"success\",");
    Serial.print("  \"count\": ");
    Serial.print(results.size());
    Serial.println(",");
    Serial.println("  \"networks\": [");

    for (size_t i = 0; i < results.size(); i++) {
        Serial.print("    {\"ssid\": \"");
        Serial.print(results[i].ssid);
        Serial.print("\", \"rssi\": ");
        Serial.print(results[i].rssi);
        Serial.print(", \"auth\": ");
        Serial.print(results[i].auth_mode);
        Serial.print("}");
        if (i < results.size() - 1) {
            Serial.println(",");
        } else {
            Serial.println();
        }
    }

    Serial.println("  ]");
    Serial.println("}");
}

// --- 主命令处理函数 ---

/**
 * @brief 处理 "wifi" 命令（内部分发子命令）
 * @param args 子命令及其参数
 */
void handle_wifi(const char* args) {
    // 解析子命令
    if (strlen(args) == 0) {
        Serial.println("{\"status\": \"error\", \"message\": \"Usage: wifi <status|connect|disconnect|scan|results>\"}");
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
    if (subcmd == "status") {
        handle_status(subcmd_args.c_str());
    }
    else if (subcmd == "connect") {
        handle_connect(subcmd_args.c_str());
    }
    else if (subcmd == "disconnect") {
        handle_disconnect(subcmd_args.c_str());
    }
    else if (subcmd == "scan") {
        handle_scan(subcmd_args.c_str());
    }
    else if (subcmd == "results") {
        handle_scan_results(subcmd_args.c_str());
    }
    else {
        Serial.print("{\"status\": \"error\", \"message\": \"Unknown subcommand: ");
        Serial.print(subcmd);
        Serial.println("\"}");
    }
}

// --- 命令定义 ---

static const CommandRegistryEntry wifi_commands[] = {
    {"wifi", handle_wifi, "Manages WiFi connections. Usage: wifi <status|connect|disconnect|scan|results>"}
};

// --- 公共 API ---

void test_commands_wifi_init() {
    test_registry_register_commands(wifi_commands, sizeof(wifi_commands) / sizeof(wifi_commands[0]));
}

#endif // TEST_MODE
