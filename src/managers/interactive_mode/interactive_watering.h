/**
 * @file interactive_watering.h
 * @brief Watering flow handler for interactive mode
 */

#ifndef INTERACTIVE_WATERING_H
#define INTERACTIVE_WATERING_H

#include "../interactive_mode_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

void interactive_watering_enter(void);
interactive_state_t interactive_watering_handle(interactive_state_t* state);

#ifdef __cplusplus
}
#endif

#endif // INTERACTIVE_WATERING_H
