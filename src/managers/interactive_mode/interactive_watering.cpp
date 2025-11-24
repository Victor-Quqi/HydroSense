/**
 * @file interactive_watering.cpp
 * @brief Watering flow implementation
 */

#include "interactive_watering.h"
#include "interactive_common.h"
#include "../../ui/ui_manager.h"

typedef enum {
    WATERING_CONFIRM,
    WATERING_IN_PROGRESS,
    WATERING_COMPLETE
} watering_substate_t;

static watering_substate_t watering_substate = WATERING_CONFIRM;
static float watering_humidity_before = 0.0f;
static float watering_humidity_after = 0.0f;
static uint32_t watering_start_time = 0;
static uint16_t watering_duration_ms = 0;
static uint8_t watering_power = 0;
static bool confirm_logged = false;
static bool progress_logged = false;
static bool complete_logged = false;

void interactive_watering_enter(void) {
    watering_substate = WATERING_CONFIRM;
    confirm_logged = false;
    progress_logged = false;
    complete_logged = false;
    LOG_DEBUG("Interactive", "Entered STATE_WATERING");
}

interactive_state_t interactive_watering_handle(interactive_state_t* state) {
    ConfigManager& config_mgr = ConfigManager::instance();
    hydro_config_t config = config_mgr.getConfig();

    switch (watering_substate) {
        case WATERING_CONFIRM: {
            if (!confirm_logged) {
                watering_power = config.watering.power;
                watering_duration_ms = config.watering.duration_ms;
                sensor_manager_get_humidity(&watering_humidity_before);

                // Calculate humidity percentage
                float humidity_pct = 0.0f;
                if (config.watering.humidity_dry > config.watering.humidity_wet) {
                    humidity_pct = 100.0f - ((watering_humidity_before - config.watering.humidity_wet) * 100.0f) /
                                  (config.watering.humidity_dry - config.watering.humidity_wet);
                    if (humidity_pct < 0.0f) humidity_pct = 0.0f;
                    if (humidity_pct > 100.0f) humidity_pct = 100.0f;
                }

#ifdef TEST_MODE
                // TEST_MODE: 串口LOG输出
                LOG_INFO("Interactive", "=== Watering Confirmation ===");
                LOG_INFO("Interactive", "Pump Power: %d/255", watering_power);
                LOG_INFO("Interactive", "Duration: %dms", watering_duration_ms);
                LOG_INFO("Interactive", "Current Humidity: %.2f ADC", watering_humidity_before);
                LOG_INFO("Interactive", "Press SINGLE CLICK to start, DOUBLE CLICK to cancel");
#else
                // 生产环境: 调用UI显示
                ui_manager_show_watering_confirm(watering_power, watering_duration_ms, humidity_pct);
#endif
                confirm_logged = true;
            }

            // 长按处理：触发全刷
            if (input_manager_get_button_long_pressed()) {
                LOG_INFO("Interactive", "Long press - full refresh");
                ui_manager_trigger_full_refresh();
            }

            if (input_manager_get_button_clicked()) {
                LOG_INFO("Interactive", "Watering confirmed, starting pump...");
                actuator_manager_run_pump_for(watering_power, watering_duration_ms);
                watering_start_time = millis();
                watering_substate = WATERING_IN_PROGRESS;
                confirm_logged = false;
                progress_logged = false;
                LOG_DEBUG("Interactive", "Switched to WATERING_IN_PROGRESS");
            }

            if (input_manager_get_button_double_clicked()) {
                LOG_INFO("Interactive", "Watering cancelled, returning to main menu");
                watering_substate = WATERING_CONFIRM;
                confirm_logged = false;
                interactive_switch_state(STATE_MAIN_MENU, state);
            }
            break;
        }

        case WATERING_IN_PROGRESS: {
            actuator_manager_loop();
            bool pump_running = actuator_manager_is_pump_running();

            if (pump_running) {
                uint32_t elapsed = millis() - watering_start_time;

                // Calculate humidity percentage for UI
                float humidity_pct = 0.0f;
                ConfigManager& config_mgr = ConfigManager::instance();
                hydro_config_t config = config_mgr.getConfig();
                if (config.watering.humidity_dry > config.watering.humidity_wet) {
                    humidity_pct = 100.0f - ((watering_humidity_before - config.watering.humidity_wet) * 100.0f) /
                                  (config.watering.humidity_dry - config.watering.humidity_wet);
                    if (humidity_pct < 0.0f) humidity_pct = 0.0f;
                    if (humidity_pct > 100.0f) humidity_pct = 100.0f;
                }

#ifdef TEST_MODE
                // TEST_MODE: 串口LOG输出（仅一次）
                if (!progress_logged) {
                    uint8_t progress = (elapsed * 100) / watering_duration_ms;
                    if (progress > 100) progress = 100;
                    LOG_INFO("Interactive", "Watering in progress... %d%%", progress);
                    progress_logged = true;
                }
#else
                // 生产环境: 添加UI更新节流，防止队列溢出
                static uint32_t last_ui_update = 0;
                if (millis() - last_ui_update >= 500) {  // 每500ms更新一次
                    ui_manager_show_watering_progress(elapsed, watering_duration_ms, humidity_pct);
                    last_ui_update = millis();
                }
#endif
            } else {
                LOG_INFO("Interactive", "Watering completed");
                sensor_manager_get_humidity(&watering_humidity_after);
                watering_substate = WATERING_COMPLETE;
                progress_logged = false;
                complete_logged = false;

                // 重置浇水进度UI状态，准备下次进入时重新创建
                ui_manager_reset_watering_progress();

                LOG_DEBUG("Interactive", "Switched to WATERING_COMPLETE");
            }

            // 长按处理：触发全刷
            if (input_manager_get_button_long_pressed()) {
                LOG_INFO("Interactive", "Long press - full refresh");
                ui_manager_trigger_full_refresh();
            }

            break;
        }

        case WATERING_COMPLETE: {
            if (!complete_logged) {
                // Calculate humidity percentages for UI
                ConfigManager& config_mgr = ConfigManager::instance();
                hydro_config_t config = config_mgr.getConfig();

                float humidity_before_pct = 0.0f;
                float humidity_after_pct = 0.0f;
                if (config.watering.humidity_dry > config.watering.humidity_wet) {
                    humidity_before_pct = 100.0f - ((watering_humidity_before - config.watering.humidity_wet) * 100.0f) /
                                         (config.watering.humidity_dry - config.watering.humidity_wet);
                    if (humidity_before_pct < 0.0f) humidity_before_pct = 0.0f;
                    if (humidity_before_pct > 100.0f) humidity_before_pct = 100.0f;

                    humidity_after_pct = 100.0f - ((watering_humidity_after - config.watering.humidity_wet) * 100.0f) /
                                        (config.watering.humidity_dry - config.watering.humidity_wet);
                    if (humidity_after_pct < 0.0f) humidity_after_pct = 0.0f;
                    if (humidity_after_pct > 100.0f) humidity_after_pct = 100.0f;
                }

#ifdef TEST_MODE
                // TEST_MODE: 串口LOG输出
                LOG_INFO("Interactive", "=== Watering Result ===");
                LOG_INFO("Interactive", "Humidity BEFORE: %.2f ADC", watering_humidity_before);
                LOG_INFO("Interactive", "Humidity AFTER: %.2f ADC", watering_humidity_after);
                LOG_INFO("Interactive", "Change: %.2f ADC",
                         watering_humidity_after - watering_humidity_before);
                LOG_INFO("Interactive", "Press DOUBLE CLICK to return to main menu");
#else
                // 生产环境: 调用UI显示
                ui_manager_show_watering_result(humidity_before_pct, humidity_after_pct);
#endif
                complete_logged = true;
            }

            // 长按处理：触发全刷
            if (input_manager_get_button_long_pressed()) {
                LOG_INFO("Interactive", "Long press - full refresh");
                ui_manager_trigger_full_refresh();
            }

            if (input_manager_get_button_double_clicked()) {
                LOG_INFO("Interactive", "Returning to main menu");
                watering_substate = WATERING_CONFIRM;
                confirm_logged = false;
                progress_logged = false;
                complete_logged = false;
                interactive_switch_state(STATE_MAIN_MENU, state);
            }
            break;
        }

        default:
            LOG_ERROR("Interactive", "Unknown watering substate: %d", watering_substate);
            watering_substate = WATERING_CONFIRM;
            break;
    }

    return *state;
}
