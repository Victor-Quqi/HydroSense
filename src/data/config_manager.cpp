/**
 * @file config_manager.cpp
 * @brief 配置管理器实现
 * *@details 基于ESP32 NVS实现系统配置的持久化存储与访问
 */

#include "config_manager.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

// 静态全局变量：内存中的当前配置
static SystemConfig s_current_config;
static bool s_initialized = false;

// 默认配置参数
static const SystemConfig s_default_config = {
    .watering_threshold = 300,       // 土壤湿度低于300触发浇水
    .watering_duration = 2000,       // 单次浇水2秒
    .sampling_interval = 1800,       // 30分钟采样一次
    .auto_watering_enabled = true,   // 默认启用自动浇水
    .device_mode = 0                 // 默认运行模式（RUN）
};

esp_err_t config_manager_init(void) {
    // 初始化NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS分区损坏，尝试擦除后重新初始化
        if (nvs_flash_erase() != ESP_OK) {
            return ESP_FAIL;
        }
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        return err;
    }

    // 打开NVS命名空间
    nvs_handle_t nvs_handle;
    err = nvs_open(NVS_CONFIG_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    // 读取配置（若不存在则使用默认值）
    size_t config_size = sizeof(SystemConfig);
    err = nvs_get_blob(nvs_handle, "system_config", &s_current_config, &config_size);
    if (err != ESP_OK) {
        // 首次启动无配置，使用默认值
        memcpy(&s_current_config, &s_default_config, sizeof(SystemConfig));
    }

    nvs_close(nvs_handle);
    s_initialized = true;
    return ESP_OK;
}

SystemConfig* config_manager_get_current(void) {
    return s_initialized ? &s_current_config : NULL;
}

esp_err_t config_manager_save(SystemConfig* config) {
    if (!s_initialized || config == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // 打开NVS命名空间
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_CONFIG_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    // 写入配置到NVS
    err = nvs_set_blob(nvs_handle, "system_config", config, sizeof(SystemConfig));
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    // 提交写入
    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    // 更新内存中的配置
    if (err == ESP_OK) {
        memcpy(&s_current_config, config, sizeof(SystemConfig));
    }
    return err;
}

void config_manager_restore_default(void) {
    if (s_initialized) {
        memcpy(&s_current_config, &s_default_config, sizeof(SystemConfig));
    }
}