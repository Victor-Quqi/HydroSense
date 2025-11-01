/**
 * @file test_cli.h
 * @brief 测试命令行接口
 * @details 提供串口CLI命令解析和处理功能，支持硬件在环测试
 */

#ifndef TEST_CLI_H
#define TEST_CLI_H

#ifdef TEST_MODE

#include <Arduino.h>

// 定义HIL测试的结束信标 (End of Transmission)
// 定义HIL测试的结束信标 (End of Transmission)
constexpr const char* EOT_BEACON = "<<EOT>>";

/**
 * @brief 初始化测试命令行接口
 */
void test_cli_init();

/**
 * @brief 测试命令行接口的主循环函数
 * @details 持续检查来自串口的输入并将其转发给命令注册器
 */
void test_cli_loop();

/**
 * @brief 发送结束信标
 * @details 这是一个辅助函数，供命令处理函数在完成时调用
 */
void send_eot();

#endif // TEST_MODE

#endif // TEST_CLI_H