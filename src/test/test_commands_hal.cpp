/**
 * @file test_commands_hal.cpp
 * @brief HAL相关测试命令的实现
 */

#include "test_commands_hal.h"
#include "test_command_registry.h"
#include "managers/power_manager.h"
#include "managers/sensor_manager.h"
#include "managers/actuator_manager.h"
#include "ui/display_manager.h"
#include "ui/ui_manager.h"
#include "data/data_models.h"
#include "managers/input_manager.h"
#include <Arduino.h>
#include <string.h>

#ifdef TEST_MODE

/* ========== 常量定义 ========== */
#define MAX_MODULE_NAME_LEN 10
#define MAX_STATE_NAME_LEN  4
#define MAX_ACTION_NAME_LEN 10

/* ========== 私有辅助函数 ========== */

// 辅助函数，用于处理单个电源模块的开关逻辑
static void process_power_command(const char* module_name, bool enable, 
                                  bool (*is_enabled_func)(), 
                                  power_result_t (*set_enable_func)(bool),
                                  const char* display_name) {
    if (is_enabled_func() == enable) {
        Serial.printf("%s power is already %s.\r\n", display_name, enable ? "on" : "off");
    } else {
        power_result_t result = set_enable_func(enable);
        if (result == POWER_OK) {
            Serial.printf("%s power set to %s.\r\n", display_name, enable ? "on" : "off");
        } else {
            Serial.printf("Error: Failed to set %s power. Result: %d\r\n",
                          display_name, result);
        }
    }
}


// --- 命令处理函数 ---

/**
 * @brief 处理 "power" 命令
 * @param args 格式: "set <module> <on|off>"
 *             module: sensor, boost12v, screen
 */
void handle_power(const char* args) {
    char action[MAX_ACTION_NAME_LEN];
    char module[MAX_MODULE_NAME_LEN];
    char state[MAX_STATE_NAME_LEN];
    int items = sscanf(args, "%9s %9s %3s", action, module, state);

    if (items != 3 || strcmp(action, "set") != 0) {
        Serial.println("Error: Invalid arguments. Usage: power set <module> <on|off>");
        return;
    }

    if (strcmp(state, "on") != 0 && strcmp(state, "off") != 0) {
        Serial.println("Error: Invalid state. Use 'on' or 'off'.");
        return;
    }

    bool enable = (strcmp(state, "on") == 0);

    if (strcmp(module, "sensor") == 0) {
        process_power_command(module, enable, power_sensor_is_enabled, power_sensor_enable, "Sensor");
    } else if (strcmp(module, "boost12v") == 0) {
        process_power_command(module, enable, power_pump_module_is_enabled, power_pump_module_enable, "12V Boost Module");
    } else if (strcmp(module, "screen") == 0) {
        process_power_command(module, enable, power_screen_is_enabled, power_screen_enable, "Screen");
    } else {
        Serial.println("Error: Unknown module. Available: sensor, boost12v, screen");
    }
}

/**
 * @brief 处理 "sensor" 命令
 * @param args 格式: "read <all|humidity|battery>"
 */
void handle_sensor(const char* args) {
    char action[MAX_ACTION_NAME_LEN];
    char source[MAX_MODULE_NAME_LEN]; // Reuse for source
    int items = sscanf(args, "%9s %9s", action, source);

    if (items < 1 || strcmp(action, "read") != 0) {
        Serial.println("Error: Invalid action. Usage: sensor read <source>");
        return;
    }

    // If only "sensor read" is provided, default to "all"
    if (items == 1) {
        strcpy(source, "all");
    }

    if (strcmp(source, "all") == 0) {
        Serial.println("Reading all sensors...");
        sensor_data_t data;
        sensor_result_t result = sensor_manager_read_all(&data);

        if (result == SENSOR_OK) {
            Serial.printf("  - Soil Moisture (ADC): %d\r\n", data.soil_moisture);
            Serial.printf("  - Battery Voltage:     %.2f V\r\n", data.battery_voltage);
        } else {
            Serial.printf("Error: Failed to read sensors. Result: %d\r\n", result);
        }
    } else if (strcmp(source, "humidity") == 0) {
        Serial.println("Reading humidity sensor...");
        float humidity;
        sensor_result_t result = sensor_manager_get_humidity(&humidity);

        if (result == SENSOR_OK) {
            Serial.printf("  - Soil Moisture (ADC): %.0f\r\n", humidity);
        } else {
            Serial.printf("Error: Failed to read humidity. Result: %d\r\n", result);
        }
    } else if (strcmp(source, "battery") == 0) {
        Serial.println("Reading battery voltage...");
        float voltage;
        sensor_result_t result = sensor_manager_get_battery_voltage(&voltage);

        if (result == SENSOR_OK) {
            Serial.printf("  - Battery Voltage: %.2f V\r\n", voltage);
        } else {
            Serial.printf("Error: Failed to read battery voltage. Result: %d\r\n", result);
        }
    } else {
        Serial.println("Error: Invalid source. Usage: sensor read <all|humidity|battery>");
        return;
    }
}

/**
 * @brief 处理 "pump" 命令
 * @param args 格式: "run <duty_cycle> <duration_ms>"
 */
