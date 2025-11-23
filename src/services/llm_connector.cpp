/**
 * @file llm_connector.cpp
 * @brief LLM连接器实现 - 支持多轮对话和动态选项
 */

#include "llm_connector.h"
#include "config_manager.h"
#include "wifi_manager.h"
#include "history_manager.h"
#include "../managers/sensor_manager.h"
#include "time_manager.h"
#include "../managers/log_manager.h"
#include <ArduinoJson.h>

LLMConnector* LLMConnector::s_instance = nullptr;

LLMConnector& LLMConnector::instance() {
    if (s_instance == nullptr) {
        s_instance = new LLMConnector();
    }
    return *s_instance;
}

LLMConnector::LLMConnector() :
    m_state(LLMState::IDLE)
{
    memset(m_last_error, 0, sizeof(m_last_error));
}

bool LLMConnector::init() {
    LOG_INFO("LLMConnector", "LLM connector initializing...");

    // 检查配置
    hydro_config_t& config = ConfigManager::instance().getConfig();

    if (strlen(config.llm.base_url) == 0) {
        LOG_WARN("LLMConnector", "LLM base_url not configured");
    }

    if (strlen(config.llm.api_key) == 0) {
        LOG_WARN("LLMConnector", "LLM api_key not configured");
    }

    LOG_INFO("LLMConnector", "LLM connector initialized (model: %s)", config.llm.model);
    return true;
}

const char* LLMConnector::getSystemPrompt(bool with_options) {
    if (with_options) {
        // System prompt requesting options
        return "You are a plant assistant in a smart plant monitoring system. "
               "You can sense soil moisture, temperature, and other sensor data, and chat with users based on this data. "
               "Please respond in a concise and friendly tone, as if the plant is speaking. "
               "Keep responses under 50 words.\n\n"
               "IMPORTANT: You must return a JSON response in the following format:\n"
               "{\"response\": \"your response here\", \"options\": [\"option 1\", \"option 2\", \"option 3\"]}\n"
               "The options array should contain 3 conversation choices for the user (max 15 words each). "
               "You do NOT have the ability to water or adjust settings, so don't suggest options that imply you can.";
    } else {
        // Regular conversation prompt
        return "You are a plant assistant in a smart plant monitoring system. "
               "You can sense soil moisture, temperature, and other sensor data, and chat with users based on this data. "
               "Please respond in a concise and friendly tone, as if the plant is speaking. "
               "Keep responses under 50 words.";
    }
}

