/**
 * @file config_manager.cpp
 * @brief 配置管理器实现
 */

#include "config_manager.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include "nvs_flash.h"
#include "../managers/log_manager.h"

// NVS配置
static const char* NVS_NAMESPACE = "hydrosense";
static const char* KEY_CONFIG = "config";

static Preferences preferences;

ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::init() {
    // 初始化NVS Flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        LOG_ERROR("ConfigManager", "NVS needs erasing, re-initializing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    if (ret != ESP_OK) {
        LOG_ERROR("ConfigManager", "NVS Flash initialization failed!");
        return false;
    }

    // 打开Preferences命名空间
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        LOG_ERROR("ConfigManager", "Failed to open NVS namespace!");
        return false;
    }

    LOG_INFO("ConfigManager", "NVS initialization successful");
    m_initialized = true;

    // 加载配置
    return loadConfig();
}

hydro_config_t ConfigManager::getDefaultConfig() {
    hydro_config_t cfg;

    // 浇水配置默认值
    cfg.watering.threshold = 2000;           // ADC阈值
    cfg.watering.duration_ms = 3000;         // 浇水3秒
    cfg.watering.min_interval_s = 3600;      // 最小间隔1小时
    strncpy(cfg.watering.plant_type, "UnnamedPlant", sizeof(cfg.watering.plant_type) - 1);
    cfg.watering.plant_type[sizeof(cfg.watering.plant_type) - 1] = '\0';

    // WiFi配置默认值
    memset(cfg.wifi.ssid, 0, sizeof(cfg.wifi.ssid));
    memset(cfg.wifi.password, 0, sizeof(cfg.wifi.password));
    cfg.wifi.auth_mode = 0;  // 0=WPA2-PSK
    memset(cfg.wifi.identity, 0, sizeof(cfg.wifi.identity));
    memset(cfg.wifi.username, 0, sizeof(cfg.wifi.username));

    // LLM配置默认值
    memset(cfg.llm.base_url, 0, sizeof(cfg.llm.base_url));
    memset(cfg.llm.api_key, 0, sizeof(cfg.llm.api_key));
    strncpy(cfg.llm.model, "gpt-3.5-turbo", sizeof(cfg.llm.model) - 1);
    cfg.llm.model[sizeof(cfg.llm.model) - 1] = '\0';

    // 系统配置默认值
    cfg.system.ntp_enabled = true;
    strncpy(cfg.system.timezone, "CST-8", sizeof(cfg.system.timezone) - 1);
    cfg.system.timezone[sizeof(cfg.system.timezone) - 1] = '\0';
    strncpy(cfg.system.ntp_server, "pool.ntp.org", sizeof(cfg.system.ntp_server) - 1);
    cfg.system.ntp_server[sizeof(cfg.system.ntp_server) - 1] = '\0';

    return cfg;
}

bool ConfigManager::loadConfig() {
    // 首先获取默认配置
    m_config = getDefaultConfig();

    // 尝试从NVS读取配置
    String config_json_str = preferences.getString(KEY_CONFIG, "");

    if (config_json_str.length() == 0) {
        LOG_INFO("ConfigManager", "No saved configuration found, using default values");
        return saveConfig();  // 保存默认配置
    }

    // 解析JSON配置
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, config_json_str);

    if (error) {
        LOG_ERROR("ConfigManager", "JSON parse failed: %s", error.c_str());
        return false;
    }

    // 加载浇水配置
    JsonObject watering_obj = doc["watering"];
    if (!watering_obj.isNull()) {
        m_config.watering.threshold = watering_obj["threshold"] | m_config.watering.threshold;
        m_config.watering.duration_ms = watering_obj["duration_ms"] | m_config.watering.duration_ms;
        m_config.watering.min_interval_s = watering_obj["min_interval_s"] | m_config.watering.min_interval_s;

        const char* plant_type = watering_obj["plant_type"] | m_config.watering.plant_type;
        strncpy(m_config.watering.plant_type, plant_type, sizeof(m_config.watering.plant_type) - 1);
        m_config.watering.plant_type[sizeof(m_config.watering.plant_type) - 1] = '\0';
    }

    // 加载WiFi配置
    JsonObject wifi_obj = doc["wifi"];
    if (!wifi_obj.isNull()) {
        const char* ssid = wifi_obj["ssid"] | "";
        strncpy(m_config.wifi.ssid, ssid, sizeof(m_config.wifi.ssid) - 1);
        m_config.wifi.ssid[sizeof(m_config.wifi.ssid) - 1] = '\0';

        const char* password = wifi_obj["password"] | "";
        strncpy(m_config.wifi.password, password, sizeof(m_config.wifi.password) - 1);
        m_config.wifi.password[sizeof(m_config.wifi.password) - 1] = '\0';

        m_config.wifi.auth_mode = wifi_obj["auth_mode"] | m_config.wifi.auth_mode;

        const char* identity = wifi_obj["identity"] | "";
        strncpy(m_config.wifi.identity, identity, sizeof(m_config.wifi.identity) - 1);
        m_config.wifi.identity[sizeof(m_config.wifi.identity) - 1] = '\0';

        const char* username = wifi_obj["username"] | "";
        strncpy(m_config.wifi.username, username, sizeof(m_config.wifi.username) - 1);
        m_config.wifi.username[sizeof(m_config.wifi.username) - 1] = '\0';
    }

    // 加载LLM配置
    JsonObject llm_obj = doc["llm"];
    if (!llm_obj.isNull()) {
        const char* base_url = llm_obj["base_url"] | "";
        strncpy(m_config.llm.base_url, base_url, sizeof(m_config.llm.base_url) - 1);
        m_config.llm.base_url[sizeof(m_config.llm.base_url) - 1] = '\0';

        const char* api_key = llm_obj["api_key"] | "";
        strncpy(m_config.llm.api_key, api_key, sizeof(m_config.llm.api_key) - 1);
        m_config.llm.api_key[sizeof(m_config.llm.api_key) - 1] = '\0';

        const char* model = llm_obj["model"] | m_config.llm.model;
        strncpy(m_config.llm.model, model, sizeof(m_config.llm.model) - 1);
        m_config.llm.model[sizeof(m_config.llm.model) - 1] = '\0';
    }

    // 加载系统配置
    JsonObject system_obj = doc["system"];
    if (!system_obj.isNull()) {
        m_config.system.ntp_enabled = system_obj["ntp_enabled"] | m_config.system.ntp_enabled;

        const char* timezone = system_obj["timezone"] | m_config.system.timezone;
        strncpy(m_config.system.timezone, timezone, sizeof(m_config.system.timezone) - 1);
        m_config.system.timezone[sizeof(m_config.system.timezone) - 1] = '\0';

        const char* ntp_server = system_obj["ntp_server"] | m_config.system.ntp_server;
        strncpy(m_config.system.ntp_server, ntp_server, sizeof(m_config.system.ntp_server) - 1);
        m_config.system.ntp_server[sizeof(m_config.system.ntp_server) - 1] = '\0';
    }

    LOG_INFO("ConfigManager", "Configuration loaded successfully");
    return true;
}

