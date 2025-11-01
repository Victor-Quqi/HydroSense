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
constexpr const char* EOT_BEACON = "<<EOT>>";

/**
 * @brief 初始化测试命令行接口
 * @details 设置串口通信速率
 */
void test_cli_init();

/**
 * @brief 测试命令行接口的主循环函数
 * @details 持续检查并处理来自串口的命令
 */
void test_cli_loop();

#endif // TEST_MODE

#endif // TEST_CLI_H