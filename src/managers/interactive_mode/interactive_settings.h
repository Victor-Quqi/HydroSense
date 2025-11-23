/**
 * @file interactive_settings.h
 * @brief Settings menu handler for interactive mode
 */

#ifndef INTERACTIVE_SETTINGS_H
#define INTERACTIVE_SETTINGS_H

#include "../interactive_mode_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

void interactive_settings_enter(void);
interactive_state_t interactive_settings_handle(interactive_state_t* state);

void interactive_setting_edit_enter(void);
interactive_state_t interactive_setting_edit_handle(interactive_state_t* state);

#ifdef __cplusplus
}
#endif

#endif // INTERACTIVE_SETTINGS_H
