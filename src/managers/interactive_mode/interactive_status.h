/**
 * @file interactive_status.h
 * @brief System status display handler for interactive mode
 */

#ifndef INTERACTIVE_STATUS_H
#define INTERACTIVE_STATUS_H

#include "../interactive_mode_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

void interactive_status_enter(void);
interactive_state_t interactive_status_handle(interactive_state_t* state);

#ifdef __cplusplus
}
#endif

#endif // INTERACTIVE_STATUS_H
