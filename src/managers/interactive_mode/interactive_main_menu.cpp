/**
 * @file interactive_main_menu.cpp
 * @brief Main menu handler implementation
 */

#include "interactive_main_menu.h"
#include "interactive_common.h"

#define MAIN_MENU_ITEM_COUNT 4
static const char* menu_items[MAIN_MENU_ITEM_COUNT] = {
    "系统状态",
    "系统设置",
    "立即浇水",
    "聊天"
};

static int menu_index = 0;
static bool menu_logged = false;

void interactive_main_menu_enter(void) {
    menu_index = 0;
    menu_logged = false;
    LOG_DEBUG("Interactive", "Entered MAIN_MENU state");
}

interactive_state_t interactive_main_menu_handle(interactive_state_t* state,
                                                  bool* exit_flag) {
    // Log menu items once
    if (!menu_logged) {
        LOG_INFO("Interactive", "=== Main Menu ===");
        for (int i = 0; i < MAIN_MENU_ITEM_COUNT; i++) {
            if (i == menu_index) {
                LOG_INFO("Interactive", "> %d. %s (selected)", i, menu_items[i]);
            } else {
                LOG_INFO("Interactive", "  %d. %s", i, menu_items[i]);
            }
        }
        menu_logged = true;
    }

    // Handle encoder rotation
    int8_t delta = input_manager_get_encoder_delta();
    if (delta != 0) {
        menu_index = (menu_index + delta + MAIN_MENU_ITEM_COUNT) % MAIN_MENU_ITEM_COUNT;
        LOG_INFO("Interactive", "Selected: %s", menu_items[menu_index]);
        menu_logged = false;  // Refresh display
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
