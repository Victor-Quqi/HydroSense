/**
 * @file test_commands_log.cpp
 * @brief Log-related test commands implementation
 */

#include "test_commands_log.h"
#include "test_command_registry.h"
#include "managers/log_manager.h"
#include <Arduino.h>
#include <string.h>

#ifdef TEST_MODE

// --- Command Handler ---

/**
 * @brief Handles the "log" command
 * @param args Format: "<level> <module> <message...>"
 *             level: debug, info, warn, error
 */
void handle_log(const char* args) {
    char level[10];
    char module[20];
    char message[100];

    int items = sscanf(args, "%9s %19s %99[^\n]", level, module, message);

    if (items < 3) {
        Serial.println("Error: Invalid arguments. Usage: log <level> <module> <message...>");
        return;
    }

    if (strcmp(level, "debug") == 0) {
        LOG_DEBUG(module, "%s", message);
    } else if (strcmp(level, "info") == 0) {
        LOG_INFO(module, "%s", message);
    } else if (strcmp(level, "warn") == 0) {
        LOG_WARN(module, "%s", message);
    } else if (strcmp(level, "error") == 0) {
        LOG_ERROR(module, "%s", message);
    } else {
        Serial.println("Error: Invalid log level. Use 'debug', 'info', 'warn', or 'error'.");
        return;
    }
    Serial.printf("Log message sent.\r\n");
}

// --- Command Definition ---

static const CommandRegistryEntry log_commands[] = {
    {"log", handle_log, "Generate a log message. Usage: log <level> <module> <message...>"}
};

// --- Public API ---

void test_commands_log_init() {
    test_registry_register_commands(log_commands, sizeof(log_commands) / sizeof(log_commands));
}

#endif // TEST_MODE