/**
 * @file interactive_settings.cpp
 * @brief Settings menu implementation
 */

#include "interactive_settings.h"
#include "interactive_common.h"

// Setting item enumeration
typedef enum {
    SETTING_THRESHOLD = 0,
    SETTING_POWER,
    SETTING_DURATION,
    SETTING_INTERVAL,
    SETTING_HUMIDITY_WET,
    SETTING_HUMIDITY_DRY,
    SETTING_COUNT
} setting_item_t;

static const char* setting_names[SETTING_COUNT] = {
    "Watering Threshold",
    "Pump Power",
    "Watering Duration",
    "Min Interval",
    "Humidity Wet",
    "Humidity Dry"
};

typedef struct {
    int min;
    int max;
    int step;
    const char* unit;
} setting_range_t;

static const setting_range_t setting_ranges[SETTING_COUNT] = {
    {100, 4000, 50, "ADC"},      // threshold
    {0, 255, 10, ""},            // power
    {1000, 60000, 500, "ms"},    // duration
    {60, 3600, 60, "s"},         // interval
    {100, 3000, 100, "ADC"},     // humidity_wet
    {100, 3000, 100, "ADC"}      // humidity_dry
};

// State variables
static int settings_menu_index = 0;
static bool settings_logged = false;
static setting_item_t current_editing_setting = SETTING_THRESHOLD;
static int setting_preview_value = 0;
static bool edit_logged = false;

// Helper to get current value
static int get_setting_value(const hydro_config_t& config, setting_item_t item) {
    switch (item) {
        case SETTING_THRESHOLD: return config.watering.threshold;
        case SETTING_POWER: return config.watering.power;
        case SETTING_DURATION: return config.watering.duration_ms;
        case SETTING_INTERVAL: return config.watering.min_interval_s;
        case SETTING_HUMIDITY_WET: return config.watering.humidity_wet;
        case SETTING_HUMIDITY_DRY: return config.watering.humidity_dry;
        default: return 0;
    }
}

// Helper to set value
static void set_setting_value(hydro_config_t& config, setting_item_t item, int value) {
    switch (item) {
        case SETTING_THRESHOLD: config.watering.threshold = value; break;
        case SETTING_POWER: config.watering.power = value; break;
        case SETTING_DURATION: config.watering.duration_ms = value; break;
        case SETTING_INTERVAL: config.watering.min_interval_s = value; break;
        case SETTING_HUMIDITY_WET: config.watering.humidity_wet = value; break;
        case SETTING_HUMIDITY_DRY: config.watering.humidity_dry = value; break;
    }
}

void interactive_settings_enter(void) {
    settings_menu_index = 0;
    settings_logged = false;
    LOG_DEBUG("Interactive", "Entered STATE_SETTINGS");
}

interactive_state_t interactive_settings_handle(interactive_state_t* state) {
    ConfigManager& config_mgr = ConfigManager::instance();
    hydro_config_t config = config_mgr.getConfig();

    if (!settings_logged) {
        LOG_INFO("Interactive", "=== Settings Menu ===");
        for (int i = 0; i < SETTING_COUNT; i++) {
            int value = get_setting_value(config, (setting_item_t)i);
            const char* marker = (i == settings_menu_index) ? ">" : " ";
            LOG_INFO("Interactive", "%s %d. %s: %d %s", marker, i, setting_names[i],
                     value, setting_ranges[i].unit);
        }
        LOG_INFO("Interactive", "Click=Edit, Double-Click=Return");
        settings_logged = true;
    }

    // Encoder rotation
    int8_t delta = input_manager_get_encoder_delta();
    if (delta != 0) {
        settings_menu_index = (settings_menu_index + delta + SETTING_COUNT) % SETTING_COUNT;
        LOG_INFO("Interactive", "Selected: %s", setting_names[settings_menu_index]);
        settings_logged = false;
    }

    // Single click - enter edit mode
    if (input_manager_get_button_clicked()) {
        current_editing_setting = (setting_item_t)settings_menu_index;
        LOG_INFO("Interactive", "Entering edit mode for: %s", setting_names[current_editing_setting]);
        interactive_switch_state(STATE_SETTING_EDIT, state);
    }

    // Double click - return to main menu
    if (input_manager_get_button_double_clicked()) {
        LOG_INFO("Interactive", "Returning to main menu from SETTINGS");
        settings_logged = false;
        interactive_switch_state(STATE_MAIN_MENU, state);
    }

    return *state;
}

void interactive_setting_edit_enter(void) {
    ConfigManager& config_mgr = ConfigManager::instance();
    hydro_config_t config = config_mgr.getConfig();
    setting_preview_value = get_setting_value(config, current_editing_setting);
    edit_logged = false;
    LOG_DEBUG("Interactive", "Entered STATE_SETTING_EDIT");
}

interactive_state_t interactive_setting_edit_handle(interactive_state_t* state) {
    ConfigManager& config_mgr = ConfigManager::instance();
    const setting_range_t& range = setting_ranges[current_editing_setting];

    if (!edit_logged) {
        LOG_INFO("Interactive", "=== Editing: %s ===", setting_names[current_editing_setting]);
        LOG_INFO("Interactive", "Preview: %d %s", setting_preview_value, range.unit);
        LOG_INFO("Interactive", "Range: %d-%d, Step: %d", range.min, range.max, range.step);
        LOG_INFO("Interactive", "Rotate=Adjust, Click=Save, Double-Click=Cancel");
        edit_logged = true;
    }

    // Encoder rotation - adjust value
    int8_t delta = input_manager_get_encoder_delta();
    if (delta != 0) {
        setting_preview_value += delta * range.step;
        if (setting_preview_value < range.min) setting_preview_value = range.min;
        if (setting_preview_value > range.max) setting_preview_value = range.max;
        LOG_INFO("Interactive", "Preview: %d %s", setting_preview_value, range.unit);
        edit_logged = false;
    }

    // Single click - save
    if (input_manager_get_button_clicked()) {
        hydro_config_t& config = config_mgr.getConfig();  // Get reference
        set_setting_value(config, current_editing_setting, setting_preview_value);
        config_mgr.saveConfig();
        LOG_INFO("Interactive", "Saved %s = %d", setting_names[current_editing_setting],
                 setting_preview_value);
        edit_logged = false;
        interactive_switch_state(STATE_SETTINGS, state);
    }

    // Double click - cancel
    if (input_manager_get_button_double_clicked()) {
        LOG_INFO("Interactive", "Edit cancelled, returning to settings menu");
        edit_logged = false;
        interactive_switch_state(STATE_SETTINGS, state);
    }

    return *state;
}
