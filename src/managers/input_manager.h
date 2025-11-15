/**
 * @file input_manager.h
 * @brief 物理输入管理器
 * @details 负责处理模式开关、旋转编码器等物理输入设备，并向上层提供状态。
 */

#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <stdint.h>

/**
 * @brief 系统运行模式枚举
 */
typedef enum {
    SYSTEM_MODE_OFF,          // 关机模式
    SYSTEM_MODE_RUN,          // 自主运行模式
    SYSTEM_MODE_INTERACTIVE,  // 人机交互模式
    SYSTEM_MODE_UNKNOWN       // 未知状态
} system_mode_t;

/**
 * @brief 初始化输入管理器
 * @details 配置模式开关和旋转编码器所需的GPIO引脚。
 */
void input_manager_init();

/**
 * @brief 获取当前由物理开关决定的系统模式
 * @return system_mode_t 当前的系统模式
 */
system_mode_t input_manager_get_mode();

/**
 * @brief 输入管理器的循环处理函数
 * @details 应在主循环中调用，以处理旋转编码器等需要轮询的输入设备。
 */
void input_manager_loop();

#endif // INPUT_MANAGER_H