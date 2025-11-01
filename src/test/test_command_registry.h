/**
 * @file test_command_registry.h
 * @brief 测试命令注册器和调度器
 * @details
 *   提供一个中心化的机制来注册和处理CLI命令。
 *   使用函数指针表将命令字符串映射到具体的处理函数，以实现解耦。
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