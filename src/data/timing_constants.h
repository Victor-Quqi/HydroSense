/**
 * @file timing_constants.h
 * @brief Centralized timing constants for the entire system
 * @details
 *   This file follows the architecture principle of "数据层: 负责共享数据结构和配置".
 *   All timing-related constants (durations, intervals, delays, timeouts) are
 *   centralized here to avoid scattered hardcoded values across the codebase.
 *
 *   按照《代码架构总纲》的设计原则，所有时间相关的常量集中在此文件管理，
 *   避免在各个模块中分散定义，便于统一调整和维护。
 */

#ifndef TIMING_CONSTANTS_H
#define TIMING_CONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Input Manager Timing Constants
// =============================================================================

/**
 * @brief 双击检测时间窗口 (ms)
 * @details 两次单击之间的最大间隔，超过此时间则判定为单击
 */
#define INPUT_DOUBLE_CLICK_INTERVAL_MS 233

/**
 * @brief 编码器去抖阈值
 * @details 累积增量超过此值才触发一次有效旋转事件
 */
#define INPUT_ENCODER_THRESHOLD 4

// =============================================================================
// Test Commands Timing Constants
// =============================================================================

/**
 * @brief input poll命令持续时间 (ms)
 * @details 用于测试输入事件检测的轮询持续时间
 */
#define TEST_INPUT_POLL_DURATION_MS 10000

/**
 * @brief interactive poll命令持续时间 (ms)
 * @details 用于测试interactive模式状态机的轮询持续时间
 */
#define TEST_INTERACTIVE_POLL_DURATION_MS 20000

/**
 * @brief 测试命令主循环延迟 (ms)
 * @details 平衡CPU占用和响应速度，1ms延迟提供良好的输入响应性
 */
#define TEST_LOOP_DELAY_MS 1

// =============================================================================
// Interactive Mode Timing Constants
// =============================================================================

/**
 * @brief Interactive模式主循环延迟 (ms)
 * @details 在正常运行模式下的循环延迟
 */
#define INTERACTIVE_LOOP_DELAY_MS 10

/**
 * @brief UI刷新最小间隔 (ms)
 * @details 防止屏幕刷新过于频繁，降低功耗
 */
#define INTERACTIVE_UI_REFRESH_INTERVAL_MS 50

// =============================================================================
// System-Level Timing Constants
// =============================================================================

/**
 * @brief 主循环默认延迟 (ms)
 * @details 系统主循环的默认延迟时间
 */
#define SYSTEM_MAIN_LOOP_DELAY_MS 10

/**
 * @brief 传感器采样间隔 (ms)
 * @details RUN模式下传感器定期采样的时间间隔
 */
#define SYSTEM_SENSOR_SAMPLE_INTERVAL_MS 5000

/**
 * @brief 看门狗超时时间 (ms)
 * @details 系统看门狗超时阈值
 */
#define SYSTEM_WATCHDOG_TIMEOUT_MS 30000

// =============================================================================
// Power Management Timing Constants
// =============================================================================

/**
 * @brief 外设上电稳定时间 (ms)
 * @details 打开外设电源后等待稳定的时间
 */
#define POWER_STABILIZATION_DELAY_MS 100

/**
 * @brief 外设断电前延迟 (ms)
 * @details 关闭外设前的安全延迟
 */
#define POWER_SHUTDOWN_DELAY_MS 50

#ifdef __cplusplus
}
#endif

#endif // TIMING_CONSTANTS_H
