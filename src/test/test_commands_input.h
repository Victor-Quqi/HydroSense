/**
 * @file test_commands_input.h
 * @brief Input Manager测试命令接口
 */

#ifndef TEST_COMMANDS_INPUT_H
#define TEST_COMMANDS_INPUT_H

#ifdef TEST_MODE

/**
 * @brief 初始化Input Manager测试命令
 * @details 向命令注册器注册所有Input Manager相关的测试命令
 */
void test_commands_input_init();

#endif // TEST_MODE

#endif // TEST_COMMANDS_INPUT_H
