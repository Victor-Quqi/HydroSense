/**
 * @file test_commands_run.cpp
 * @brief RUN mode test commands implementation
 */

#include "test_commands_run.h"
#include "test_command_registry.h"
#include "../managers/run_mode_manager.h"
#include <Arduino.h>

#ifdef TEST_MODE

// --- Command Handler Functions ---

/**
 * @brief Handle "run" command
 *
 * @param args Expected format: "force_water"
 */
void handle_run(const char* args) {
    char action[20];
    int items = sscanf(args, "%19s", action);

    if (items != 1 || strcmp(action, "force_water") != 0) {
        Serial.println("Error: Invalid arguments. Usage: run force_water");
        return;
    }

    // Execute forced watering cycle
    run_mode_result_t result = run_mode_manager_force_water();

    // Return JSON response
    Serial.print("{");
    Serial.print("\"command\":\"run force_water\",");
    Serial.print("\"status\":");
    if (result == RUN_MODE_OK) {
        Serial.print("\"success\",");
        Serial.print("\"message\":\"Watering sequence completed successfully\"");
    } else {
        Serial.print("\"error\",");
        Serial.print("\"error_code\":");
        Serial.print(result);
        Serial.print(",");
        Serial.print("\"message\":\"");
        switch (result) {
            case RUN_MODE_ERR_NOT_INITIALIZED:
                Serial.print("Run mode manager not initialized");
                break;
            case RUN_MODE_ERR_SENSOR_READ_FAILED:
                Serial.print("Failed to read humidity sensor");
                break;
            case RUN_MODE_ERR_ACTUATOR_FAILED:
                Serial.print("Failed to start pump");
                break;
            default:
                Serial.print("Unknown error");
                break;
        }
        Serial.print("\"");
    }
    Serial.println("}");
}

// --- Command Definitions ---

// Define all commands provided by this module
static const CommandRegistryEntry run_commands[] = {
    {"run", handle_run, "RUN mode commands. Usage: run <action>\r\n"
                       "  - action: force_water (triggers a full watering cycle)"}
};

// --- Public API ---

void test_commands_run_init() {
    // Register this module's commands with the registry
    test_registry_register_commands(run_commands, sizeof(run_commands) / sizeof(run_commands[0]));
}

#endif // TEST_MODE
