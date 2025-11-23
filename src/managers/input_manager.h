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

/**
 * @brief 获取旋转编码器的增量值（消费型API）
 * @details 返回增量后自动清零。顺时针为正，逆时针为负。
 * @return int8_t 旋转增量（范围：-1, 0, +1）
 */
int8_t input_manager_get_encoder_delta();

/**
 * @brief 检查按键是否被单击（消费型API）
 * @details 如果检测到单击事件，返回true并自动清除标志。
 * @return bool true=检测到单击事件，false=无单击事件
 */
bool input_manager_get_button_clicked();

/**
 * @brief 检查按键是否被双击（消费型API）
 * @details 如果检测到双击事件（两次点击间隔<双击间隔阈值），返回true并自动清除标志。
 * @return bool true=检测到双击事件，false=无双击事件
 */
bool input_manager_get_button_double_clicked();

/**
 * @brief 清除所有累积的编码器和按键状态
 * @details 用于状态切换时避免累积值干扰，手动调用清零。
 */
void input_manager_clear_events();

#endif // INPUT_MANAGER_H