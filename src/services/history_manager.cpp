/**
 * @file history_manager.cpp
 * @brief 对话历史管理器实现
 */

#include "history_manager.h"
#include "time_manager.h"
#include "../managers/log_manager.h"
#include <SPIFFS.h>

const char* HistoryManager::HISTORY_FILE_PATH = "/conversation.json";
HistoryManager* HistoryManager::s_instance = nullptr;

HistoryManager& HistoryManager::instance() {
    if (s_instance == nullptr) {
        s_instance = new HistoryManager();
    }
    return *s_instance;
}

HistoryManager::HistoryManager() {
    m_history.reserve(MAX_HISTORY);
}

bool HistoryManager::init() {
    LOG_INFO("HistoryManager", "Initializing history manager...");

    // 初始化SPIFFS
    if (!SPIFFS.begin(true)) {
        LOG_ERROR("HistoryManager", "SPIFFS mount failed");
        return false;
    }

    // 获取文件系统信息
    size_t total_bytes = SPIFFS.totalBytes();
    size_t used_bytes = SPIFFS.usedBytes();
    LOG_INFO("HistoryManager", "SPIFFS: %u/%u bytes used", used_bytes, total_bytes);

    // 尝试加载历史
    load();

    LOG_INFO("HistoryManager", "History manager initialized (%d turns loaded)", m_history.size());
    return true;
}

void HistoryManager::addTurn(const char* user_msg, const char* plant_msg,
                             const char* options[], uint8_t option_count) {
    ConversationTurn turn;

    // 复制消息
    strncpy(turn.user_msg, user_msg, sizeof(turn.user_msg) - 1);
    turn.user_msg[sizeof(turn.user_msg) - 1] = '\0';

    strncpy(turn.plant_msg, plant_msg, sizeof(turn.plant_msg) - 1);
    turn.plant_msg[sizeof(turn.plant_msg) - 1] = '\0';

    // 复制选项
    turn.option_count = option_count > 3 ? 3 : option_count;
    for (uint8_t i = 0; i < turn.option_count; i++) {
        strncpy(turn.options[i], options[i], sizeof(turn.options[i]) - 1);
        turn.options[i][sizeof(turn.options[i]) - 1] = '\0';
    }

    // 设置时间戳
    turn.timestamp = TimeManager::instance().getTimestamp();

    // 添加到历史
    m_history.push_back(turn);

    // 保持最多MAX_HISTORY轮
    if (m_history.size() > MAX_HISTORY) {
        m_history.erase(m_history.begin());
    }

    LOG_INFO("HistoryManager", "Added turn (total: %d)", m_history.size());

    // 自动保存
    save();
}

const std::vector<ConversationTurn>& HistoryManager::getHistory() {
    return m_history;
}

size_t HistoryManager::getHistoryCount() {
    return m_history.size();
}

void HistoryManager::clear() {
    m_history.clear();
    LOG_INFO("HistoryManager", "History cleared");

    // 删除文件
    if (SPIFFS.exists(HISTORY_FILE_PATH)) {
        SPIFFS.remove(HISTORY_FILE_PATH);
        LOG_INFO("HistoryManager", "History file deleted");
    }
}

bool HistoryManager::load() {
    if (!SPIFFS.exists(HISTORY_FILE_PATH)) {
        LOG_INFO("HistoryManager", "No history file found");
        return true;
    }

    File file = SPIFFS.open(HISTORY_FILE_PATH, "r");
    if (!file) {
        LOG_ERROR("HistoryManager", "Failed to open history file for reading");
        return false;
    }

    // 读取JSON
    String json_str = file.readString();
    file.close();

    if (json_str.length() == 0) {
        LOG_WARN("HistoryManager", "History file is empty");
        return true;
    }

    // 解析JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json_str);

    if (error) {
        LOG_ERROR("HistoryManager", "JSON parse error: %s", error.c_str());
        return false;
    }

    // 加载对话历史
    m_history.clear();
    JsonArray history_array = doc["history"].as<JsonArray>();

    for (JsonObject turn_obj : history_array) {
        ConversationTurn turn;

        strncpy(turn.user_msg, turn_obj["user"] | "", sizeof(turn.user_msg) - 1);
        turn.user_msg[sizeof(turn.user_msg) - 1] = '\0';

        strncpy(turn.plant_msg, turn_obj["plant"] | "", sizeof(turn.plant_msg) - 1);
        turn.plant_msg[sizeof(turn.plant_msg) - 1] = '\0';

        JsonArray options_array = turn_obj["options"].as<JsonArray>();
        turn.option_count = 0;
        for (JsonVariant option : options_array) {
            if (turn.option_count >= 3) break;
            strncpy(turn.options[turn.option_count], option.as<const char*>(), sizeof(turn.options[0]) - 1);
            turn.options[turn.option_count][sizeof(turn.options[0]) - 1] = '\0';
            turn.option_count++;
        }

        turn.timestamp = turn_obj["timestamp"] | 0;

        m_history.push_back(turn);
    }

    LOG_INFO("HistoryManager", "Loaded %d turns from SPIFFS", m_history.size());
    return true;
}

bool HistoryManager::save() {
    // 构建JSON
    JsonDocument doc;
    JsonArray history_array = doc["history"].to<JsonArray>();

    for (const auto& turn : m_history) {
        JsonObject turn_obj = history_array.add<JsonObject>();
        turn_obj["user"] = turn.user_msg;
        turn_obj["plant"] = turn.plant_msg;

        JsonArray options_array = turn_obj["options"].to<JsonArray>();
        for (uint8_t i = 0; i < turn.option_count; i++) {
            options_array.add(turn.options[i]);
        }

        turn_obj["timestamp"] = turn.timestamp;
    }

    // 序列化到字符串
    String json_str;
    serializeJson(doc, json_str);

    // 写入文件
    File file = SPIFFS.open(HISTORY_FILE_PATH, "w");
    if (!file) {
        LOG_ERROR("HistoryManager", "Failed to open history file for writing");
        return false;
    }

    size_t written = file.print(json_str);
    file.close();

    if (written == 0) {
        LOG_ERROR("HistoryManager", "Failed to write to history file");
        return false;
    }

    LOG_INFO("HistoryManager", "Saved %d turns to SPIFFS (%u bytes)", m_history.size(), written);
    return true;
}

void HistoryManager::buildContextMessages(JsonArray& messages) {
    for (const auto& turn : m_history) {
        // 添加用户消息
        JsonObject user_msg = messages.add<JsonObject>();
        user_msg["role"] = "user";
        user_msg["content"] = turn.user_msg;

        // 添加助手回复
        JsonObject assistant_msg = messages.add<JsonObject>();
        assistant_msg["role"] = "assistant";
        assistant_msg["content"] = turn.plant_msg;
    }
}
