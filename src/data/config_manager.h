/**
 * @file config_manager.h
 * @brief 配置管理器
 * @details 负责系统配置的持久化存储与访问
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "data_models.h"
#include "esp_err.h"

#define NVS_CONFIG_NAMESPACE "hydro_config"

/**
 * @brief 初始化配置管理器
 * @return esp_err_t 初始化结果
 */
esp_err_t config_manager_init(void);

#endif // CONFIG_MANAGER_H