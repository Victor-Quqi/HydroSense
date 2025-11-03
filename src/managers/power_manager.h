/**
 * @file power_manager.h
 * @brief 电源管理器
 * @details 负责系统电源门控和功耗管理控制
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <stdbool.h>

/**
 * @brief 电源管理器操作结果枚举
 */
typedef enum {
    POWER_OK = 0,              ///< 操作成功
    POWER_ERROR_GPIO_FAILED,   ///< GPIO操作失败
    POWER_ERROR_INVALID_PARAM, ///< 无效参数
    POWER_ERROR_NOT_INIT       ///< 电源管理器未初始化
} power_result_t;

/**
 * @brief 初始化电源管理器
 * @details 设置所有电源门控GPIO为输出模式，并默认关闭所有外设电源
 * @return power_result_t 初始化结果
 */
power_result_t power_manager_init();

/**
 * @brief 控制传感器电源
 * @param enable true: 打开电源, false: 关闭电源
 * @return power_result_t 操作结果
 */
power_result_t power_sensor_enable(bool enable);

/**
 * @brief 控制12V升压模块电源 (水泵和电磁阀)
 * @param enable true: 打开电源, false: 关闭电源
 * @return power_result_t 操作结果
 */
power_result_t power_pump_module_enable(bool enable);

/**
 * @brief 控制墨水屏电源
 * @param enable true: 打开电源, false: 关闭电源
 * @return power_result_t 操作结果
 */
power_result_t power_screen_enable(bool enable);

/**
 * @brief 查询传感器电源状态 (新命名)
 * @return bool true: 已打开, false: 已关闭
 */
bool power_sensor_is_enabled();

/**
 * @brief 查询12V升压模块电源状态 (新命名)
 * @return bool true: 已打开, false: 已关闭
 */
bool power_pump_module_is_enabled();

/**
 * @brief 查询墨水屏电源状态 (新命名)
 * @return bool true: 已打开, false: 已关闭
 */
bool power_screen_is_enabled();

/* ========== 兼容性函数 (保留旧命名) ========== */
/**
 * @brief 查询传感器电源状态 (旧命名，兼容性保留)
 * @deprecated 请使用 power_sensor_is_enabled()
 * @return bool true: 已打开, false: 已关闭
 */
bool is_sensor_power_on();

/**
 * @brief 查询12V升压模块电源状态 (旧命名，兼容性保留)
 * @deprecated 请使用 power_pump_module_is_enabled()
 * @return bool true: 已打开, false: 已关闭
 */
bool is_pump_module_power_on();

/**
 * @brief 查询墨水屏电源状态 (旧命名，兼容性保留)
 * @deprecated 请使用 power_screen_is_enabled()
 * @return bool true: 已打开, false: 已关闭
 */
bool is_screen_power_on();

#endif // POWER_MANAGER_H