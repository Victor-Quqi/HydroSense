/**
 * @file test_commands_core.h
 * @brief 提供核心的、通用的测试命令
 */

#ifndef TEST_COMMANDS_CORE_H
#define TEST_COMMANDS_CORE_H

#ifdef TEST_MODE

/**
 * @brief 初始化核心命令模块并注册其命令
 */
void test_commands_core_init();

#endif // TEST_MODE

#endif // TEST_COMMANDS_CORE_H