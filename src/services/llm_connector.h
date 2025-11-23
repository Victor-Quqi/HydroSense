/**
 * @file llm_connector.h
 * @brief LLM连接器 - 通过HTTPS调用LLM API实现植物对话
 */

#ifndef LLM_CONNECTOR_H
#define LLM_CONNECTOR_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

/**
 * @brief LLM请求状态
 */
enum class LLMState {
    IDLE,           // 空闲
    CONNECTING,     // 连接中
    SENDING,        // 发送请求中
    RECEIVING,      // 接收响应中
    SUCCESS,        // 成功
    ERROR           // 错误
};

/**
 * @brief LLM连接器单例类
 *
 * 功能特性:
 * - HTTPS安全连接（演示模式跳过证书验证）
 * - JSON请求构建和响应解析
 * - 硬编码植物上下文（快速演示版）
 * - 超时处理
 */
class LLMConnector {
public:
    /**
     * @brief 获取单例实例
     */
    static LLMConnector& instance();

    /**
     * @brief 初始化LLM连接器
     * @return true 成功, false 失败
     */
    bool init();

    /**
     * @brief 发送聊天请求
     * @param user_message 用户消息
     * @param response_buffer 响应缓冲区
     * @param buffer_size 缓冲区大小
     * @return true 成功, false 失败
     */
    bool chat(const char* user_message, char* response_buffer, size_t buffer_size);

    /**
     * @brief 发送聊天请求并获取动态选项（使用对话历史）
     * @param user_message 用户消息
     * @param response_buffer 响应缓冲区
     * @param response_size 响应缓冲区大小
     * @param options 选项数组（最多3个）
     * @param option_count 选项数量指针（输出）
     * @return true 成功, false 失败
     */
    bool chatWithOptions(const char* user_message,
                         char* response_buffer, size_t response_size,
                         char options[][64], uint8_t* option_count);

    /**
     * @brief 获取当前状态
     * @return 状态枚举
     */
    LLMState getState();

    /**
     * @brief 获取最后错误信息
     * @return 错误信息字符串
     */
    const char* getLastError();

    /**
     * @brief 返回状态JSON字符串
     * @return JSON格式的状态信息
     */
    String getStatusJson();

private:
    LLMConnector();
    LLMConnector(const LLMConnector&) = delete;
    LLMConnector& operator=(const LLMConnector&) = delete;

    /**
     * @brief 构建聊天请求JSON
     * @param user_message 用户消息
     * @param use_history 是否包含对话历史
     * @return JSON字符串
     */
    String buildChatRequest(const char* user_message, bool use_history = false);

    /**
     * @brief 解析聊天响应JSON
     * @param response_json 响应JSON字符串
     * @param output_buffer 输出缓冲区
     * @param buffer_size 缓冲区大小
     * @return true 成功, false 失败
     */
    bool parseChatResponse(const String& response_json, char* output_buffer, size_t buffer_size);

    /**
     * @brief 解析带选项的聊天响应JSON
     * @param response_json 响应JSON字符串
     * @param response_buffer 响应缓冲区
     * @param response_size 响应缓冲区大小
     * @param options 选项数组
     * @param option_count 选项数量指针
     * @return true 成功, false 失败
     */
    bool parseStructuredResponse(const String& response_json,
                                  char* response_buffer, size_t response_size,
                                  char options[][64], uint8_t* option_count);

    /**
     * @brief 获取硬编码的系统提示词
     * @param with_options 是否要求生成选项
     * @return 系统提示词
     */
    const char* getSystemPrompt(bool with_options = false);

    // 状态变量
    LLMState m_state;
    char m_last_error[128];

    // 常量
    static const unsigned long HTTP_TIMEOUT_MS = 30000;
    static LLMConnector* s_instance;
};

#endif // LLM_CONNECTOR_H
