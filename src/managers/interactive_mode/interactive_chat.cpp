/**
 * @file interactive_chat.cpp
 * @brief Chat interface implementation
 */

#include "interactive_chat.h"
#include "interactive_common.h"
#include "../../services/llm_connector.h"
#include "../../services/history_manager.h"
#include "../../ui/ui_manager.h"
#include <string.h>

// --- 常量定义 ---
static const char* WELCOME_MESSAGE = "Hello! I'm your plant companion.";

static const char* DEFAULT_OPTIONS[] = {
    "Check my status",
    "Do you need water?",
    "Plant care tips"
};

static const char* CLEAR_HISTORY_OPTION = "Clear history";
static const uint8_t DEFAULT_OPTION_COUNT = 3;
static const unsigned long ERROR_DISPLAY_MS = 2000;

// --- 状态变量 ---
static uint8_t selected_option_index = 0;
static char current_plant_message[256] = "";
static char current_options[4][64];  // 最多4个（3个常规+1个"清空历史"）
static uint8_t current_option_count = 0;
static bool logged = false;
static bool is_loading = false;
static char error_message[128] = "";
static unsigned long error_display_start = 0;

// --- 辅助函数：设置默认选项 ---
static void set_default_options(void) {
    for (uint8_t i = 0; i < DEFAULT_OPTION_COUNT; i++) {
        strncpy(current_options[i], DEFAULT_OPTIONS[i], sizeof(current_options[i]) - 1);
        current_options[i][sizeof(current_options[i]) - 1] = '\0';
    }
    strncpy(current_options[DEFAULT_OPTION_COUNT], CLEAR_HISTORY_OPTION,
            sizeof(current_options[DEFAULT_OPTION_COUNT]) - 1);
    current_options[DEFAULT_OPTION_COUNT][sizeof(current_options[DEFAULT_OPTION_COUNT]) - 1] = '\0';
    current_option_count = DEFAULT_OPTION_COUNT + 1;
}

void interactive_chat_enter(void) {
    LOG_DEBUG("Interactive", "Entered STATE_CHAT");

    // 清空对话历史
    HistoryManager::instance().clear();
    LOG_INFO("Interactive", "Chat history cleared");

    // 设置欢迎语
    strncpy(current_plant_message, WELCOME_MESSAGE, sizeof(current_plant_message) - 1);
    current_plant_message[sizeof(current_plant_message) - 1] = '\0';

    // 设置默认选项（3个默认 + 1个"清空对话历史"）
    set_default_options();

    // 重置状态
    selected_option_index = 0;
    logged = false;
    is_loading = false;
    error_message[0] = '\0';

    LOG_INFO("Interactive", "Chat interface initialized with welcome message");
}

