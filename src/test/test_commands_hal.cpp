/**
 * @file test_commands_hal.cpp
 * @brief HAL相关测试命令的实现
 */

#include "test_commands_hal.h"
#include "test_command_registry.h"
#include "managers/power_manager.h"
#include "managers/sensor_manager.h"
#include "managers/actuator_manager.h"
#include "data/data_models.h"
#include <Arduino.h>
#include <string.h>

#ifdef TEST_MODE

/* ========== 常量定义 ========== */
#define MAX_MODULE_NAME_LEN 10
#define MAX_STATE_NAME_LEN  4
#define MODULE_COUNT        3

/* ========== 私有辅助函数 ========== */
static bool is_valid_module(const char* module) {
    const char* valid_modules[MODULE_COUNT] = {"sensor", "boost12v", "screen"};
    for (int i = 0; i < MODULE_COUNT; i++) {
        if (strcmp(module, valid_modules[i]) == 0) {
            return true;
        }
    }
    return false;
}

static const char* get_module_display_name(const char* module) {
    if (strcmp(module, "sensor") == 0) return "Sensor";
    if (strcmp(module, "boost12v") == 0) return "12V Boost Module";
    if (strcmp(module, "screen") == 0) return "Screen";
    return "Unknown";
}

// --- 命令处理函数 ---

/**
 * @brief 处理 "power" 命令
 * @param args 格式: "<module> <on|off>"
 *             module: sensor, pump, screen
 */
void handle_power(const char* args) {
    char module[MAX_MODULE_NAME_LEN];   // Buffer for module name
    char state[MAX_STATE_NAME_LEN];     // Buffer for state ("on" or "off")
    int items = sscanf(args, "%9s %3s", module, state);

    if (items != 2) {
        Serial.println("Error: Invalid arguments. Usage: power <module> <on|off>");
        return;
    }

    if (strcmp(state, "on") != 0 && strcmp(state, "off") != 0) {
        Serial.println("Error: Invalid state. Use 'on' or 'off'.");
        return;
    }

    if (!is_valid_module(module)) {
        Serial.println("Error: Unknown module. Available: sensor, boost12v, screen");
        return;
    }

    bool enable = (strcmp(state, "on") == 0);
    const char* display_name = get_module_display_name(module);
    power_result_t result;

    // 使用新的错误处理API
    if (strcmp(module, "sensor") == 0) {
        if (power_sensor_is_enabled() == enable) {
            Serial.printf("%s power is already %s.\r\n", display_name, state);
        } else {
            result = power_sensor_enable(enable);
            if (result == POWER_OK) {
                Serial.printf("%s power set to %s.\r\n", display_name, state);
            } else {
                Serial.printf("Error: Failed to set %s power. Result: %d\r\n",
                            display_name, result);
            }
        }
    } else if (strcmp(module, "boost12v") == 0) {
        if (power_pump_module_is_enabled() == enable) {
            Serial.printf("%s power is already %s.\r\n", display_name, state);
        } else {
            result = power_pump_module_enable(enable);
            if (result == POWER_OK) {
                Serial.printf("%s power set to %s.\r\n", display_name, state);
            } else {
                Serial.printf("Error: Failed to set %s power. Result: %d\r\n",
                            display_name, result);
            }
        }
    } else if (strcmp(module, "screen") == 0) {
        if (power_screen_is_enabled() == enable) {
            Serial.printf("%s power is already %s.\r\n", display_name, state);
        } else {
            result = power_screen_enable(enable);
            if (result == POWER_OK) {
                Serial.printf("%s power set to %s.\r\n", display_name, state);
            } else {
                Serial.printf("Error: Failed to set %s power. Result: %d\r\n",
                            display_name, result);
            }
        }
    }
}

/**
 * @brief 处理 "read" 命令
 * @param args 格式: "<all|humidity|battery>"
 */
void handle_read(const char* args) {
    if (strcmp(args, "all") == 0) {
        // 读取所有传感器
        Serial.println("Reading all sensors...");
        sensor_data_t data;
        sensor_result_t result = sensor_manager_read_all(&data);

        if (result == SENSOR_OK) {
            Serial.printf("  - Soil Moisture (ADC): %d\r\n", data.soil_moisture);
            Serial.printf("  - Battery Voltage:     %.2f V\r\n", data.battery_voltage);
        } else {
            Serial.printf("Error: Failed to read sensors. Result: %d\r\n", result);
        }
    } else if (strcmp(args, "humidity") == 0) {
        // 读取湿度
        Serial.println("Reading humidity sensor...");
        float humidity;
        sensor_result_t result = sensor_manager_get_humidity(&humidity);

        if (result == SENSOR_OK) {
            Serial.printf("  - Soil Moisture (ADC): %.0f\r\n", humidity);
        } else {
            Serial.printf("Error: Failed to read humidity. Result: %d\r\n", result);
        }
    } else if (strcmp(args, "battery") == 0) {
        // 读取电池电压
        Serial.println("Reading battery voltage...");
        float voltage;
        sensor_result_t result = sensor_manager_get_battery_voltage(&voltage);

        if (result == SENSOR_OK) {
            Serial.printf("  - Battery Voltage: %.2f V\r\n", voltage);
        } else {
            Serial.printf("Error: Failed to read battery voltage. Result: %d\r\n", result);
        }
    } else {
        Serial.println("Error: Invalid arguments. Usage: read <all|humidity|battery>");
        return;
    }
}

/**
 * @brief 处理 "pump" 命令
 * @param args 格式: "run <duty_cycle> <duration_ms>"
 */
void handle_pump(const char* args) {
    char action[10];
    int duty_cycle;
    uint32_t duration;
    int items = sscanf(args, "%9s %d %lu", action, &duty_cycle, &duration);

    if (items != 3 || strcmp(action, "run") != 0) {
        Serial.println("Error: Invalid arguments. Usage: pump run <duty_cycle> <duration_ms>");
        return;
    }

    if (duty_cycle < 0 || duty_cycle > 255) {
        Serial.println("Error: Duty cycle must be between 0 and 255.");
        return;
    }

    if (duration == 0 || duration > 30000) {
        Serial.println("Error: Duration must be between 1 and 30000 ms.");
        return;
    }

    Serial.printf("Running pump with duty cycle %d for %lu ms...\r\n", duty_cycle, duration);
    actuator_manager_run_pump((uint8_t)duty_cycle, duration);
    Serial.println("Pump command finished.");
}

/**
 * @brief 处理 "valve" 命令
 * @param args 格式: "<on|off>"
 */
void handle_valve(const char* args) {
    if (strcmp(args, "on") == 0) {
        actuator_manager_set_valve(true);
        Serial.println("Valve opened.");
    } else if (strcmp(args, "off") == 0) {
        actuator_manager_set_valve(false);
        Serial.println("Valve closed.");
    } else {
        Serial.println("Error: Invalid arguments. Usage: valve <on|off>");
    }
}


// --- 命令定义 ---

static const CommandRegistryEntry hal_commands[] = {
    {"power", handle_power, "Control power gates. Usage: power <sensor|boost12v|screen> <on|off>"},
    {"read", handle_read, "Read sensor data. Usage: read <all|humidity|battery>"},
    {"pump", handle_pump, "Run pump. Usage: pump run <duty_cycle> <duration_ms>"},
    {"valve", handle_valve, "Control solenoid valve. Usage: valve <on|off>"}
};

// --- 公共 API ---

void test_commands_hal_init() {
    test_registry_register_commands(hal_commands,
                                   sizeof(hal_commands) / sizeof(hal_commands[0]));
}

#endif // TEST_MODE