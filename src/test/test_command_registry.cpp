/**
 * @file test_command_registry.cpp
 * @brief 测试命令注册器的实现
 */

#include "test_command_registry.h"
#include "test_cli.h" // For send_eot()

#ifdef TEST_MODE

// --- 私有变量 ---

// 定义命令注册表的最大容量
#define MAX_COMMANDS 20

// 命令注册表
static CommandRegistryEntry command_registry[MAX_COMMANDS];
// 当前已注册的命令数量
static size_t registered_command_count = 0;

// --- 内部辅助函数 ---

/**
 * @brief 查找命令并返回其处理函数
 * @param name 命令名称
 * @return CommandHandler 函数指针，如果未找到则为 nullptr
 */
const CommandRegistryEntry* find_command_entry(const char* name) {
    for (size_t i = 0; i < registered_command_count; ++i) {
        if (strcmp(command_registry[i].name, name) == 0) {
            return &command_registry[i];
        }
    }
    return nullptr;
}

/**
 * @brief "help" 命令的特殊处理函数
 */
void handle_help(const char* args) {
    if (args && *args) { // 如果有参数
        const CommandRegistryEntry* entry = find_command_entry(args);
        if (entry) {
            Serial.printf("Usage: %s\r\n", entry->help);
        } else {
            Serial.printf("Error: Command '%s' not found.\r\n", args);
        }
    } else { // 如果没有参数
        Serial.println("--- Available Commands ---");
        Serial.println("Type 'help <command>' for more details.");
        for (size_t i = 0; i < registered_command_count; ++i) {
            // 提取用法的第一个片段作为简短说明
            String help_str = String(command_registry[i].help);
            int first_sentence_end = help_str.indexOf('.');
            if (first_sentence_end != -1) {
                help_str = help_str.substring(0, first_sentence_end + 1);
            }
            Serial.printf("  %-10s - %s\r\n", command_registry[i].name, help_str.c_str());
        }
        Serial.println("--------------------------");
    }
}

// --- 公共 API 实现 ---

void test_registry_init() {
    registered_command_count = 0;
    // 自动注册内置的 'help' 命令
    CommandRegistryEntry help_command = {"help", handle_help, "Displays help information. Usage: help [command]"};
    test_registry_register_commands(&help_command, 1);
}

bool test_registry_register_commands(const CommandRegistryEntry commands[], size_t count) {
    if (registered_command_count + count > MAX_COMMANDS) {
        return false; // 空间不足
    }
    for (size_t i = 0; i < count; ++i) {
        command_registry[registered_command_count++] = commands[i];
    }
    return true;
}

void test_registry_handle_command(const String& commandLine) {
    // 解析命令和参数
    String command_name;
    String command_args;

    int first_space = commandLine.indexOf(' ');
    if (first_space == -1) {
        command_name = commandLine;
        command_args = "";
    } else {
        command_name = commandLine.substring(0, first_space);
        command_args = commandLine.substring(first_space + 1);
        command_args.trim();
    }

    // 查找并执行命令
    const CommandRegistryEntry* entry = find_command_entry(command_name.c_str());

    if (entry) {
        entry->handler(command_args.c_str());
    } else {
        Serial.println("Error: Unknown command");
    }

    // 任何命令处理后都必须发送信标
    send_eot();
}

#endif // TEST_MODE