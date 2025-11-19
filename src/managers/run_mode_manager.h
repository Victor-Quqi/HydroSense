/**
 * @file run_mode_manager.h
 * @brief RUN mode manager for automatic watering logic
 *
 * This manager encapsulates all business logic for SYSTEM_MODE_RUN,
 * including periodic humidity checking and automatic watering decisions.
 */

#ifndef RUN_MODE_MANAGER_H
#define RUN_MODE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Result codes for run mode operations
 */
typedef enum {
    RUN_MODE_OK = 0,
    RUN_MODE_ERR_NOT_INITIALIZED,
    RUN_MODE_ERR_SENSOR_READ_FAILED,
    RUN_MODE_ERR_ACTUATOR_FAILED,
    RUN_MODE_ERR_INVALID_PARAM
} run_mode_result_t;

/**
 * @brief Initialize the run mode manager
 *
 * Must be called once during system setup before using other functions.
 *
 * @return RUN_MODE_OK on success, error code otherwise
 */
run_mode_result_t run_mode_manager_init(void);

/**
 * @brief Enter RUN mode
 *
 * Called when transitioning into SYSTEM_MODE_RUN.
 * Performs any necessary setup for automatic watering operation.
 *
 * @return RUN_MODE_OK on success, error code otherwise
 */
run_mode_result_t run_mode_manager_enter(void);

/**
 * @brief Run mode main loop handler
 *
 * Must be called repeatedly in the main loop when in SYSTEM_MODE_RUN.
 * Implements non-blocking periodic humidity checks and watering logic:
 * - Every 5 seconds, reads soil moisture
 * - If moisture below threshold, triggers watering
 * - Logs all watering events
 *
 * @return RUN_MODE_OK on success, error code otherwise
 */
run_mode_result_t run_mode_manager_loop(void);

/**
 * @brief Exit RUN mode
 *
 * Called when transitioning out of SYSTEM_MODE_RUN.
 * Performs cleanup and ensures safe state.
 *
 * @return RUN_MODE_OK on success, error code otherwise
 */
run_mode_result_t run_mode_manager_exit(void);

/**
 * @brief Force a watering cycle for testing
 *
 * Bypasses the timer and threshold check to immediately execute
 * a complete "measure -> water -> log" sequence.
 * Used by test commands and debugging.
 *
 * @return RUN_MODE_OK on success, error code otherwise
 */
run_mode_result_t run_mode_manager_force_water(void);

#ifdef __cplusplus
}
#endif

#endif // RUN_MODE_MANAGER_H
