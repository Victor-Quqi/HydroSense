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
#include "ui/ui_manager.h"
#include "managers/input_manager.h"
#include "hal/hal_rtc.h"

// --- 私有状态变量 ---
static system_mode_t current_mode = SYSTEM_MODE_UNKNOWN;
static system_mode_t last_read_mode = SYSTEM_MODE_UNKNOWN;
static unsigned long last_debounce_time = 0;
static const unsigned long debounce_delay = 50; // 50ms 消抖延迟

// --- 私有函数 ---
static void enter_off_mode_logic();

#ifdef TEST_MODE
/**
 * @brief 初始化测试模式相关的模块
 * @details 封装所有仅在测试模式下运行的初始化代码
 */
static void test_mode_setup() {
  // 在测试模式下，尽早初始化串口以捕获所有启动日志
  Serial.begin(115200);

  // 初始化核心测试模块
  test_mode_init();
  test_cli_init();
  test_registry_init();

  // 注册所有测试命令
  test_commands_core_init();
  test_commands_hal_init();
  test_commands_log_init();
}
#endif

void setup() {
  // 系统初始化
  #ifdef TEST_MODE
    test_mode_setup();
  #endif

  log_manager_init();
  power_result_t power_init_result = power_manager_init();
  sensor_manager_init();
  actuator_manager_init();
  input_manager_init();
  hal_rtc_init();
  ui_manager_init();

  #ifdef TEST_MODE
    if (power_init_result != POWER_OK) {
      // 此时日志系统已可用
      LOG_ERROR("Main", "Power manager initialization failed: %d", power_init_result);
    }
  #else
    // 在正常模式下，获取初始模式

  #endif
}

void loop() {
  // 主循环

  #ifdef TEST_MODE
    actuator_manager_loop();
    ui_manager_loop();
    input_manager_loop();
    // 测试模式处理 - 禁用深度睡眠，持续响应CLI
    test_cli_loop();
  #else
    // 正常运行模式 - 包含深度睡眠逻辑
    system_mode_t reading = input_manager_get_mode();

    // 如果读到的模式与上次不同，重置消抖计时器
    if (reading != last_read_mode) {
        last_debounce_time = millis();
    }
    last_read_mode = reading;

    // 如果距离上次模式变化已经超过了消抖延迟
    if ((millis() - last_debounce_time) > debounce_delay) {
        // 并且这个稳定的模式与当前激活的模式不同
        if (reading != current_mode) {
            LOG_INFO("Main", "Mode changed from %d to %d", current_mode, reading);
            current_mode = reading; // 采纳新的稳定模式

            if (current_mode == SYSTEM_MODE_OFF) {
                enter_off_mode_logic();
            }
            // 在这里可以添加其他模式切换的逻辑
        }
    }
  #endif
}

/**
 * @brief 执行进入OFF模式的完整序列
 */
static void enter_off_mode_logic() {
    LOG_INFO("Main", "Entering OFF mode...");

    // 1. 确保执行器停止
    actuator_manager_stop_pump();

    // 2. 显示关机屏幕
    power_screen_enable(true);
    ui_manager_show_shutdown_screen();
    delay(200); // 等待屏幕刷新指令发送

    // 3. 关闭所有外设电源
    power_sensor_enable(false);
    power_pump_module_enable(false);
    power_screen_enable(false);

    // 4. 进入深度睡眠
    hal_rtc_enter_deep_sleep();
}