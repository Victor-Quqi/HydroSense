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
        // 要求生成选项的系统提示词
        return "你是一个智能植物监测系统中的植物助手。"
               "你可以感知土壤湿度、环境温度等传感器数据，并根据这些数据与用户对话。"
               "请用简洁、友好的语气回答用户的问题，就像植物在说话一样。"
               "回答限制在50字以内。\n\n"
               "重要：你必须返回JSON格式的响应，格式为：\n"
               "{\"response\": \"你的回复内容\", \"options\": [\"选项1\", \"选项2\", \"选项3\"]}\n"
               "其中options是3个用户可以继续对话的选项，每个选项15字以内。";
    } else {
        // 普通对话提示词
        return "你是一个智能植物监测系统中的植物助手。"
               "你可以感知土壤湿度、环境温度等传感器数据，并根据这些数据与用户对话。"
               "请用简洁、友好的语气回答用户的问题，就像植物在说话一样。"
               "回答限制在50字以内。";
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

    // 系统提示词
    JsonObject system_msg = messages.add<JsonObject>();
    system_msg["role"] = "system";
    system_msg["content"] = getSystemPrompt(use_history);  // 使用历史时需要生成选项

    // 添加传感器数据上下文
    sensor_data_t sensor_data;
    sensor_manager_read_all(&sensor_data);
    char context[256];
    snprintf(context, sizeof(context),
             "当前传感器数据 - 土壤湿度ADC: %d, 电池电压: %.2fV, 时间: %s",
             sensor_data.soil_moisture,
             sensor_data.battery_voltage,
             TimeManager::instance().isTimeSynced() ? "已同步" : "未同步");

    JsonObject context_msg = messages.add<JsonObject>();
    context_msg["role"] = "system";
    context_msg["content"] = context;

    // 如果使用历史，添加历史对话
    if (use_history) {
        HistoryManager::instance().buildContextMessages(messages);
    }

    // 用户消息
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

    LOG_INFO("LLMConnector", "Raw content: %s", content);

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

    LOG_INFO("LLMConnector", "Connecting to: %s", url.c_str());

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
    LOG_INFO("LLMConnector", "Request size: %d bytes", request_json.length());

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

    LOG_INFO("LLMConnector", "Response size: %d bytes", response_json.length());

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

    LOG_INFO("LLMConnector", "Connecting to: %s", url.c_str());

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
    LOG_INFO("LLMConnector", "Request size: %d bytes", request_json.length());

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

    LOG_INFO("LLMConnector", "Response size: %d bytes", response_json.length());

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
