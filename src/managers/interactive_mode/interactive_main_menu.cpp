/**
 * @file interactive_main_menu.cpp
 * @brief Main menu handler implementation
 */

#include "interactive_main_menu.h"
#include "interactive_common.h"
#include "../../ui/ui_manager.h"

#define MAIN_MENU_ITEM_COUNT 4
static const char* menu_items[MAIN_MENU_ITEM_COUNT] = {
    "System Status",
    "Settings",
    "Water Now",
    "Chat"
};

static int menu_index = 0;
static bool menu_logged = false;

void interactive_main_menu_enter(void) {
    menu_index = 0;
    menu_logged = false;
    // Note: Do NOT reset g_interactive_needs_initial_refresh here
    // It's set by interactive_mode_manager_enter() only on mode switch
    LOG_DEBUG("Interactive", "Entered MAIN_MENU state");
}

interactive_state_t interactive_main_menu_handle(interactive_state_t* state,
                                                  bool* exit_flag) {
    // Display menu
    if (!menu_logged) {
#ifdef TEST_MODE
        // TEST_MODE: 串口LOG输出
        LOG_INFO("Interactive", "=== Main Menu ===");
        for (int i = 0; i < MAIN_MENU_ITEM_COUNT; i++) {
            if (i == menu_index) {
                LOG_INFO("Interactive", "> %d. %s (selected)", i, menu_items[i]);
            } else {
                LOG_INFO("Interactive", "  %d. %s", i, menu_items[i]);
            }
        }
#else
        // 生产环境: 调用UI显示
        ui_manager_show_menu("Main Menu", menu_items, MAIN_MENU_ITEM_COUNT,
                             menu_index, NULL);
#endif
        menu_logged = true;

        // 仅在模式切换后的首次显示时强制全刷，清除残影并确保屏幕已稳定
        if (g_interactive_needs_initial_refresh) {
            LOG_INFO("Interactive", "Initial display - triggering full refresh");
            ui_manager_trigger_full_refresh();
            g_interactive_needs_initial_refresh = false;
        }
    }

    // Handle encoder rotation - batch consume all deltas
    int total_delta = 0;
    int8_t delta;
    while ((delta = input_manager_get_encoder_delta()) != 0) {
        total_delta += delta;
    }
    if (total_delta != 0) {
        menu_index = (menu_index + total_delta + MAIN_MENU_ITEM_COUNT * 10) % MAIN_MENU_ITEM_COUNT;
        LOG_INFO("Interactive", "Selected: %s (delta=%d)", menu_items[menu_index], total_delta);
        menu_logged = false;  // Refresh display once
    }

    // Handle long press - trigger full refresh
    if (input_manager_get_button_long_pressed()) {
        LOG_INFO("Interactive", "Long press detected - triggering full refresh");
        ui_manager_trigger_full_refresh();
    }

    // Handle single click - enter submenu
    if (input_manager_get_button_clicked()) {
        LOG_INFO("Interactive", "Confirmed: %s", menu_items[menu_index]);
        menu_logged = false;  // Reset for next time

        switch (menu_index) {
            case 0: // 系统状态
                interactive_switch_state(STATE_STATUS, state);
                LOG_DEBUG("Interactive", "Switched to STATE_STATUS");
                break;

            case 1: // 系统设置
                interactive_switch_state(STATE_SETTINGS, state);
                LOG_DEBUG("Interactive", "Switched to STATE_SETTINGS");
                break;

            case 2: // 立即浇水
                interactive_switch_state(STATE_WATERING, state);
                LOG_DEBUG("Interactive", "Switched to STATE_WATERING");
                break;

            case 3: // 聊天
                interactive_switch_state(STATE_CHAT, state);
                LOG_DEBUG("Interactive", "Switched to STATE_CHAT");
                break;

            default:
                LOG_ERROR("Interactive", "Invalid menu index: %d", menu_index);
                break;
        }
    }

    // Handle double click - exit (TEST_MODE only)
    if (input_manager_get_button_double_clicked()) {
#ifdef TEST_MODE
        LOG_INFO("Interactive", "Double click in main menu - exit requested");
        *exit_flag = true;
#else
        LOG_DEBUG("Interactive", "Double click in main menu (ignored in production)");
#endif
    }

    return *state;
}
