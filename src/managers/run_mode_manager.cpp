/**
 * @file run_mode_manager.cpp
 * @brief RUN mode manager implementation
 */

#include "run_mode_manager.h"
#include "sensor_manager.h"
#include "actuator_manager.h"
#include "log_manager.h"
#include "power_manager.h"
#include "ui/ui_manager.h"
#include "ui/display_manager.h"
#include <Arduino.h>
#include <stdio.h>
#include <lvgl.h>

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

// Display update state (for smart refresh mechanism)
static float s_last_displayed_humidity = -1.0f;  // Last displayed humidity percentage
static float s_last_displayed_voltage = -1.0f;   // Last displayed battery voltage
static uint8_t s_partial_refresh_count = 0;      // Counter for partial refreshes
static uint32_t s_last_full_refresh_time = 0;    // Timestamp of last full refresh
static uint32_t s_last_watering_time = 0;        // Timestamp of last watering event (millis)
static bool s_last_pump_state = false;           // Last known pump state

// Display update configuration
static const float HUMIDITY_CHANGE_THRESHOLD = 5.0f;  // Trigger update if humidity changes by 5%
static const float VOLTAGE_CHANGE_THRESHOLD = 0.1f;   // Trigger update if voltage changes by 0.1V
static const uint8_t PARTIAL_REFRESH_LIMIT = 10;      // Full refresh every 10 partial refreshes
static const uint32_t FULL_REFRESH_INTERVAL_MS = 1800000;  // Full refresh every 30 minutes

// Soil moisture calibration (hardcoded, future: user-configurable via NVS)
static const uint16_t SOIL_ADC_DRY = 2600;
static const uint16_t SOIL_ADC_WET = 1000;

/**
 * @brief Internal helper to convert ADC to humidity percentage
 */
static float adc_to_humidity_percent(uint16_t adc_value) {
    if (adc_value >= SOIL_ADC_DRY) return 0.0f;
    if (adc_value <= SOIL_ADC_WET) return 100.0f;
    return 100.0f - ((float)(adc_value - SOIL_ADC_WET) * 100.0f) / (SOIL_ADC_DRY - SOIL_ADC_WET);
}

/**
 * @brief Format relative time string for "last watering" display
 */
static void format_time_ago(uint32_t timestamp_ms, char* buf, size_t buf_size) {
    if (timestamp_ms == 0) {
        snprintf(buf, buf_size, "N/A");
        return;
    }

    uint32_t elapsed_ms = millis() - timestamp_ms;
    uint32_t elapsed_sec = elapsed_ms / 1000;

    if (elapsed_sec < 60) {
        snprintf(buf, buf_size, "%lus ago", elapsed_sec);
    } else if (elapsed_sec < 3600) {
        snprintf(buf, buf_size, "%lum ago", elapsed_sec / 60);
    } else {
        snprintf(buf, buf_size, "%luh ago", elapsed_sec / 3600);
    }
}

/**
 * @brief Update dashboard display with current system state
 * @param force_full_refresh If true, performs full refresh; otherwise decides based on counters
 */