bool ConfigManager::saveConfig() {
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Uninitialized, unable to save configuration");
        return false;
    }

    // 构建JSON配置
    JsonDocument doc;

    // 浇水配置
    JsonObject watering_obj = doc["watering"].to<JsonObject>();
    watering_obj["threshold"] = m_config.watering.threshold;
    watering_obj["duration_ms"] = m_config.watering.duration_ms;
    watering_obj["min_interval_s"] = m_config.watering.min_interval_s;
    watering_obj["plant_type"] = m_config.watering.plant_type;

    // WiFi配置
    JsonObject wifi_obj = doc["wifi"].to<JsonObject>();
    wifi_obj["ssid"] = m_config.wifi.ssid;
    wifi_obj["password"] = m_config.wifi.password;
    wifi_obj["auth_mode"] = m_config.wifi.auth_mode;
    wifi_obj["identity"] = m_config.wifi.identity;
    wifi_obj["username"] = m_config.wifi.username;

    // LLM配置
    JsonObject llm_obj = doc["llm"].to<JsonObject>();
    llm_obj["base_url"] = m_config.llm.base_url;
    llm_obj["api_key"] = m_config.llm.api_key;
    llm_obj["model"] = m_config.llm.model;

    // 系统配置
    JsonObject system_obj = doc["system"].to<JsonObject>();
    system_obj["ntp_enabled"] = m_config.system.ntp_enabled;
    system_obj["timezone"] = m_config.system.timezone;
    system_obj["ntp_server"] = m_config.system.ntp_server;

    // 序列化到字符串
    String json_output;
    serializeJson(doc, json_output);

    // 写入NVS
    size_t written = preferences.putString(KEY_CONFIG, json_output);
    if (written == 0) {
        LOG_ERROR("ConfigManager", "Failed to write to NVS!");
        return false;
    }

    LOG_INFO("ConfigManager", "Configuration saved successfully (%u bytes)", written);
    return true;
}

bool ConfigManager::resetToDefault() {
    LOG_INFO("ConfigManager", "Reset to default settings");
    m_config = getDefaultConfig();
    return saveConfig();
}

hydro_config_t& ConfigManager::getConfig() {
    return m_config;
}

String ConfigManager::getConfigJson() {
    JsonDocument doc;

    // 浇水配置
    JsonObject watering_obj = doc["watering"].to<JsonObject>();
    watering_obj["threshold"] = m_config.watering.threshold;
    watering_obj["duration_ms"] = m_config.watering.duration_ms;
    watering_obj["min_interval_s"] = m_config.watering.min_interval_s;
    watering_obj["plant_type"] = m_config.watering.plant_type;

    // WiFi配置（隐藏密码和API Key）
    JsonObject wifi_obj = doc["wifi"].to<JsonObject>();
    wifi_obj["ssid"] = m_config.wifi.ssid;
    wifi_obj["password"] = strlen(m_config.wifi.password) > 0 ? "***" : "";
    wifi_obj["auth_mode"] = m_config.wifi.auth_mode;
    wifi_obj["identity"] = m_config.wifi.identity;
    wifi_obj["username"] = m_config.wifi.username;

    // LLM配置（隐藏API Key）
    JsonObject llm_obj = doc["llm"].to<JsonObject>();
    llm_obj["base_url"] = m_config.llm.base_url;
    llm_obj["api_key"] = strlen(m_config.llm.api_key) > 0 ? "***" : "";
    llm_obj["model"] = m_config.llm.model;

    // 系统配置
    JsonObject system_obj = doc["system"].to<JsonObject>();
    system_obj["ntp_enabled"] = m_config.system.ntp_enabled;
    system_obj["timezone"] = m_config.system.timezone;
    system_obj["ntp_server"] = m_config.system.ntp_server;

    String json_output;
    serializeJsonPretty(doc, json_output);
    return json_output;
}
