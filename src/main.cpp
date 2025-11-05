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
  #include "test/test_commands_log.h"
#endif

#include "managers/power_manager.h"
#include "managers/sensor_manager.h"
#include "managers/log_manager.h"
#include "managers/actuator_manager.h"

void setup() {
  // 系统初始化
  #ifdef TEST_MODE
    // 在测试模式下，尽早初始化串口以捕获所有启动日志
    Serial.begin(115200);
  #endif

  log_manager_init();
  power_result_t power_init_result = power_manager_init();
  sensor_manager_init();
  actuator_manager_init();

#ifdef TEST_MODE
  if (power_init_result != POWER_OK) {
    // 此时日志系统已可用
    LOG_ERROR("Main", "Power manager initialization failed: %d", power_init_result);
  }
#endif

  #ifdef TEST_MODE
    // --- 测试模式初始化 ---
    // 1. 初始化核心测试模块 (物理接口已初始化)
    test_mode_init();
    test_cli_init();
    test_registry_init();

    // 3. 注册所有测试命令
    test_commands_core_init();
    test_commands_hal_init();
    test_commands_log_init();

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