static void update_dashboard(bool force_full_refresh) {
    // Read current sensor data
    float humidity_raw = 0.0f;
    float battery_voltage = 0.0f;

    sensor_manager_get_humidity(&humidity_raw);
    sensor_manager_get_battery_voltage(&battery_voltage);

    // Convert to displayable values
    float humidity_pct = adc_to_humidity_percent((uint16_t)humidity_raw);
    float threshold_pct = adc_to_humidity_percent(HUMIDITY_THRESHOLD);
    bool pump_running = actuator_manager_is_pump_running();

    // Format time and status strings
    char time_buf[16];
    format_time_ago(s_last_watering_time, time_buf, sizeof(time_buf));

    const char* status_str = pump_running ? "Watering..." : "Monitoring...";

    // Update LVGL dashboard
    ui_manager_show_run_dashboard(humidity_pct, threshold_pct, battery_voltage, time_buf, status_str);

    // Force LVGL to complete rendering synchronously
    lv_refr_now(NULL);

    // Smart refresh strategy: mix partial and full refresh
    bool do_full_refresh = force_full_refresh ||
                          (s_partial_refresh_count >= PARTIAL_REFRESH_LIMIT) ||
                          ((millis() - s_last_full_refresh_time) >= FULL_REFRESH_INTERVAL_MS);

    if (do_full_refresh) {
        LOG_INFO("RunMode", "Before full display refresh");
        display_manager_refresh(true);
        LOG_INFO("RunMode", "After full display refresh");
        s_partial_refresh_count = 0;
        s_last_full_refresh_time = millis();
    } else {
        display_manager_refresh(false);
        s_partial_refresh_count++;
    }

    // Update cached values
    s_last_displayed_humidity = humidity_pct;
    s_last_displayed_voltage = battery_voltage;
    s_last_pump_state = pump_running;
}

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

    // Step 4: Log watering event and record timestamp
    s_watering_count++;
    s_last_watering_time = millis();  // Record when watering started
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

    // Start timer from now, first periodic check after CHECK_INTERVAL_MS
    s_last_check_time = millis();

    // Power on display
    power_result_t power_result = power_screen_enable(true);
    if (power_result != POWER_OK) {
        LOG_ERROR("RunMode", "Failed to power on display (error %d)", power_result);
    }

    // Initialize display update state
    s_last_displayed_humidity = -1.0f;
    s_last_displayed_voltage = -1.0f;
    s_partial_refresh_count = 0;
    s_last_full_refresh_time = millis();
    s_last_pump_state = false;

    // Show initial dashboard with full refresh
    LOG_INFO("RunMode", "Displaying initial dashboard");
    update_dashboard(true);  // Force full refresh on entry

    return RUN_MODE_OK;
}

run_mode_result_t run_mode_manager_loop(void) {
    if (!s_initialized) {
        return RUN_MODE_ERR_NOT_INITIALIZED;
    }

    uint32_t current_time = millis();

    // Handle timer wrap-around (occurs after ~49 days)
    if (current_time < s_last_check_time) {
        s_last_check_time = 0;
    }

    // Check if it's time for periodic humidity check
    if (current_time - s_last_check_time >= CHECK_INTERVAL_MS) {
        s_last_check_time = current_time;

        LOG_INFO("RunMode", "Periodic humidity check triggered");

        // Read current sensor values for change detection
        float humidity_raw = 0.0f;
        float battery_voltage = 0.0f;
        sensor_manager_get_humidity(&humidity_raw);
        sensor_manager_get_battery_voltage(&battery_voltage);

        float humidity_pct = adc_to_humidity_percent((uint16_t)humidity_raw);
        bool pump_running = actuator_manager_is_pump_running();

        // Detect significant changes
        bool humidity_changed = (s_last_displayed_humidity < 0) ||
                               (fabs(humidity_pct - s_last_displayed_humidity) >= HUMIDITY_CHANGE_THRESHOLD);
        bool voltage_changed = (s_last_displayed_voltage < 0) ||
                              (fabs(battery_voltage - s_last_displayed_voltage) >= VOLTAGE_CHANGE_THRESHOLD);
        bool pump_state_changed = (pump_running != s_last_pump_state);

        // Execute watering sequence (will only water if humidity is low)
        run_mode_result_t result = execute_watering_sequence(false);
        if (result != RUN_MODE_OK) {
            LOG_ERROR("RunMode", "Watering sequence failed (error %d)", result);
        }

        // Update dashboard if significant change detected
        if (humidity_changed || voltage_changed || pump_state_changed) {
            LOG_INFO("RunMode", "Significant change detected - updating dashboard (H:%d V:%d P:%d)",
                     humidity_changed, voltage_changed, pump_state_changed);
            update_dashboard(false);  // Smart refresh (may be partial or full)
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

    // Power off display without calling display_manager_sleep()
    // (GxEPD2 static object state issue prevents proper reinitialization)
    power_result_t power_result = power_screen_enable(false);
    if (power_result != POWER_OK) {
        LOG_ERROR("RunMode", "Failed to power off display (error %d)", power_result);
    }

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
