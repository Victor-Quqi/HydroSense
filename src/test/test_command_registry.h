/**
 * @file test_command_registry.h
 * @brief 测试命令注册器和调度器
 * @details
 *   提供一个中心化的机制来注册和处理CLI命令。
 *   使用函数指针表将命令字符串映射到具体的处理函数，以实现解耦。
 *
 * ## 命令风格规范
 *
 * ### 1. 命令命名风格
 * - **使用单个主命令 + 子命令的模式**
 * - 主命令：功能模块名（如 `wifi`, `config`, `input`）
 * - 子命令：具体操作（如 `status`, `connect`, `poll`）
 *
 * ### 2. 命令注册风格
 * ```c
 * // 单个命令入口，内部分发子命令
 * static const CommandRegistryEntry xxx_commands[] = {
 *     {"wifi", handle_wifi, "Manages WiFi. Usage: wifi <status|connect|disconnect>"}
 * };
 * ```
 *
 * ### 3. 命令处理函数风格
 * ```c
 * void handle_wifi(const char* args) {
 *     char action[16];
 *     sscanf(args, "%15s", action);
 *
 *     if (strcmp(action, "status") == 0) {
 *         handle_wifi_status();  // 内部子函数处理具体逻辑
 *     } else if (strcmp(action, "connect") == 0) {
 *         handle_wifi_connect();
 *     }
 *     // ...
 * }
 * ```
 */

#ifndef TEST_COMMAND_REGISTRY_H
#define TEST_COMMAND_REGISTRY_H

#ifdef TEST_MODE

#include <Arduino.h>

// 定义命令处理函数的原型
// 参数: 命令行中跟在命令本身后面的部分 (可能为空字符串)
typedef void (*CommandHandler)(const char* args);

// 定义命令注册表中的条目结构
typedef struct {
    const char* name;    // 命令名称 (例如 "ping", "echo")
    CommandHandler handler; // 指向处理该命令的函数指针
    const char* help;    // 命令的简短帮助说明
} CommandRegistryEntry;

/**
 * @brief 初始化命令注册系统
 */
void test_registry_init();

/**
 * @brief 注册一个命令数组
 * @param commands 要注册的命令数组
 * @param count 数组中的命令数量
 * @return bool true 如果注册成功, false 如果注册表已满
 */
bool test_registry_register_commands(const CommandRegistryEntry commands[], size_t count);

/**
 * @brief 解析并执行一个完整的命令行
 * @param commandLine 从CLI接收到的完整命令行字符串
 */
void test_registry_handle_command(const String& commandLine);

#endif // TEST_MODE

#endif // TEST_COMMAND_REGISTRY_H