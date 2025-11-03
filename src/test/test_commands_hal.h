/**
 * @file test_commands_hal.h
 * @brief 提供硬件抽象层 (HAL) 相关的测试命令
 */

#ifndef TEST_COMMANDS_HAL_H
#define TEST_COMMANDS_HAL_H

#ifdef TEST_MODE

/**
 * @brief 初始化HAL命令模块并注册其命令
 */
void test_commands_hal_init();

#endif // TEST_MODE

#endif // TEST_COMMANDS_HAL_H