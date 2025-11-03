/**
 * @file main.cpp
 * @brief HydroSense主程序
 * @details 系统主入口文件
 */

#include <Arduino.h>

// 测试模式编译宏 - 通过PlatformIO构建配置控制
#ifdef TEST_MODE
  #include "test/test_mode.h"
  #include "test/test_cli.h"
  #include "test/test_command_registry.h"
  #include "test/test_commands_core.h"
  #include "test/test_commands_hal.h"
#endif

#include "managers/power_manager.h"
#include "managers/sensor_manager.h"

void setup() {
  // 系统初始化
  power_result_t power_init_result = power_manager_init();
  sensor_manager_init();

#ifdef TEST_MODE
  if (power_init_result != POWER_OK) {
    Serial.printf("[ERROR] Power manager initialization failed: %d\r\n",
                  power_init_result);
  }
#endif

  #ifdef TEST_MODE
    // --- 测试模式初始化 ---
    // 1. 初始化物理接口
    Serial.begin(115200);
    
    // 2. 初始化核心测试模块
    test_mode_init();
    test_cli_init();
    test_registry_init();

    // 3. 注册所有测试命令
    test_commands_core_init();
    test_commands_hal_init();

  #endif
}

void loop() {
  // 主循环

  #ifdef TEST_MODE
    // 测试模式处理 - 禁用深度睡眠，持续响应CLI
    test_cli_loop();
  #else
    // 正常运行模式 - 包含深度睡眠逻辑
  #endif
}