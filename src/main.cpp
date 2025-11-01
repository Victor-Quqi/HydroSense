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
#endif

void setup() {
  // 系统初始化

  #ifdef TEST_MODE
    // 初始化串口，用于CLI通信
    Serial.begin(115200);
    // 测试模式初始化
    test_mode_init();
    test_cli_init();
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