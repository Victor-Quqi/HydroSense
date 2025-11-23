/**
 * @file interactive_status.cpp
 * @brief System status display implementation
 */

#include "interactive_status.h"
#include "interactive_common.h"

static bool status_logged = false;

void interactive_status_enter(void) {
    status_logged = false;
    LOG_DEBUG("Interactive", "Entered STATE_STATUS");
}

interactive_state_t interactive_status_handle(interactive_state_t* state) {
    if (!status_logged) {
        ConfigManager& config_mgr = ConfigManager::instance();
        hydro_config_t config = config_mgr.getConfig();

        // Read sensor data
        float humidity_raw = 0.0f;
        float battery_voltage = 0.0f;
        sensor_manager_get_humidity(&humidity_raw);
        sensor_manager_get_battery_voltage(&battery_voltage);

        // Calculate humidity percentage
        float humidity_pct = 0.0f;
        if (config.watering.humidity_dry > config.watering.humidity_wet) {
            humidity_pct = 100.0f - ((humidity_raw - config.watering.humidity_wet) * 100.0f) /
                          (config.watering.humidity_dry - config.watering.humidity_wet);
            if (humidity_pct < 0.0f) humidity_pct = 0.0f;
            if (humidity_pct > 100.0f) humidity_pct = 100.0f;
        }

        // Get network status
        WiFiManager& wifi = WiFiManager::instance();
        bool wifi_connected = wifi.isConnected();

        TimeManager& time_mgr = TimeManager::instance();
        bool time_synced = time_mgr.isTimeSynced();

        // Log status
        LOG_INFO("Interactive", "=== System Status ===");
        LOG_INFO("Interactive", "Sensor Data:");
        LOG_INFO("Interactive", "  Humidity: %.0f ADC (%.0f%%)", humidity_raw, humidity_pct);
        LOG_INFO("Interactive", "  Battery: %.2fV", battery_voltage);
        LOG_INFO("Interactive", "");
        LOG_INFO("Interactive", "Config Parameters:");
        LOG_INFO("Interactive", "  Threshold: %d ADC", config.watering.threshold);
        LOG_INFO("Interactive", "  Power: %d/255", config.watering.power);
        LOG_INFO("Interactive", "  Duration: %dms", config.watering.duration_ms);
        LOG_INFO("Interactive", "  Interval: %ds", config.watering.min_interval_s);
        LOG_INFO("Interactive", "");
        LOG_INFO("Interactive", "Network Status:");
        LOG_INFO("Interactive", "  WiFi: %s", wifi_connected ? "Connected" : "Disconnected");
        LOG_INFO("Interactive", "  Time: %s", time_synced ? "Synced" : "Not synced");
        LOG_INFO("Interactive", "");
        LOG_INFO("Interactive", "Press DOUBLE CLICK to return");

        status_logged = true;
    }

    // Double click to return
    if (input_manager_get_button_double_clicked()) {
        LOG_INFO("Interactive", "Returning to main menu from STATUS");
        status_logged = false;
        interactive_switch_state(STATE_MAIN_MENU, state);
    }

    return *state;
}
