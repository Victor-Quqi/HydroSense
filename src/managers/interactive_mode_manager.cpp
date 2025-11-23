/**
 * @file interactive_mode_manager.cpp
 * @brief Interactive mode manager implementation
 */

#include "interactive_mode_manager.h"
#include "input_manager.h"
#include "log_manager.h"
#include <Arduino.h>

// --- 状态机变量 ---
static bool is_initialized = false;
static interactive_state_t current_state = STATE_MAIN_MENU;

// --- 主菜单定义 ---
#define MAIN_MENU_ITEM_COUNT 4
static const char* main_menu_items[MAIN_MENU_ITEM_COUNT] = {
    "系统状态",
    "系统设置",
    "立即浇水",
    "聊天"
};
static int main_menu_index = 0; // 当前选中的菜单项索引

// --- 状态处理函数声明 ---
static void handle_main_menu_state();
static void handle_status_state();
static void handle_settings_state();
static void handle_setting_edit_state();
static void handle_watering_state();
static void handle_chat_state();

// --- API实现 ---

interactive_mode_result_t interactive_mode_manager_init(void) {
    LOG_INFO("Interactive", "Initializing interactive mode manager");

    is_initialized = true;
    current_state = STATE_MAIN_MENU;
    main_menu_index = 0;

    return INTERACTIVE_MODE_OK;
}

interactive_mode_result_t interactive_mode_manager_enter(void) {
    if (!is_initialized) {
        LOG_ERROR("Interactive", "Manager not initialized");
        return INTERACTIVE_MODE_ERR_NOT_INITIALIZED;
    }

    LOG_INFO("Interactive", "Entering interactive mode");

    // 清除累积的输入事件，避免模式切换时的干扰
    input_manager_clear_events();

    // 重置状态为主菜单
    current_state = STATE_MAIN_MENU;
    main_menu_index = 0;

    // TODO: 显示主菜单UI
    LOG_DEBUG("Interactive", "Displaying main menu");

    return INTERACTIVE_MODE_OK;
}

interactive_mode_result_t interactive_mode_manager_exit(void) {
    if (!is_initialized) {
        return INTERACTIVE_MODE_ERR_NOT_INITIALIZED;
    }

    LOG_INFO("Interactive", "Exiting interactive mode");

    // TODO: 清除屏幕，保存状态

    return INTERACTIVE_MODE_OK;
}

interactive_mode_result_t interactive_mode_manager_loop(void) {
    if (!is_initialized) {
        return INTERACTIVE_MODE_ERR_NOT_INITIALIZED;
    }

    // 基于当前状态的switch-case处理
    switch (current_state) {
        case STATE_MAIN_MENU:
            handle_main_menu_state();
            break;

        case STATE_STATUS:
            handle_status_state();
            break;

        case STATE_SETTINGS:
            handle_settings_state();
            break;

        case STATE_SETTING_EDIT:
            handle_setting_edit_state();
            break;

        case STATE_WATERING:
            handle_watering_state();
            break;

        case STATE_CHAT:
            handle_chat_state();
            break;

        default:
            LOG_ERROR("Interactive", "Unknown state: %d", current_state);
            current_state = STATE_MAIN_MENU;
            break;
    }

    return INTERACTIVE_MODE_OK;
}

// --- 状态处理函数（占位符，后续实现） ---

static void handle_main_menu_state() {
    // TODO: P1.2 实现主菜单逻辑
    // - 读取编码器增量，调整menu_index
    // - 检测单击，进入对应子功能
    // - 更新UI显示
}

static void handle_status_state() {
    // TODO: P1.3 实现查看状态逻辑
    // - 显示系统状态信息
    // - 检测双击返回主菜单
}

static void handle_settings_state() {
    // TODO: P1.4 实现设置菜单逻辑
    // - 显示设置项列表
    // - 单击进入编辑，双击返回
}

static void handle_setting_edit_state() {
    // TODO: P1.4 实现设置编辑逻辑
    // - 调整设置值
    // - 单击保存，双击取消
}

static void handle_watering_state() {
    // TODO: P1.5 实现浇水逻辑
    // - 显示浇水进度
    // - 完成后返回主菜单
}

static void handle_chat_state() {
    // TODO: P1.6 实现聊天界面逻辑
    // - 显示LLM回复和选项
    // - 单击选择，双击返回
}
