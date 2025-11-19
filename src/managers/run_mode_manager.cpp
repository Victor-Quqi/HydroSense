/**
 * @file run_mode_manager.cpp
 * @brief RUN mode manager implementation
 */

#include "run_mode_manager.h"
#include "sensor_manager.h"
#include "actuator_manager.h"
#include "log_manager.h"
#include <Arduino.h>

// Configuration constants (hardcoded for initial implementation)
// TODO: Move these to config_manager for user-adjustable settings
static const uint32_t CHECK_INTERVAL_MS = 5000;      // Check humidity every 5 seconds
static const uint16_t HUMIDITY_THRESHOLD = 1500;     // ADC raw value threshold (capacitive sensor: higher = drier)
static const uint16_t WATERING_DURATION_MS = 3000;   // Run pump for 3 seconds
static const uint8_t PUMP_DUTY_CYCLE = 200;          // PWM duty cycle (0-255)

// State variables
static bool s_initialized = false;
static uint32_t s_last_check_time = 0;
static uint32_t s_watering_count = 0;  // Total watering events this session

/**
 * @brief Internal helper to execute watering sequence
 *
 * Performs: measure humidity -> decide -> water -> log
 *
 * @param force If true, skips threshold check and always waters
 * @return RUN_MODE_OK on success, error code otherwise
 */
static run_mode_result_t execute_watering_sequence(bool force) {
    float humidity = 0.0f;

    // Step 1: Read humidity sensor
    sensor_result_t sensor_result = sensor_manager_get_humidity(&humidity);
    if (sensor_result != SENSOR_OK) {
        LOG_ERROR("RunMode", "Failed to read humidity sensor (error %d)", sensor_result);
        return RUN_MODE_ERR_SENSOR_READ_FAILED;
    }

    LOG_DEBUG("RunMode", "Humidity reading: %.2f ADC units", humidity);

    // Step 2: Check if watering is needed
    // Capacitive sensor: higher value = drier soil
    bool should_water = force || (humidity > HUMIDITY_THRESHOLD);

    if (!should_water) {
        LOG_DEBUG("RunMode", "Humidity OK (%.2f <= %d), no watering needed", humidity, HUMIDITY_THRESHOLD);
        return RUN_MODE_OK;
    }

    // Step 3: Execute watering
    LOG_INFO("RunMode", "Humidity LOW (%.2f > %d), starting watering cycle", humidity, HUMIDITY_THRESHOLD);

    actuator_manager_run_pump_for(PUMP_DUTY_CYCLE, WATERING_DURATION_MS);

    // Step 4: Log watering event
    s_watering_count++;
    LOG_INFO("RunMode", "Watering event #%lu: humidity=%.2f, duration=%dms, duty=%d/255",
             s_watering_count, humidity, WATERING_DURATION_MS, PUMP_DUTY_CYCLE);

    return RUN_MODE_OK;
}

run_mode_result_t run_mode_manager_init(void) {
    if (s_initialized) {
        LOG_DEBUG("RunMode", "Run mode manager already initialized");
        return RUN_MODE_OK;
    }

    s_last_check_time = 0;
    s_watering_count = 0;
    s_initialized = true;

    LOG_INFO("RunMode", "Run mode manager initialized (check_interval=%lums, threshold=%d)",
             CHECK_INTERVAL_MS, HUMIDITY_THRESHOLD);

    return RUN_MODE_OK;
}

run_mode_result_t run_mode_manager_enter(void) {
    if (!s_initialized) {
        LOG_ERROR("RunMode", "Run mode manager not initialized");
        return RUN_MODE_ERR_NOT_INITIALIZED;
    }

    LOG_INFO("RunMode", "Entering RUN mode - automatic watering active");

    // Reset timer to trigger first check immediately
    s_last_check_time = 0;

    return RUN_MODE_OK;
}

run_mode_result_t run_mode_manager_loop(void) {
    if (!s_initialized) {
        return RUN_MODE_ERR_NOT_INITIALIZED;
    }

    // Non-blocking timer check
    uint32_t current_time = millis();

    // Handle timer wrap-around (occurs after ~49 days)
    if (current_time < s_last_check_time) {
        s_last_check_time = 0;
    }

    // Check if it's time for periodic humidity check
    if (current_time - s_last_check_time >= CHECK_INTERVAL_MS) {
        s_last_check_time = current_time;

        LOG_DEBUG("RunMode", "Periodic humidity check triggered");

        // Execute watering sequence (will only water if humidity is low)
        run_mode_result_t result = execute_watering_sequence(false);
        if (result != RUN_MODE_OK) {
            LOG_ERROR("RunMode", "Watering sequence failed (error %d)", result);
        }
    }

    return RUN_MODE_OK;
}

run_mode_result_t run_mode_manager_exit(void) {
    if (!s_initialized) {
        return RUN_MODE_ERR_NOT_INITIALIZED;
    }

    LOG_INFO("RunMode", "Exiting RUN mode - %lu watering events this session", s_watering_count);

    // Stop any ongoing pump operation
    actuator_manager_stop_pump();

    return RUN_MODE_OK;
}

run_mode_result_t run_mode_manager_force_water(void) {
    if (!s_initialized) {
        LOG_ERROR("RunMode", "Run mode manager not initialized");
        return RUN_MODE_ERR_NOT_INITIALIZED;
    }

    LOG_INFO("RunMode", "Force water command received - executing watering sequence");

    // Execute watering sequence with force flag (bypasses threshold check)
    return execute_watering_sequence(true);
}
