/**
 * @file test_commands_input.cpp
 * @brief Input Manager测试命令实现
 * @details
 *   本文件展示了标准的测试命令实现风格：
 *   - 单个主命令 "input" + 多个子命令（poll, status, clear）
 *   - 主处理函数 handle_input() 负责分发到具体的子处理函数
 *   - 遵循 test_command_registry.h 中的命令风格规范
 */

#include "test_commands_input.h"
#include "test_command_registry.h"
#include "managers/input_manager.h"
#include <Arduino.h>

#ifdef TEST_MODE

// --- 命令处理函数 ---

/**
 * @brief 处理 "input" 命令 - poll子命令
 * @details 持续轮询5秒，显示所有编码器和按键事件
 */
void handle_input_poll() {
    Serial.println("{");
    Serial.println("  \"command\": \"input_poll\",");
    Serial.println("  \"status\": \"polling\",");
    Serial.println("  \"duration_ms\": 5000,");
    Serial.println("  \"message\": \"Polling for 5 seconds. Rotate encoder or press button...\"");
    Serial.println("}");

    // 清除开始前累积的事件，避免莫名其妙的旧事件
    input_manager_clear_events();

    unsigned long start_time = millis();
    unsigned long poll_duration = 5000; // 5秒
    int event_count = 0;

    while (millis() - start_time < poll_duration) {
        // 关键：必须先调用loop更新硬件状态
        input_manager_loop();

        // 读取编码器增量
        int8_t delta = input_manager_get_encoder_delta();
        if (delta != 0) {
            Serial.print("{\"event\": \"encoder\", \"delta\": ");
            Serial.print(delta);
            Serial.print(", \"direction\": \"");
            Serial.print(delta > 0 ? "CW" : "CCW");
            Serial.println("\"}");
            event_count++;
        }

        // 检测单击
        if (input_manager_get_button_clicked()) {
            Serial.println("{\"event\": \"button\", \"type\": \"single_click\"}");
            event_count++;
        }

        // 检测双击
        if (input_manager_get_button_double_clicked()) {
            Serial.println("{\"event\": \"button\", \"type\": \"double_click\"}");
            event_count++;
        }

        // 缩短延迟，提高响应速度（减少漏检）
        delay(1); // 1ms延迟，避免CPU占用过高
    }

    Serial.println("{");
    Serial.println("  \"command\": \"input_poll\",");
    Serial.println("  \"status\": \"completed\",");
    Serial.print("  \"events_detected\": ");
    Serial.println(event_count);
    Serial.println("}");
}

/**
 * @brief 处理 "input" 命令 - status子命令
 * @details 显示当前输入状态（系统模式和待处理事件）
 */
void handle_input_status() {
    // 获取系统模式
    system_mode_t mode = input_manager_get_mode();
    const char* mode_str = "UNKNOWN";
    switch (mode) {
        case SYSTEM_MODE_OFF: mode_str = "OFF"; break;
        case SYSTEM_MODE_RUN: mode_str = "RUN"; break;
        case SYSTEM_MODE_INTERACTIVE: mode_str = "INTERACTIVE"; break;
        default: mode_str = "UNKNOWN"; break;
    }

    // 检查待处理事件（非消费型读取）
    int8_t delta = input_manager_get_encoder_delta();
    bool clicked = input_manager_get_button_clicked();
    bool double_clicked = input_manager_get_button_double_clicked();

    Serial.println("{");
    Serial.println("  \"command\": \"input_status\",");
    Serial.print("  \"system_mode\": \"");
    Serial.print(mode_str);
    Serial.println("\",");
    Serial.print("  \"encoder_delta\": ");
    Serial.print(delta);
    Serial.println(",");
    Serial.print("  \"button_clicked\": ");
    Serial.print(clicked ? "true" : "false");
    Serial.println(",");
    Serial.print("  \"button_double_clicked\": ");
    Serial.print(double_clicked ? "true" : "false");
    Serial.println();
    Serial.println("}");
}

/**
 * @brief 处理 "input" 命令 - clear子命令
 * @details 清除所有累积的输入事件
 */
void handle_input_clear() {
    input_manager_clear_events();
    Serial.println("{");
    Serial.println("  \"command\": \"input_clear\",");
    Serial.println("  \"status\": \"ok\",");
    Serial.println("  \"message\": \"All input events cleared\"");
    Serial.println("}");
}

/**
 * @brief 处理 "input" 命令
 * @param args 格式: "<poll|status|clear>"
 */
void handle_input(const char* args) {
    char action[16];
    int items = sscanf(args, "%15s", action);

    if (items < 1) {
        Serial.println("{");
        Serial.println("  \"command\": \"input\",");
        Serial.println("  \"status\": \"error\",");
        Serial.println("  \"message\": \"Missing action. Usage: input <poll|status|clear>\"");
        Serial.println("}");
        return;
    }

    if (strcmp(action, "poll") == 0) {
        handle_input_poll();
    } else if (strcmp(action, "status") == 0) {
        handle_input_status();
    } else if (strcmp(action, "clear") == 0) {
        handle_input_clear();
    } else {
        Serial.println("{");
        Serial.println("  \"command\": \"input\",");
        Serial.println("  \"status\": \"error\",");
        Serial.print("  \"message\": \"Unknown action: ");
        Serial.print(action);
        Serial.println(". Usage: input <poll|status|clear>\"");
        Serial.println("}");
    }
}

// --- 命令定义 ---

static const CommandRegistryEntry input_commands[] = {
    {"input", handle_input, "Manages input devices. Usage: input <poll|status|clear>"}
};

// --- 公共 API ---

void test_commands_input_init() {
    test_registry_register_commands(input_commands, sizeof(input_commands) / sizeof(input_commands[0]));
}

#endif // TEST_MODE
