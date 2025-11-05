/**
 * @file test_commands_log.h
 * @brief Log-related test commands
 */

#ifndef TEST_COMMANDS_LOG_H
#define TEST_COMMANDS_LOG_H

#include "hal/hal_config.h"

#ifdef TEST_MODE

/**
 * @brief Initializes and registers log-related test commands.
 */
void test_commands_log_init();

#endif // TEST_MODE

#endif // TEST_COMMANDS_LOG_H