interactive_state_t interactive_chat_handle(interactive_state_t* state) {
    // 1. 检查是否在显示错误（2秒后返回主菜单）
    if (error_message[0] != '\0') {
#ifndef TEST_MODE
        // 生产环境: 调用UI显示错误
        ui_manager_show_error(error_message);
#else
        // TEST_MODE: LOG已经在设置error_message时输出
#endif
        if (millis() - error_display_start >= ERROR_DISPLAY_MS) {
            LOG_INFO("Interactive", "Error timeout, returning to main menu");
            interactive_switch_state(STATE_MAIN_MENU, state);
        }
        // 显示错误期间不处理输入
        return *state;
    }

    // 2. 如果正在加载，跳过输入处理（等待LLM响应）
    if (is_loading) {
#ifndef TEST_MODE
        // 生产环境: 显示加载动画
        ui_manager_show_loading("Thinking...");
#else
        // TEST_MODE: LOG已经输出"Calling LLM..."
#endif
        return *state;
    }

    // 3. 显示聊天界面
    if (!logged) {
#ifdef TEST_MODE
        // TEST_MODE: 串口LOG输出
        LOG_INFO("Interactive", "=== CHAT INTERFACE ===");
        LOG_INFO("Interactive", "Plant says: %s", current_plant_message);
        for (uint8_t i = 0; i < current_option_count; i++) {
            const char* prefix = (i == selected_option_index) ? ">" : " ";
            LOG_INFO("Interactive", "%s %s", prefix, current_options[i]);
        }
        LOG_INFO("Interactive", "======================");
#else
        // 生产环境: 调用UI显示聊天界面
        const char* options_ptr[4];
        for (uint8_t i = 0; i < current_option_count; i++) {
            options_ptr[i] = current_options[i];
        }
        ui_manager_show_chat_screen(current_plant_message, options_ptr,
                                     current_option_count, selected_option_index);
#endif
        logged = true;
    }

    // 4. 处理编码器旋转（选择选项） - 批量消费所有增量
    int total_delta = 0;
    int8_t delta;
    while ((delta = input_manager_get_encoder_delta()) != 0) {
        total_delta += delta;
    }
    if (total_delta != 0) {
        selected_option_index = (selected_option_index + total_delta + current_option_count * 10)
                                % current_option_count;
        logged = false;  // 触发重新LOG
        LOG_DEBUG("Interactive", "Option selected: %d (delta=%d)", selected_option_index, total_delta);
    }

    // 长按 - 触发全刷
    if (input_manager_get_button_long_pressed()) {
        LOG_INFO("Interactive", "Long press detected - triggering full refresh");
#ifndef TEST_MODE
        ui_manager_trigger_full_refresh();
#endif
    }

    // 5. 处理单击确认
    if (input_manager_get_button_clicked()) {
        const char* selected_text = current_options[selected_option_index];

        // 5.1 检查是否是"清空对话历史"
        if (strcmp(selected_text, CLEAR_HISTORY_OPTION) == 0) {
            LOG_INFO("Interactive", "User selected: Clear history");
            HistoryManager::instance().clear();

            // 重置为欢迎语和默认选项
            strncpy(current_plant_message, WELCOME_MESSAGE, sizeof(current_plant_message) - 1);
            current_plant_message[sizeof(current_plant_message) - 1] = '\0';
            set_default_options();
            selected_option_index = 0;
            logged = false;

            LOG_INFO("Interactive", "Chat history cleared, reset to welcome");
            return *state;
        }

        // 5.2 否则，调用LLM
        LOG_INFO("Interactive", "User selected: %s", selected_text);
        LOG_INFO("Interactive", "Calling LLM...");
        is_loading = true;
        logged = false;

        // 准备响应缓冲区
        char response_buffer[256];
        char options_buffer[3][64];
        uint8_t option_count = 0;

        // 调用LLM（同步阻塞）
        LLMConnector& llm = LLMConnector::instance();
        bool success = llm.chatWithOptions(selected_text,
                                            response_buffer, sizeof(response_buffer),
                                            options_buffer, &option_count);

        is_loading = false;

        if (!success) {
            // 显示错误
            const char* error = llm.getLastError();
            snprintf(error_message, sizeof(error_message), "Error: %s", error);
            error_display_start = millis();
            LOG_ERROR("Interactive", "LLM chat failed: %s", error);
            logged = false;
            return *state;
        }

        // 成功：更新消息和选项
        strncpy(current_plant_message, response_buffer, sizeof(current_plant_message) - 1);
        current_plant_message[sizeof(current_plant_message) - 1] = '\0';

        // 复制LLM返回的选项
        for (uint8_t i = 0; i < option_count && i < 3; i++) {
            strncpy(current_options[i], options_buffer[i], sizeof(current_options[i]) - 1);
            current_options[i][sizeof(current_options[i]) - 1] = '\0';
        }

        // Fallback：如果LLM没返回选项，使用默认选项
        if (option_count == 0) {
            for (uint8_t i = 0; i < DEFAULT_OPTION_COUNT; i++) {
                strncpy(current_options[i], DEFAULT_OPTIONS[i], sizeof(current_options[i]) - 1);
                current_options[i][sizeof(current_options[i]) - 1] = '\0';
            }
            option_count = DEFAULT_OPTION_COUNT;
            LOG_WARN("Interactive", "LLM returned no options, using defaults");
        }

        // 总是添加"清空对话历史"选项
        strncpy(current_options[option_count], CLEAR_HISTORY_OPTION,
                sizeof(current_options[option_count]) - 1);
        current_options[option_count][sizeof(current_options[option_count]) - 1] = '\0';
        current_option_count = option_count + 1;

        // 注意：历史保存已由LLMConnector::chatWithOptions()自动完成

        selected_option_index = 0;
        logged = false;

        LOG_INFO("Interactive", "LLM response received with %d options", option_count);
    }

    // 6. 处理双击返回
    if (input_manager_get_button_double_clicked()) {
        LOG_INFO("Interactive", "Returning to main menu from CHAT");
        interactive_switch_state(STATE_MAIN_MENU, state);
    }

    return *state;
}
