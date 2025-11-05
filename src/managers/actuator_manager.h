/**
 * @file actuator_manager.h
 * @brief 执行器管理器
 * @details 负责水泵和电磁阀等执行器的控制
 */

#ifndef ACTUATOR_MANAGER_H
#define ACTUATOR_MANAGER_H

#include <stdint.h>

/**
 * @brief 初始化执行器管理器
 * @details 该函数应在系统启动时调用一次
 */
void actuator_manager_init();

/**
 * @brief 运行水泵指定时长
 * @param duration_ms 运行的时长 (毫秒)
 */
void actuator_manager_run_pump(uint32_t duration_ms);

#endif // ACTUATOR_MANAGER_H