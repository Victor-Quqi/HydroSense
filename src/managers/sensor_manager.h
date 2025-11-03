/**
 * @file sensor_manager.h
 * @brief 传感器管理器
 * @details 负责采集和处理所有传感器数据
 */

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdint.h>
#include "data/data_models.h"

/**
 * @brief 传感器管理器操作结果枚举
 */
typedef enum {
    SENSOR_OK = 0,              ///< 操作成功
    SENSOR_ERROR_NOT_INIT,      ///< 管理器未初始化
    SENSOR_ERROR_POWER_FAILED,  ///< 传感器电源操作失败
    SENSOR_ERROR_READ_FAILED    ///< 传感器读数失败
} sensor_result_t;

/**
 * @brief 初始化传感器管理器
 * @return sensor_result_t 初始化结果
 */
sensor_result_t sensor_manager_init();

/**
 * @brief 读取所有传感器数据
 * @details 这是一个阻塞操作，会打开传感器电源，读取数据，然后关闭电源
 * @param p_sensor_data 指向用于存储传感器数据的结构体指针
 * @return sensor_result_t 操作结果
 */
sensor_result_t sensor_manager_read_all(sensor_data_t* p_sensor_data);

/**
 * @brief 读取土壤湿度传感器数据
 * @details 这是一个阻塞操作，会打开传感器电源，读取数据，然后关闭电源
 * @param p_humidity 指向用于存储湿度值的指针
 * @return sensor_result_t 操作结果
 */
sensor_result_t sensor_manager_get_humidity(float* p_humidity);

/**
 * @brief 读取电池电压
 * @details 直接读取ADC值并进行电压转换，无需控制传感器电源
 * @param p_voltage 指向用于存储电压值的指针
 * @return sensor_result_t 操作结果
 */
sensor_result_t sensor_manager_get_battery_voltage(float* p_voltage);

#endif // SENSOR_MANAGER_H