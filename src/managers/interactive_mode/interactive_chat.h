/**
 * @file interactive_chat.h
 * @brief Chat interface handler for interactive mode
 */

#ifndef INTERACTIVE_CHAT_H
#define INTERACTIVE_CHAT_H

#include "../interactive_mode_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

void interactive_chat_enter(void);
interactive_state_t interactive_chat_handle(interactive_state_t* state);

#ifdef __cplusplus
}
#endif

#endif // INTERACTIVE_CHAT_H