String LLMConnector::buildChatRequest(const char* user_message, bool use_history) {
    hydro_config_t& config = ConfigManager::instance().getConfig();

    // 构建OpenAI兼容格式的请求
    JsonDocument doc;
    doc["model"] = config.llm.model;
    doc["max_tokens"] = 200;
    doc["temperature"] = 0.7;

    // 添加消息
    JsonArray messages = doc["messages"].to<JsonArray>();

    // ===== 1. 系统提示词 =====
    JsonObject system_msg = messages.add<JsonObject>();
    system_msg["role"] = "system";
    system_msg["content"] = getSystemPrompt(use_history);

    // ===== 2. 系统状态（只发一次，最新）=====
    sensor_data_t sensor_data;
    sensor_manager_read_all(&sensor_data);

    // 计算湿度百分比
    float humidity_pct = 0.0f;
    if (config.watering.humidity_dry > config.watering.humidity_wet) {
        humidity_pct = 100.0f - ((sensor_data.soil_moisture - config.watering.humidity_wet) * 100.0f) /
                      (config.watering.humidity_dry - config.watering.humidity_wet);
        if (humidity_pct < 0.0f) humidity_pct = 0.0f;
        if (humidity_pct > 100.0f) humidity_pct = 100.0f;
    }

    // 计算阈值百分比
    float threshold_pct = 0.0f;
    if (config.watering.humidity_dry > config.watering.humidity_wet) {
        threshold_pct = 100.0f - ((config.watering.threshold - config.watering.humidity_wet) * 100.0f) /
                       (config.watering.humidity_dry - config.watering.humidity_wet);
        if (threshold_pct < 0.0f) threshold_pct = 0.0f;
        if (threshold_pct > 100.0f) threshold_pct = 100.0f;
    }

    // 获取网络状态
    WiFiManager& wifi = WiFiManager::instance();
    bool wifi_connected = wifi.isConnected();
    const char* ssid = wifi_connected ? WiFi.SSID().c_str() : "未连接";

    // 获取时间
    TimeManager& time_mgr = TimeManager::instance();
    bool time_synced = time_mgr.isTimeSynced();
    String time_str;
    if (time_synced) {
        time_t now = time_mgr.getTimestamp();
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        char time_buf[32];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        time_str = String(time_buf);
    } else {
        time_str = "未同步";
    }

    // 构建系统状态字符串
    char status[512];
    snprintf(status, sizeof(status),
             "系统状态 -\n"
             "传感器: 湿度%d ADC (%.0f%%), 电池%.2fV\n"
             "配置: 阈值%d (%.0f%%), 功率%d, 时长%dms, 间隔%ds, 范围%d-%d\n"
             "网络: WiFi=%s(%s), 时间=%s",
             sensor_data.soil_moisture, humidity_pct,
             sensor_data.battery_voltage,
             config.watering.threshold, threshold_pct,
             config.watering.power,
             config.watering.duration_ms,
             config.watering.min_interval_s,
             config.watering.humidity_wet,
             config.watering.humidity_dry,
             wifi_connected ? "已连接" : "未连接",
             ssid,
             time_str.c_str());

    JsonObject status_msg = messages.add<JsonObject>();
    status_msg["role"] = "system";
    status_msg["content"] = status;

    // ===== 3. 日志摘要（只发一次，最新20条）=====
    String recent_logs = log_manager_get_recent_logs(20);
    String log_summary = "最近系统日志:\n" + recent_logs;

    JsonObject log_msg = messages.add<JsonObject>();
    log_msg["role"] = "system";
    log_msg["content"] = log_summary;

    // ===== 4. 对话历史（如果use_history）=====
    if (use_history) {
        HistoryManager::instance().buildContextMessages(messages);
    }

    // ===== 5. 当前用户消息 =====
    JsonObject user_msg = messages.add<JsonObject>();
    user_msg["role"] = "user";
    user_msg["content"] = user_message;

    String request_json;
    serializeJson(doc, request_json);
    return request_json;
}

bool LLMConnector::parseChatResponse(const String& response_json, char* output_buffer, size_t buffer_size) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response_json);

    if (error) {
        snprintf(m_last_error, sizeof(m_last_error), "JSON parse error: %s", error.c_str());
        LOG_ERROR("LLMConnector", "%s", m_last_error);
        return false;
    }

    // 提取响应内容
    const char* content = doc["choices"][0]["message"]["content"];
    if (content == nullptr) {
        snprintf(m_last_error, sizeof(m_last_error), "No content in response");
        LOG_ERROR("LLMConnector", "%s", m_last_error);
        return false;
    }

    strncpy(output_buffer, content, buffer_size - 1);
    output_buffer[buffer_size - 1] = '\0';
    return true;
}

bool LLMConnector::parseStructuredResponse(const String& response_json,
                                           char* response_buffer, size_t response_size,
                                           char options[][64], uint8_t* option_count) {
    // 首先解析LLM API响应
    JsonDocument api_doc;
    DeserializationError error = deserializeJson(api_doc, response_json);

    if (error) {
        snprintf(m_last_error, sizeof(m_last_error), "JSON parse error: %s", error.c_str());
        LOG_ERROR("LLMConnector", "%s", m_last_error);
        return false;
    }

    // 提取content
    const char* content = api_doc["choices"][0]["message"]["content"];
    if (content == nullptr) {
        snprintf(m_last_error, sizeof(m_last_error), "No content in response");
        LOG_ERROR("LLMConnector", "%s", m_last_error);
        return false;
    }

    LOG_DEBUG("LLMConnector", "Raw content: %s", content);

    // 解析content中的JSON结构
    JsonDocument content_doc;
    error = deserializeJson(content_doc, content);

    if (error) {
        // 如果不是JSON格式，直接返回纯文本（兼容模式）
        LOG_WARN("LLMConnector", "Content is not JSON, using as plain text");
        strncpy(response_buffer, content, response_size - 1);
        response_buffer[response_size - 1] = '\0';
        *option_count = 0;
    } else {
        // 提取response字段
        const char* response_text = content_doc["response"];
        if (response_text) {
            strncpy(response_buffer, response_text, response_size - 1);
            response_buffer[response_size - 1] = '\0';
        } else {
            snprintf(m_last_error, sizeof(m_last_error), "No response field in content");
            LOG_ERROR("LLMConnector", "%s", m_last_error);
            return false;
        }

        // 提取options字段
        JsonArray options_array = content_doc["options"].as<JsonArray>();
        *option_count = 0;

        if (!options_array.isNull()) {
            for (JsonVariant option : options_array) {
                if (*option_count >= 3) break;
                strncpy(options[*option_count], option.as<const char*>(), 63);
                options[*option_count][63] = '\0';
                (*option_count)++;
            }
        }
    }

    LOG_INFO("LLMConnector", "Parsed response with %d options", *option_count);

    // 如果没有选项，提供默认选项以保证对话可以继续
    if (*option_count == 0) {
        LOG_WARN("LLMConnector", "No options generated, adding default fallback options");
        strncpy(options[0], "继续聊聊", 63);
        strncpy(options[1], "查看传感器数据", 63);
        strncpy(options[2], "支持Prof.黄，谢谢喵", 63);
        *option_count = 3;
    }

    return true;
}

