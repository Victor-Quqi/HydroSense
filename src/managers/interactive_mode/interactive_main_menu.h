/**
 * @file interactive_main_menu.h
 * @brief Main menu handler for interactive mode
 */

#ifndef INTERACTIVE_MAIN_MENU_H
#define INTERACTIVE_MAIN_MENU_H

#include "../interactive_mode_manager.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize main menu state
 */
void interactive_main_menu_enter(void);

/**
 * @brief Handle main menu logic (called every loop)
 * @param state Pointer to current state variable
 * @param exit_flag Pointer to exit request flag (TEST_MODE only)
 * @return New state (may be changed)
 */
interactive_state_t interactive_main_menu_handle(interactive_state_t* state,
                                                  bool* exit_flag);

#ifdef __cplusplus
}
#endif

#endif // INTERACTIVE_MAIN_MENU_H
