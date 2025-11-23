/**
 * @file test_commands_interactive.cpp
 * @brief Interactive Manager测试命令实现
 * @details
 *   本文件提供测试命令用于验证Interactive Manager的状态机逻辑
 */

#include "test_commands_interactive.h"
#include "test_command_registry.h"
#include "managers/interactive_mode_manager.h"
#include "managers/input_manager.h"
#include "data/timing_constants.h"
#include <Arduino.h>

#ifdef TEST_MODE

// --- 命令处理函数 ---

/**
 * @brief 处理 "interactive" 命令 - poll子命令
 * @details 模拟Interactive模式运行，测试状态机逻辑
 */
void handle_interactive_poll() {
    Serial.println("{");
    Serial.println("  \"command\": \"interactive_poll\",");
    Serial.println("  \"status\": \"starting\",");
    Serial.println("  \"message\": \"Starting interactive mode test...\"");
    Serial.println("}");

    // 初始化并进入interactive mode
    interactive_mode_manager_init();
    interactive_mode_manager_enter();

    Serial.println("{");
    Serial.println("  \"status\": \"running\",");
    Serial.println("  \"message\": \"Rotate to navigate, click to enter, double-click to return. Double-click in MAIN MENU to exit.\"");
    Serial.println("}");

    // 无限循环，直到在主菜单双击退出
    while (!interactive_mode_manager_should_exit()) {
        // 更新输入状态
        input_manager_loop();

        // 运行interactive manager的状态机
        interactive_mode_manager_loop();

        delay(TEST_LOOP_DELAY_MS); // 平衡CPU占用和响应速度
    }

    // 退出interactive mode
    interactive_mode_manager_exit();

    Serial.println("{");
    Serial.println("  \"command\": \"interactive_poll\",");
    Serial.println("  \"status\": \"completed\",");
    Serial.println("  \"message\": \"Interactive mode test completed (exited by double-click in main menu)\"");
    Serial.println("}");
}

/**
 * @brief 处理 "interactive" 命令
 * @param args 格式: "<poll>"
 */
void handle_interactive(const char* args) {
    char action[16];
    int items = sscanf(args, "%15s", action);

    if (items < 1) {
        Serial.println("{");
        Serial.println("  \"command\": \"interactive\",");
        Serial.println("  \"status\": \"error\",");
        Serial.println("  \"message\": \"Missing action. Usage: interactive <poll>\"");
        Serial.println("}");
        return;
    }

    if (strcmp(action, "poll") == 0) {
        handle_interactive_poll();
    } else {
        Serial.println("{");
        Serial.println("  \"command\": \"interactive\",");
        Serial.println("  \"status\": \"error\",");
        Serial.print("  \"message\": \"Unknown action: ");
        Serial.print(action);
        Serial.println(". Usage: interactive <poll>\"");
        Serial.println("}");
    }
}

// --- 命令定义 ---

static const CommandRegistryEntry interactive_commands[] = {
    {"interactive", handle_interactive, "Tests interactive mode. Usage: interactive <poll>"}
};

// --- 公共 API ---

void test_commands_interactive_init() {
    test_registry_register_commands(interactive_commands, sizeof(interactive_commands) / sizeof(interactive_commands[0]));
}

#endif // TEST_MODE
