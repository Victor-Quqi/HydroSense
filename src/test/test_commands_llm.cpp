/**
 * @file test_commands_llm.cpp
 * @brief LLM连接测试命令实现
 */

#include "test_commands_llm.h"
#include "test_command_registry.h"
#include "../services/llm_connector.h"
#include <Arduino.h>

#ifdef TEST_MODE

// --- 子命令处理函数 ---

/**
 * @brief 显示LLM状态
 */
static void handle_status(const char* args) {
    String status_json = LLMConnector::instance().getStatusJson();
    Serial.println(status_json);
}

/**
 * @brief 发送测试消息
 */
static void handle_test(const char* args) {
    char response[512];

    Serial.println("{\"status\": \"info\", \"message\": \"Sending test message...\"}");

    bool success = LLMConnector::instance().chat("你好，植物！你现在感觉怎么样？", response, sizeof(response));

    if (success) {
        Serial.print("{\"status\": \"success\", \"response\": \"");
        Serial.print(response);
        Serial.println("\"}");
    } else {
        Serial.print("{\"status\": \"error\", \"message\": \"");
        Serial.print(LLMConnector::instance().getLastError());
        Serial.println("\"}");
    }
}

/**
 * @brief 发送自定义消息
 * 用法: llm chat <message>
 */
static void handle_chat(const char* args) {
    if (strlen(args) == 0) {
        Serial.println("{\"status\": \"error\", \"message\": \"Usage: llm chat <message>\"}");
        return;
    }

    char response[512];

    Serial.print("{\"status\": \"info\", \"message\": \"Sending: ");
    Serial.print(args);
    Serial.println("\"}");

    bool success = LLMConnector::instance().chat(args, response, sizeof(response));

    if (success) {
        Serial.print("{\"status\": \"success\", \"response\": \"");
        Serial.print(response);
        Serial.println("\"}");
    } else {
        Serial.print("{\"status\": \"error\", \"message\": \"");
        Serial.print(LLMConnector::instance().getLastError());
        Serial.println("\"}");
    }
}

// --- 主命令处理函数 ---

/**
 * @brief 处理 "llm" 命令（内部分发子命令）
 * @param args 子命令及其参数
 */
void handle_llm(const char* args) {
    // 解析子命令
    if (strlen(args) == 0) {
        Serial.println("{\"status\": \"error\", \"message\": \"Usage: llm <status|test|chat>\"}");
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
    else if (subcmd == "test") {
        handle_test(subcmd_args.c_str());
    }
    else if (subcmd == "chat") {
        handle_chat(subcmd_args.c_str());
    }
    else {
        Serial.print("{\"status\": \"error\", \"message\": \"Unknown subcommand: ");
        Serial.print(subcmd);
        Serial.println("\"}");
    }
}

// --- 命令定义 ---

static const CommandRegistryEntry llm_commands[] = {
    {"llm", handle_llm, "Manages LLM connection. Usage: llm <status|test|chat>"}
};

// --- 公共 API ---

void test_commands_llm_init() {
    test_registry_register_commands(llm_commands, sizeof(llm_commands) / sizeof(llm_commands[0]));
}

#endif // TEST_MODE
