/**
 * @file interactive_common.h
 * @brief Interactive mode common definitions and utilities
 */

#ifndef INTERACTIVE_COMMON_H
#define INTERACTIVE_COMMON_H

#include "../interactive_mode_manager.h"
#include "../input_manager.h"
#include "../log_manager.h"
#include "../sensor_manager.h"
#include "../actuator_manager.h"
#include "../../ui/ui_manager.h"
#include "../../services/config_manager.h"
#include "../../services/wifi_manager.h"
#include "../../services/time_manager.h"
#include <Arduino.h>

// Global flag for initial full refresh when entering Interactive mode
extern bool g_interactive_needs_initial_refresh;

/**
 * @brief Switch to a new state and clear input events
 * @param new_state Target state
 * @param state_var Pointer to current state variable
 */
static inline void interactive_switch_state(interactive_state_t new_state,
                                             interactive_state_t* state_var) {
    *state_var = new_state;
    // 只清除按键事件，保留编码器队列（避免丢失旋转事件）
    input_manager_clear_button_events();
}

#endif // INTERACTIVE_COMMON_H
