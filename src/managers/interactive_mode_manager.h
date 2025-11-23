/**
 * @file interactive_mode_manager.h
 * @brief Interactive mode manager for user interaction logic
 * @details
 *   This manager encapsulates all business logic for SYSTEM_MODE_INTERACTIVE,
 *   including menu navigation, settings editing, manual watering, and chat.
 */

#ifndef INTERACTIVE_MODE_MANAGER_H
#define INTERACTIVE_MODE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Result codes for interactive mode operations
 */
typedef enum {
    INTERACTIVE_MODE_OK = 0,
    INTERACTIVE_MODE_ERR_NOT_INITIALIZED,
    INTERACTIVE_MODE_ERR_INVALID_STATE,
    INTERACTIVE_MODE_ERR_INVALID_PARAM
} interactive_mode_result_t;

/**
 * @brief Interactive mode states (internal state machine)
 */
typedef enum {
    STATE_MAIN_MENU,        // 主菜单
    STATE_STATUS,           // 查看系统状态
    STATE_SETTINGS,         // 设置菜单（列表）
    STATE_SETTING_EDIT,     // 编辑单个设置项
    STATE_WATERING,         // 浇水进度显示
    STATE_CHAT              // LLM对话
} interactive_state_t;

/**
 * @brief Initialize the interactive mode manager
 * @details Must be called once during system setup before using other functions.
 * @return INTERACTIVE_MODE_OK on success, error code otherwise
 */
interactive_mode_result_t interactive_mode_manager_init(void);

/**
 * @brief Enter INTERACTIVE mode
 * @details
 *   Called when transitioning into SYSTEM_MODE_INTERACTIVE.
 *   Performs setup: clears input events, displays main menu.
 * @return INTERACTIVE_MODE_OK on success, error code otherwise
 */
interactive_mode_result_t interactive_mode_manager_enter(void);

/**
 * @brief Interactive mode main loop handler
 * @details
 *   Must be called repeatedly in the main loop when in SYSTEM_MODE_INTERACTIVE.
 *   Implements non-blocking state machine logic:
 *   - Handles encoder input for navigation
 *   - Processes button clicks (confirm/return)
 *   - Updates UI based on current state
 * @return INTERACTIVE_MODE_OK on success, error code otherwise
 */
interactive_mode_result_t interactive_mode_manager_loop(void);

/**
 * @brief Exit INTERACTIVE mode
 * @details
 *   Called when transitioning out of SYSTEM_MODE_INTERACTIVE.
 *   Performs cleanup: clears screen, saves state if needed.
 * @return INTERACTIVE_MODE_OK on success, error code otherwise
 */
interactive_mode_result_t interactive_mode_manager_exit(void);

#ifdef __cplusplus
}
#endif

#endif // INTERACTIVE_MODE_MANAGER_H