bool LLMConnector::chat(const char* user_message, char* response_buffer, size_t buffer_size) {
    // 检查WiFi连接
    if (!WiFiManager::instance().isConnected()) {
        snprintf(m_last_error, sizeof(m_last_error), "WiFi not connected");
        LOG_ERROR("LLMConnector", "%s", m_last_error);
        return false;
    }

    // 检查配置
    hydro_config_t& config = ConfigManager::instance().getConfig();
    if (strlen(config.llm.base_url) == 0 || strlen(config.llm.api_key) == 0) {
        snprintf(m_last_error, sizeof(m_last_error), "LLM not configured");
        LOG_ERROR("LLMConnector", "%s", m_last_error);
        return false;
    }

    LOG_INFO("LLMConnector", "Sending chat request: %s", user_message);
    m_state = LLMState::CONNECTING;

    // 创建HTTPS客户端
    WiFiClientSecure client;
    HTTPClient http;

    // 跳过证书验证（演示模式）
    client.setInsecure();

    // 构建完整URL
    String url = String(config.llm.base_url);
    if (!url.endsWith("/")) {
        url += "/";
    }
    url += "chat/completions";

    LOG_DEBUG("LLMConnector", "Connecting to: %s", url.c_str());

    if (!http.begin(client, url)) {
        snprintf(m_last_error, sizeof(m_last_error), "HTTP begin failed");
        LOG_ERROR("LLMConnector", "%s", m_last_error);
        m_state = LLMState::ERROR;
        return false;
    }

    // 设置请求头
    http.addHeader("Content-Type", "application/json");

    String auth_header = "Bearer ";
    auth_header += config.llm.api_key;
    http.addHeader("Authorization", auth_header);

    // 设置超时
    http.setTimeout(HTTP_TIMEOUT_MS);

    // 构建请求体（不使用历史）
    String request_json = buildChatRequest(user_message, false);
    LOG_DEBUG("LLMConnector", "Request size: %d bytes", request_json.length());

    // 发送POST请求
    m_state = LLMState::SENDING;
    int http_code = http.POST(request_json);

    if (http_code != HTTP_CODE_OK) {
        snprintf(m_last_error, sizeof(m_last_error), "HTTP error: %d", http_code);
        LOG_ERROR("LLMConnector", "%s", m_last_error);
        http.end();
        m_state = LLMState::ERROR;
        return false;
    }

    // 接收响应
    m_state = LLMState::RECEIVING;
    String response_json = http.getString();
    http.end();

    LOG_DEBUG("LLMConnector", "Response size: %d bytes", response_json.length());

    // 解析响应
    if (!parseChatResponse(response_json, response_buffer, buffer_size)) {
        m_state = LLMState::ERROR;
        return false;
    }

    LOG_INFO("LLMConnector", "Chat response: %s", response_buffer);
    m_state = LLMState::SUCCESS;
    return true;
}

