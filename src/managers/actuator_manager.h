/**
 * @file actuator_manager.h
 * @brief 执行器管理器
 * @details 负责水泵等执行器的控制
 */

#ifndef ACTUATOR_MANAGER_H
#define ACTUATOR_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 初始化执行器管理器
 * @details 该函数应在系统启动时调用一次
 */
void actuator_manager_init();

/**
 * @brief 启动水泵
 * @param duty_cycle 功率 (0-255)
 */
void actuator_manager_start_pump(uint8_t duty_cycle);

/**
 * @brief 停止水泵
 */
void actuator_manager_stop_pump();

/**
 * @brief 运行水泵指定时长 (非阻塞)
 * @details 此函数会立即返回。水泵将在后台运行指定时间后自动停止。
 *          需要周期性调用 actuator_manager_loop() 来驱动此逻辑。
 * @param duty_cycle 功率 (0-255)
 * @param duration_ms 运行的时长 (毫秒)
 */
void actuator_manager_run_pump_for(uint8_t duty_cycle, uint32_t duration_ms);

/**
 * @brief 执行器管理器的循环函数
 * @details 该函数应在主循环中被周期性调用，以处理非阻塞逻辑。
 */
void actuator_manager_loop();

/**
 * @brief 查询水泵是否正在运行
 * @return true 水泵正在运行, false 水泵已停止
 */
bool actuator_manager_is_pump_running();

#endif // ACTUATOR_MANAGER_H