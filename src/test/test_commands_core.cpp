/**
 * @file test_commands_core.cpp
 * @brief 核心测试命令的实现
 */

#include "test_commands_core.h"
#include "test_command_registry.h"
#include <Arduino.h>

#ifdef TEST_MODE

// --- 命令处理函数 ---

/**
 * @brief 处理 "echo" 命令
 * @param args 要回显的字符串
 */
void handle_echo(const char* args) {
    Serial.println(args);
}

// --- 命令定义 ---

// 定义此模块提供的所有命令
static const CommandRegistryEntry core_commands[] = {
    {"echo", handle_echo, "Echoes the provided string back. Usage: echo <text>"}
};

// --- 公共 API ---

void test_commands_core_init() {
    // 向注册器注册本模块的命令
    test_registry_register_commands(core_commands, sizeof(core_commands) / sizeof(core_commands));
}

#endif // TEST_MODE