bool LLMConnector::chatWithOptions(const char* user_message,
                                   char* response_buffer, size_t response_size,
                                   char options[][64], uint8_t* option_count) {
    // 检查WiFi连接
    if (!WiFiManager::instance().isConnected()) {
        snprintf(m_last_error, sizeof(m_last_error), "WiFi not connected");
        LOG_ERROR("LLMConnector", "%s", m_last_error);
        return false;
    }

    // 检查配置
    hydro_config_t& config = ConfigManager::instance().getConfig();
    if (strlen(config.llm.base_url) == 0 || strlen(config.llm.api_key) == 0) {
        snprintf(m_last_error, sizeof(m_last_error), "LLM not configured");
        LOG_ERROR("LLMConnector", "%s", m_last_error);
        return false;
    }

    LOG_INFO("LLMConnector", "Sending chat request with history: %s", user_message);
    m_state = LLMState::CONNECTING;

    // 创建HTTPS客户端
    WiFiClientSecure client;
    HTTPClient http;

    // 跳过证书验证（演示模式）
    client.setInsecure();

    // 构建完整URL
    String url = String(config.llm.base_url);
    if (!url.endsWith("/")) {
        url += "/";
    }
    url += "chat/completions";

    LOG_DEBUG("LLMConnector", "Connecting to: %s", url.c_str());

    if (!http.begin(client, url)) {
        snprintf(m_last_error, sizeof(m_last_error), "HTTP begin failed");
        LOG_ERROR("LLMConnector", "%s", m_last_error);
        m_state = LLMState::ERROR;
        return false;
    }

    // 设置请求头
    http.addHeader("Content-Type", "application/json");

    String auth_header = "Bearer ";
    auth_header += config.llm.api_key;
    http.addHeader("Authorization", auth_header);

    // 设置超时
    http.setTimeout(HTTP_TIMEOUT_MS);

    // 构建请求体（使用历史）
    String request_json = buildChatRequest(user_message, true);
    LOG_DEBUG("LLMConnector", "Request size: %d bytes", request_json.length());

    // 发送POST请求
    m_state = LLMState::SENDING;
    int http_code = http.POST(request_json);

    if (http_code != HTTP_CODE_OK) {
        snprintf(m_last_error, sizeof(m_last_error), "HTTP error: %d", http_code);
        LOG_ERROR("LLMConnector", "%s", m_last_error);
        http.end();
        m_state = LLMState::ERROR;
        return false;
    }

    // 接收响应
    m_state = LLMState::RECEIVING;
    String response_json = http.getString();
    http.end();

    LOG_DEBUG("LLMConnector", "Response size: %d bytes", response_json.length());

    // 解析结构化响应
    if (!parseStructuredResponse(response_json, response_buffer, response_size, options, option_count)) {
        m_state = LLMState::ERROR;
        return false;
    }

    LOG_INFO("LLMConnector", "Chat response with %d options: %s", *option_count, response_buffer);
    m_state = LLMState::SUCCESS;

    // 保存到历史
    const char* option_ptrs[3];
    for (uint8_t i = 0; i < *option_count; i++) {
        option_ptrs[i] = options[i];
    }
    HistoryManager::instance().addTurn(user_message, response_buffer, option_ptrs, *option_count);

    return true;
}

LLMState LLMConnector::getState() {
    return m_state;
}

const char* LLMConnector::getLastError() {
    return m_last_error;
}

String LLMConnector::getStatusJson() {
    JsonDocument doc;

    // 状态
    switch (m_state) {
        case LLMState::IDLE:
            doc["status"] = "IDLE";
            break;
        case LLMState::CONNECTING:
            doc["status"] = "CONNECTING";
            break;
        case LLMState::SENDING:
            doc["status"] = "SENDING";
            break;
        case LLMState::RECEIVING:
            doc["status"] = "RECEIVING";
            break;
        case LLMState::SUCCESS:
            doc["status"] = "SUCCESS";
            break;
        case LLMState::ERROR:
            doc["status"] = "ERROR";
            doc["error"] = m_last_error;
            break;
    }

    // 配置信息
    hydro_config_t& config = ConfigManager::instance().getConfig();
    doc["configured"] = (strlen(config.llm.base_url) > 0 && strlen(config.llm.api_key) > 0);
    doc["model"] = config.llm.model;

    if (strlen(config.llm.base_url) > 0) {
        doc["base_url"] = config.llm.base_url;
    }

    // 历史信息
    doc["history_count"] = HistoryManager::instance().getHistoryCount();

    String output;
    serializeJson(doc, output);
    return output;
}
