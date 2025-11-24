/**
 * @file interactive_mode_manager.cpp
 * @brief Interactive mode main coordinator (refactored)
 * @details Delegates state handling to specialized modules
 */

#include "interactive_mode_manager.h"
#include "interactive_mode/interactive_common.h"
#include "interactive_mode/interactive_main_menu.h"
#include "interactive_mode/interactive_status.h"
#include "interactive_mode/interactive_settings.h"
#include "interactive_mode/interactive_watering.h"
#include "interactive_mode/interactive_chat.h"
#include "../ui/ui_manager.h"
#include "../ui/display_manager.h"
#include "power_manager.h"
#include "../data/timing_constants.h"
#include <Arduino.h>

// Manager state
static bool is_initialized = false;
static interactive_state_t current_state = STATE_MAIN_MENU;
static bool exit_requested = false;

// Global flag for initial full refresh (defined here, declared in interactive_common.h)
bool g_interactive_needs_initial_refresh = false;

interactive_mode_result_t interactive_mode_manager_init(void) {
    LOG_INFO("Interactive", "Initializing interactive mode manager");
    is_initialized = true;
    current_state = STATE_MAIN_MENU;
    return INTERACTIVE_MODE_OK;
}

interactive_mode_result_t interactive_mode_manager_enter(void) {
    if (!is_initialized) {
        LOG_ERROR("Interactive", "Manager not initialized");
        return INTERACTIVE_MODE_ERR_NOT_INITIALIZED;
    }

    LOG_INFO("Interactive", "Entering interactive mode");

#ifndef TEST_MODE
    // 1. Enable screen power before any display operations
    power_result_t power_result = power_screen_enable(true);
    if (power_result != POWER_OK) {
        LOG_ERROR("Interactive", "Failed to enable screen power (error %d)", power_result);
        return INTERACTIVE_MODE_ERR_NOT_INITIALIZED;
    }
    LOG_DEBUG("Interactive", "Screen power enabled, waiting for stabilization");

    // 2. Wait for power to stabilize
    delay(POWER_STABILIZATION_DELAY_MS);

    // 3. Initialize display manager (will check s_initialized internally)
    display_result_t display_result = display_manager_init();
    if (display_result != DISPLAY_OK) {
        LOG_ERROR("Interactive", "Failed to initialize display (error %d)", display_result);
        return INTERACTIVE_MODE_ERR_NOT_INITIALIZED;
    }
    LOG_DEBUG("Interactive", "Display manager ready");
#else
    LOG_INFO("Interactive", "TEST_MODE: Skipping display initialization");
#endif

    // 4. Clear input events and setup state
    input_manager_clear_events();
    current_state = STATE_MAIN_MENU;
    exit_requested = false;

#ifndef TEST_MODE
    // 5. Set flag for initial full refresh
    g_interactive_needs_initial_refresh = true;

    // 移除立即全刷：避免在屏幕可能未稳定时操作
    // 全刷将在第一次显示主菜单时触发（interactive_main_menu_handle）
#endif

    // Initialize first state
    interactive_main_menu_enter();

    return INTERACTIVE_MODE_OK;
}

interactive_mode_result_t interactive_mode_manager_exit(void) {
    if (!is_initialized) {
        return INTERACTIVE_MODE_ERR_NOT_INITIALIZED;
    }

    LOG_INFO("Interactive", "Exiting interactive mode");
    return INTERACTIVE_MODE_OK;
}

interactive_mode_result_t interactive_mode_manager_loop(void) {
    if (!is_initialized) {
        return INTERACTIVE_MODE_ERR_NOT_INITIALIZED;
    }

    interactive_state_t prev_state = current_state;

    // Route to state handler
    switch (current_state) {
        case STATE_MAIN_MENU:
            interactive_main_menu_handle(&current_state, &exit_requested);
            break;

        case STATE_STATUS:
            interactive_status_handle(&current_state);
            break;

        case STATE_SETTINGS:
            interactive_settings_handle(&current_state);
            break;

        case STATE_SETTING_EDIT:
            interactive_setting_edit_handle(&current_state);
            break;

        case STATE_WATERING:
            interactive_watering_handle(&current_state);
            break;

        case STATE_CHAT:
            interactive_chat_handle(&current_state);
            break;

        default:
            LOG_ERROR("Interactive", "Unknown state: %d", current_state);
            current_state = STATE_MAIN_MENU;
            break;
    }

    // Call enter() when state changes
    if (current_state != prev_state) {
        switch (current_state) {
            case STATE_MAIN_MENU:
                interactive_main_menu_enter();
                break;
            case STATE_STATUS:
                interactive_status_enter();
                break;
            case STATE_SETTINGS:
                interactive_settings_enter();
                break;
            case STATE_SETTING_EDIT:
                interactive_setting_edit_enter();
                break;
            case STATE_WATERING:
                interactive_watering_enter();
                break;
            case STATE_CHAT:
                interactive_chat_enter();
                break;
            default:
                break;
        }
    }

    return INTERACTIVE_MODE_OK;
}

bool interactive_mode_manager_should_exit(void) {
    return exit_requested;
}
