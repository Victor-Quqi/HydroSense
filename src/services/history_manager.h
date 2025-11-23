/**
 * @file history_manager.h
 * @brief 对话历史管理器 - 内存缓存 + SPIFFS持久化
 */

#ifndef HISTORY_MANAGER_H
#define HISTORY_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

/**
 * @brief 单轮对话结构
 */
struct ConversationTurn {
    char user_msg[128];       // 用户消息
    char plant_msg[256];      // 植物回复
    char options[3][64];      // 3个可选项
    uint8_t option_count;     // 实际选项数量
    uint32_t timestamp;       // 时间戳
};

/**
 * @brief 对话历史管理器单例类
 *
 * 功能特性:
 * - 内存缓存最近5轮对话
 * - SPIFFS持久化
 * - 自动加载/保存
 * - JSON格式存储
 */
class HistoryManager {
public:
    /**
     * @brief 获取单例实例
     */
    static HistoryManager& instance();

    /**
     * @brief 初始化历史管理器
     * @return true 成功, false 失败
     */
    bool init();

    /**
     * @brief 添加一轮对话
     * @param user_msg 用户消息
     * @param plant_msg 植物回复
     * @param options 选项数组
     * @param option_count 选项数量
     */
    void addTurn(const char* user_msg, const char* plant_msg,
                 const char* options[], uint8_t option_count);

    /**
     * @brief 获取对话历史
     * @return 对话历史列表的常量引用
     */
    const std::vector<ConversationTurn>& getHistory();

    /**
     * @brief 获取历史轮数
     * @return 历史轮数
     */
    size_t getHistoryCount();

    /**
     * @brief 清空历史
     */
    void clear();

    /**
     * @brief 从SPIFFS加载历史
     * @return true 成功, false 失败
     */
    bool load();

    /**
     * @brief 保存历史到SPIFFS
     * @return true 成功, false 失败
     */
    bool save();

    /**
     * @brief 构建完整对话上下文（用于LLM请求）
     * @param messages JsonArray引用，将追加历史消息
     */
    void buildContextMessages(JsonArray& messages);

private:
    HistoryManager();
    HistoryManager(const HistoryManager&) = delete;
    HistoryManager& operator=(const HistoryManager&) = delete;

    std::vector<ConversationTurn> m_history;

    // 常量
    static const size_t MAX_HISTORY = 5;
    static const char* HISTORY_FILE_PATH;
    static HistoryManager* s_instance;
};

#endif // HISTORY_MANAGER_H
