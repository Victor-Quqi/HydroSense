/**
 * @file config_manager.h
 * @brief 配置管理器
 * @details 负责从NVS加载/保存系统配置
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "data_models.h"
#include <esp_err.h>

// ============================================================================
// 配置管理常量
// ============================================================================
#define NVS_CONFIG_NAMESPACE "hydro_config"  // NVS存储命名空间
#define TEST_CLI_BAUD_RATE 115200            // 测试模式串口波特率

// ============================================================================
// 配置管理器接口
// ============================================================================

/**
 * @brief 初始化配置管理器（初始化NVS并加载配置）
 * @return esp_err_t ESP_OK表示成功，其他为错误码
 */
esp_err_t config_manager_init(void);

/**
 * @brief 获取当前系统配置（返回配置指针）
 * @return SystemConfig* 配置结构体指针（NULL表示未初始化）
 */
SystemConfig* config_manager_get_current(void);

/**
 * @brief 保存配置到NVS
 * @param config 待保存的配置结构体
 * @return esp_err_t ESP_OK表示成功，其他为错误码
 */
esp_err_t config_manager_save(SystemConfig* config);

/**
 * @brief 恢复默认配置（不写入NVS，仅更新内存中的配置）
 */
void config_manager_restore_default(void);

#endif // CONFIG_MANAGER_H