void handle_pump(const char* args) {
    char action[MAX_ACTION_NAME_LEN];
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
    actuator_manager_run_pump_for((uint8_t)duty_cycle, duration);
    
    // 在测试命令中，我们可以阻塞地等待，以观察完整过程
    uint32_t start = millis();
    while (millis() - start < duration) {
        actuator_manager_loop(); // 持续驱动管理器
        delay(1);
    }
    // 确保最后一次状态更新被执行
    actuator_manager_loop();

    Serial.println("Pump command finished.");
}


// 显示相关命令处理
void handle_display(const char* args) {
    char action[MAX_ACTION_NAME_LEN] = {0};

    if (args == nullptr) args = "";
    while (*args == ' ') ++args; // 跳过前导空格

    int items = sscanf(args, "%9s", action);
    if (items != 1) {
        Serial.println("Error: Invalid arguments. Usage: display <init|clear|text <message>|refresh|sleep>");
        return;
    }

    if (strcmp(action, "init") == 0) {
        display_result_t r = display_manager_init();
        Serial.printf("Display init: %s (%d)\r\n", r == DISPLAY_OK ? "OK" : "FAIL", r);
        return;
    }

    // 其他操作前确保已初始化
    if (!display_manager_is_initialized()) {
        display_result_t r = display_manager_init();
        if (r != DISPLAY_OK) {
            Serial.printf("Error: display init failed (%d)\r\n", r);
            return;
        }
    }

    if (strcmp(action, "clear") == 0) {
        (void)display_manager_clear();
        Serial.println("Display cleared (buffer only).");
        return;
    } else if (strcmp(action, "text") == 0) {
        // 提取 text 后的剩余部分作为消息
        const char* content = strchr(args, ' ');
        if (!content) {
            Serial.println("Error: Missing text. Usage: display text <message>");
            return;
        }
        content++; // 跳过空格

        String msg = String(content);
        msg.trim();
        // 去掉首尾的单引号或双引号（如果存在）
        if (msg.length() >= 2) {
            if ((msg.startsWith("\"") && msg.endsWith("\"")) || (msg.startsWith("'") && msg.endsWith("'"))) {
                msg = msg.substring(1, msg.length() - 1);
            }
        }

        (void)display_manager_clear();
        (void)display_manager_draw_text(msg.c_str(), 10, 40);
        (void)display_manager_refresh(true); // 全局刷新，确保可见
        Serial.println("Display text updated.");
        return;
    } else if (strcmp(action, "refresh") == 0) {
        (void)display_manager_refresh(true);
        Serial.println("Display refreshed (full).");
        return;
    } else if (strcmp(action, "sleep") == 0) {
        (void)display_manager_sleep();
        Serial.println("Display hibernated.");
        return;
    } else if (strcmp(action, "lvgl_test") == 0) {
        ui_result_t r = ui_manager_init();
        if (r != UI_OK) {
            Serial.printf("Error: ui_manager_init failed (%d)\r\n", r);
            return;
        }
        ui_manager_show_test_screen();
        Serial.println("LVGL test screen displayed.");
        return;
    } else {
        Serial.println("Error: Unknown action. Use: init|clear|text|refresh|sleep|lvgl_test");
        return;
    }
}

/**
 * @brief 处理 "system" 命令
 * @param args 格式: "get mode"
 */
void handle_system(const char* args) {
    char action[MAX_ACTION_NAME_LEN];
    char target[MAX_MODULE_NAME_LEN];
    int items = sscanf(args, "%9s %9s", action, target);

    if (items != 2 || strcmp(action, "get") != 0 || strcmp(target, "mode") != 0) {
        Serial.println("Error: Invalid arguments. Usage: system get mode");
        return;
    }

    system_mode_t mode = input_manager_get_mode();
    const char* mode_str;
    switch (mode) {
        case SYSTEM_MODE_OFF:
            mode_str = "OFF";
            break;
        case SYSTEM_MODE_RUN:
            mode_str = "RUN";
            break;
        case SYSTEM_MODE_INTERACTIVE:
            mode_str = "INTERACTIVE";
            break;
        default:
            mode_str = "UNKNOWN";
            break;
    }
    Serial.printf("Current system mode: %s\r\n", mode_str);
}

// --- 命令定义 ---

static const CommandRegistryEntry hal_commands[] = {
    {"power", handle_power, "Controls power gates. Usage: power set <module> <on|off>\r\n"
                           "  - module: sensor, boost12v, screen"},
    {"sensor", handle_sensor, "Reads sensor data. Usage: sensor read <source>\r\n"
                            "  - source: all, humidity, battery"},
    {"pump", handle_pump, "Runs the water pump. Usage: pump run <duty> <ms>\r\n"
                         "  - duty: 0-255 (PWM duty cycle)\r\n"
                         "  - ms: 1-30000 (duration in milliseconds)"},
    {"display", handle_display, "Controls the display. Usage: display <action> [params]\r\n"
                               "  - actions: init, text \"msg\", sleep, lvgl_test"},
    {"system", handle_system, "Gets system status. Usage: system get mode"}
};

// --- 公共 API ---

void test_commands_hal_init() {
    test_registry_register_commands(hal_commands, sizeof(hal_commands) / sizeof(hal_commands[0]));
}

#endif // TEST_MODE