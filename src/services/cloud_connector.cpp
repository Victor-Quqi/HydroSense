/**
 * @file cloud_connector.cpp
 * @brief 云端连接器实现
 */

#include "cloud_connector.h"
#include "wifi_manager.h"
#include "time_manager.h"
#include "log_manager.h"
#include <esp_http_client.h>
#include <cJSON.h>
#include <string.h>

static cloud_state_t s_cloud_state = CLOUD_DISCONNECTED;
static char s_api_endpoint[256] = {0};
static char s_api_key[64] = {0};

// HTTP请求回调
static esp_err_t http_event_handler(esp_http_client_event_t* evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ON_SUCCESS:
            LOG_DEBUG("Cloud", "HTTP request succeeded");
            break;
        case HTTP_EVENT_ON_ERROR:
            LOG_ERROR("Cloud", "HTTP request failed");
            break;
        default:
            break;
    }
    return ESP_OK;
}

void cloud_connector_init(void) {
    s_cloud_state = CLOUD_DISCONNECTED;
    LOG_DEBUG("Cloud", "Connector initialized");
}

void cloud_connect(const char* api_endpoint, const char* api_key) {
    if (!api_endpoint || !api_key) {
        LOG_ERROR("Cloud", "Invalid endpoint or API key");
        return;
    }

    strncpy(s_api_endpoint, api_endpoint, sizeof(s_api_endpoint)-1);
    strncpy(s_api_key, api_key, sizeof(s_api_key)-1);
    
    if (wifi_get_state() == WIFI_CONNECTED) {
        s_cloud_state = CLOUD_CONNECTED;
        LOG_INFO("Cloud", "Connected to %s", api_endpoint);
    } else {
        s_cloud_state = CLOUD_CONNECTING;
        LOG_WARN("Cloud", "Waiting for Wi-Fi to connect to cloud");
    }
}

void cloud_disconnect(void) {
    s_cloud_state = CLOUD_DISCONNECTED;
    LOG_INFO("Cloud", "Disconnected from cloud");
}

cloud_state_t cloud_get_state(void) {
    return s_cloud_state;
}

bool cloud_send_sensor_data(const sensor_data_t* data) {
    if (s_cloud_state != CLOUD_CONNECTED || !data) return false;

    // 创建JSON payload
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "timestamp", data->timestamp);
    cJSON_AddNumberToObject(root, "soil_moisture", data->soil_moisture);
    cJSON_AddNumberToObject(root, "temperature", data->temperature);
    cJSON_AddNumberToObject(root, "battery_voltage", data->battery_voltage);
    char* payload = cJSON_Print(root);

    // 配置HTTP客户端
    esp_http_client_config_t config = {
        .url = s_api_endpoint,
        .event_handler = http_event_handler,
        .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    // 设置请求头
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", s_api_key);
    esp_http_client_set_post_field(client, payload, strlen(payload));

    // 发送请求
    esp_err_t err = esp_http_client_perform(client);
    bool success = (err == ESP_OK && esp_http_client_get_status_code(client) == 200);
    
    // 清理资源
    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(payload);

    if (success) {
        LOG_INFO("Cloud", "Sensor data sent successfully");
    } else {
        LOG_ERROR("Cloud", "Failed to send sensor data");
    }
    return success;
}

bool cloud_get_ai_report(char* buffer, size_t buffer_size) {
    if (s_cloud_state != CLOUD_CONNECTED || !buffer || buffer_size == 0) return false;

    // 构建请求URL（假设AI报告接口为端点+"/ai/report"）
    char url[300];
    snprintf(url, sizeof(url), "%s/ai/report", s_api_endpoint);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .method = HTTP_METHOD_GET,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Authorization", s_api_key);

    esp_err_t err = esp_http_client_perform(client);
    bool success = false;

    if (err == ESP_OK && esp_http_client_get_status_code(client) == 200) {
        // 读取响应数据
        int len = esp_http_client_read(client, buffer, buffer_size - 1);
        if (len > 0) {
            buffer[len] = '\0';
            success = true;
            LOG_INFO("Cloud", "AI report received");
        }
    }

    esp_http_client_cleanup(client);
    return success;
}

void cloud_loop(void) {
    // 检查Wi-Fi状态更新云端状态
    if (s_cloud_state == CLOUD_CONNECTING && wifi_get_state() == WIFI_CONNECTED) {
        s_cloud_state = CLOUD_CONNECTED;
        LOG_INFO("Cloud", "Connected to cloud (Wi-Fi ready)");
    } else if (s_cloud_state == CLOUD_CONNECTED && wifi_get_state() != WIFI_CONNECTED) {
        s_cloud_state = CLOUD_CONNECTING;
        LOG_WARN("Cloud", "Disconnected (Wi-Fi down)");
    }
}