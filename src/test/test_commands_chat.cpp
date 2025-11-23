/**
 * @file test_commands_chat.cpp
 * @brief 对话历史测试命令实现
 */

#include "test_commands_chat.h"
#include "test_command_registry.h"
#include "../services/llm_connector.h"
#include "../services/history_manager.h"
#include <Arduino.h>

#ifdef TEST_MODE

// --- 子命令处理函数 ---

/**
 * @brief 显示对话历史
 */
static void handle_history(const char* args) {
    const auto& history = HistoryManager::instance().getHistory();
    size_t count = history.size();

    Serial.println("{");
    Serial.print("  \"status\": \"success\",");
    Serial.print("  \"count\": ");
    Serial.print(count);
    Serial.println(",");
    Serial.println("  \"history\": [");

    for (size_t i = 0; i < count; i++) {
        const auto& turn = history[i];

        Serial.println("    {");
        Serial.print("      \"user\": \"");
        Serial.print(turn.user_msg);
        Serial.println("\",");
        Serial.print("      \"plant\": \"");
        Serial.print(turn.plant_msg);
        Serial.println("\",");
        Serial.print("      \"timestamp\": ");
        Serial.print(turn.timestamp);
        Serial.println(",");
        Serial.println("      \"options\": [");

        for (uint8_t j = 0; j < turn.option_count; j++) {
            Serial.print("        \"");
            Serial.print(turn.options[j]);
            Serial.print("\"");
            if (j < turn.option_count - 1) {
                Serial.println(",");
            } else {
                Serial.println();
            }
        }

        Serial.print("      ]");
        Serial.println();
        Serial.print("    }");

        if (i < count - 1) {
            Serial.println(",");
        } else {
            Serial.println();
        }
    }

    Serial.println("  ]");
    Serial.println("}");
}

/**
 * @brief 清空对话历史
 */
static void handle_clear(const char* args) {
    HistoryManager::instance().clear();
    Serial.println("{\"status\": \"success\", \"message\": \"History cleared\"}");
}

/**
 * @brief 发送带选项的对话请求
 * 用法: chat ask <message>
 */
static void handle_ask(const char* args) {
    if (strlen(args) == 0) {
        Serial.println("{\"status\": \"error\", \"message\": \"Usage: chat ask <message>\"}");
        return;
    }

    char response[512];
    char options[3][64];
    uint8_t option_count = 0;

    Serial.print("{\"status\": \"info\", \"message\": \"Asking: ");
    Serial.print(args);
    Serial.println("\"}");

    bool success = LLMConnector::instance().chatWithOptions(
        args, response, sizeof(response), options, &option_count
    );

    if (success) {
        Serial.println("{");
        Serial.println("  \"status\": \"success\",");
        Serial.print("  \"response\": \"");
        Serial.print(response);
        Serial.println("\",");
        Serial.println("  \"options\": [");

        for (uint8_t i = 0; i < option_count; i++) {
            Serial.print("    \"");
            Serial.print(options[i]);
            Serial.print("\"");
            if (i < option_count - 1) {
                Serial.println(",");
            } else {
                Serial.println();
            }
        }

        Serial.println("  ]");
        Serial.println("}");
    } else {
        Serial.print("{\"status\": \"error\", \"message\": \"");
        Serial.print(LLMConnector::instance().getLastError());
        Serial.println("\"}");
    }
}

/**
 * @brief 保存历史到SPIFFS
 */
static void handle_save(const char* args) {
    bool success = HistoryManager::instance().save();
    if (success) {
        Serial.println("{\"status\": \"success\", \"message\": \"History saved to SPIFFS\"}");
    } else {
        Serial.println("{\"status\": \"error\", \"message\": \"Failed to save history\"}");
    }
}

/**
 * @brief 从SPIFFS加载历史
 */
static void handle_load(const char* args) {
    bool success = HistoryManager::instance().load();
    if (success) {
        Serial.print("{\"status\": \"success\", \"message\": \"History loaded\", \"count\": ");
        Serial.print(HistoryManager::instance().getHistoryCount());
        Serial.println("}");
    } else {
        Serial.println("{\"status\": \"error\", \"message\": \"Failed to load history\"}");
    }
}

// --- 主命令处理函数 ---

/**
 * @brief 处理 "chat" 命令（内部分发子命令）
 * @param args 子命令及其参数
 */
void handle_chat(const char* args) {
    // 解析子命令
    if (strlen(args) == 0) {
        Serial.println("{\"status\": \"error\", \"message\": \"Usage: chat <history|clear|ask|save|load>\"}");
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
    if (subcmd == "history") {
        handle_history(subcmd_args.c_str());
    }
    else if (subcmd == "clear") {
        handle_clear(subcmd_args.c_str());
    }
    else if (subcmd == "ask") {
        handle_ask(subcmd_args.c_str());
    }
    else if (subcmd == "save") {
        handle_save(subcmd_args.c_str());
    }
    else if (subcmd == "load") {
        handle_load(subcmd_args.c_str());
    }
    else {
        Serial.print("{\"status\": \"error\", \"message\": \"Unknown subcommand: ");
        Serial.print(subcmd);
        Serial.println("\"}");
    }
}

// --- 命令定义 ---

static const CommandRegistryEntry chat_commands[] = {
    {"chat", handle_chat, "Manages conversation history. Usage: chat <history|clear|ask|save|load>"}
};

// --- 公共 API ---

void test_commands_chat_init() {
    test_registry_register_commands(chat_commands, sizeof(chat_commands) / sizeof(chat_commands[0]));
}

#endif // TEST_MODE
