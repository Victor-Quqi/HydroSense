/**
 * @file interactive_mode_manager.cpp
 * @brief Interactive mode manager implementation
 */

#include "interactive_mode_manager.h"
#include "input_manager.h"
#include "log_manager.h"
#include "sensor_manager.h"
#include "../ui/ui_manager.h"
#include "../services/config_manager.h"
#include "../services/wifi_manager.h"
#include "../services/time_manager.h"
#include <Arduino.h>

// --- 状态机变量 ---
static bool is_initialized = false;
static interactive_state_t current_state = STATE_MAIN_MENU;
static bool exit_requested = false; // 用于测试模式退出

// --- 主菜单定义 ---
#define MAIN_MENU_ITEM_COUNT 4
static const char* main_menu_items[MAIN_MENU_ITEM_COUNT] = {
    "系统状态",
    "系统设置",
    "立即浇水",
    "聊天"
};
static int main_menu_index = 0; // 当前选中的菜单项索引

// --- 设置菜单定义 ---
typedef enum {
    SETTING_THRESHOLD = 0,    // 浇水阈值
    SETTING_POWER,            // 浇水功率
    SETTING_DURATION,         // 浇水时长
    SETTING_INTERVAL,         // 最短间隔
    SETTING_HUMIDITY_WET,     // 湿度下限
    SETTING_HUMIDITY_DRY,     // 湿度上限
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

// 设置项范围和步长
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

static int settings_menu_index = 0;  // 当前选中的设置项索引
static int setting_preview_value = 0; // 编辑时的预览值
static setting_item_t current_editing_setting = SETTING_THRESHOLD; // 当前编辑的设置项

// --- 状态处理函数声明 ---
static void handle_main_menu_state();
static void handle_status_state();
static void handle_settings_state();
static void handle_setting_edit_state();
static void handle_watering_state();
static void handle_chat_state();

// --- API实现 ---

interactive_mode_result_t interactive_mode_manager_init(void) {
    LOG_INFO("Interactive", "Initializing interactive mode manager");

    is_initialized = true;
    current_state = STATE_MAIN_MENU;
    main_menu_index = 0;

    return INTERACTIVE_MODE_OK;
}

interactive_mode_result_t interactive_mode_manager_enter(void) {
    if (!is_initialized) {
        LOG_ERROR("Interactive", "Manager not initialized");
        return INTERACTIVE_MODE_ERR_NOT_INITIALIZED;
    }

    LOG_INFO("Interactive", "Entering interactive mode");

    // 清除累积的输入事件，避免模式切换时的干扰
    input_manager_clear_events();

    // 重置状态为主菜单
    current_state = STATE_MAIN_MENU;
    main_menu_index = 0;
    exit_requested = false; // 清除退出标志

    // TODO: 显示主菜单UI
    LOG_DEBUG("Interactive", "Displaying main menu");

    return INTERACTIVE_MODE_OK;
}

interactive_mode_result_t interactive_mode_manager_exit(void) {
    if (!is_initialized) {
        return INTERACTIVE_MODE_ERR_NOT_INITIALIZED;
    }

    LOG_INFO("Interactive", "Exiting interactive mode");

    // TODO: 清除屏幕，保存状态

    return INTERACTIVE_MODE_OK;
}

interactive_mode_result_t interactive_mode_manager_loop(void) {
    if (!is_initialized) {
        return INTERACTIVE_MODE_ERR_NOT_INITIALIZED;
    }

    // 基于当前状态的switch-case处理
    switch (current_state) {
        case STATE_MAIN_MENU:
            handle_main_menu_state();
            break;

        case STATE_STATUS:
            handle_status_state();
            break;

        case STATE_SETTINGS:
            handle_settings_state();
            break;

        case STATE_SETTING_EDIT:
            handle_setting_edit_state();
            break;

        case STATE_WATERING:
            handle_watering_state();
            break;

        case STATE_CHAT:
            handle_chat_state();
            break;

        default:
            LOG_ERROR("Interactive", "Unknown state: %d", current_state);
            current_state = STATE_MAIN_MENU;
            break;
    }

    return INTERACTIVE_MODE_OK;
}

// --- 状态处理函数实现 ---

static void handle_main_menu_state() {
    // 读取编码器增量
    int8_t delta = input_manager_get_encoder_delta();
    if (delta != 0) {
        // 调整菜单索引（循环）
        main_menu_index += delta;
        if (main_menu_index < 0) {
            main_menu_index = MAIN_MENU_ITEM_COUNT - 1; // 回到末尾
        } else if (main_menu_index >= MAIN_MENU_ITEM_COUNT) {
            main_menu_index = 0; // 回到开头
        }

        LOG_DEBUG("Interactive", "Main menu: selected [%d] %s",
                  main_menu_index, main_menu_items[main_menu_index]);

        // TODO: 更新UI显示（高亮当前选项）
    }

    // 检测单击：进入子功能
    if (input_manager_get_button_clicked()) {
        LOG_INFO("Interactive", "Main menu: entering [%d] %s",
                 main_menu_index, main_menu_items[main_menu_index]);

        // 根据选中项进入对应状态
        switch (main_menu_index) {
            case 0: // 系统状态
                current_state = STATE_STATUS;
                LOG_DEBUG("Interactive", "Switched to STATE_STATUS");
                break;

            case 1: // 系统设置
                current_state = STATE_SETTINGS;
                LOG_DEBUG("Interactive", "Switched to STATE_SETTINGS");
                break;

            case 2: // 立即浇水
                current_state = STATE_WATERING;
                LOG_DEBUG("Interactive", "Switched to STATE_WATERING");
                break;

            case 3: // 聊天
                current_state = STATE_CHAT;
                LOG_DEBUG("Interactive", "Switched to STATE_CHAT");
                break;

            default:
                LOG_ERROR("Interactive", "Invalid menu index: %d", main_menu_index);
                break;
        }
    }

    // 双击在主菜单：生产模式无效果，测试模式设置退出标志
    if (input_manager_get_button_double_clicked()) {
#ifdef TEST_MODE
        LOG_INFO("Interactive", "Double click in main menu - exit requested");
        exit_requested = true;
#else
        LOG_DEBUG("Interactive", "Double click in main menu (ignored)");
#endif
    }
}

static void handle_status_state() {
    static bool status_logged = false;

    // 首次进入状态时，读取数据并输出LOG验证
    if (!status_logged) {
        LOG_INFO("Interactive", "=== System Status ===");

        // 1. 读取传感器数据
        sensor_data_t sensor_data;
        sensor_result_t sensor_result = sensor_manager_read_all(&sensor_data);

        float battery_v = 0.0f;
        uint16_t humidity_raw = 0;

        if (sensor_result == SENSOR_OK) {
            battery_v = sensor_data.battery_voltage;
            humidity_raw = sensor_data.soil_moisture;
        } else {
            LOG_ERROR("Interactive", "Failed to read sensor data");
            humidity_raw = 0;
            battery_v = 0.0f;
        }

        // 2. 获取配置
        hydro_config_t& config = ConfigManager::instance().getConfig();

        uint16_t threshold_raw = config.watering.threshold;
        uint16_t power = 200; // 默认值，等待config扩展
        uint16_t duration_ms = config.watering.duration_ms;
        uint16_t interval_s = config.watering.min_interval_s;

        // TODO: 等待config扩展power, humidity_wet, humidity_dry字段后更新
        uint16_t humidity_wet = 1000; // 湿润阈值（临时硬编码）
        uint16_t humidity_dry = 2600; // 干燥阈值（临时硬编码）

        // 3. 计算百分比（ADC值越高越干，反向映射）
        auto calc_humidity_percent = [humidity_wet, humidity_dry](uint16_t raw) -> float {
            if (raw >= humidity_dry) return 0.0f;
            if (raw <= humidity_wet) return 100.0f;
            return 100.0f - ((float)(raw - humidity_wet) * 100.0f) / (humidity_dry - humidity_wet);
        };

        float humidity_pct = calc_humidity_percent(humidity_raw);
        float threshold_pct = calc_humidity_percent(threshold_raw);

        // 4. 检查网络状态
        bool wifi_connected = WiFiManager::instance().isConnected();
        bool time_synced = TimeManager::instance().isTimeSynced();

        // 5. 输出LOG验证业务逻辑
        LOG_INFO("Interactive", "Sensor Data:");
        LOG_INFO("Interactive", "  Humidity: %u (%.1f%%)", humidity_raw, humidity_pct);
        LOG_INFO("Interactive", "  Battery: %.2fV", battery_v);
        LOG_INFO("Interactive", "Config:");
        LOG_INFO("Interactive", "  Threshold: %u (%.1f%%)", threshold_raw, threshold_pct);
        LOG_INFO("Interactive", "  Power: %u", power);
        LOG_INFO("Interactive", "  Duration: %ums", duration_ms);
        LOG_INFO("Interactive", "  Interval: %us", interval_s);
        LOG_INFO("Interactive", "Network:");
        LOG_INFO("Interactive", "  WiFi: %s", wifi_connected ? "Connected" : "Disconnected");
        LOG_INFO("Interactive", "  Time: %s", time_synced ? "Synced" : "Not synced");
        LOG_INFO("Interactive", "===================");

        // TODO: P2阶段统一实现UI显示
        // ui_manager_show_status(humidity_raw, humidity_pct, battery_v,
        //                       threshold_raw, threshold_pct,
        //                       power, duration_ms, interval_s,
        //                       wifi_connected, time_synced);

        status_logged = true;
    }

    // 检测双击返回主菜单
    if (input_manager_get_button_double_clicked()) {
        LOG_INFO("Interactive", "Returning to main menu from STATUS");
        status_logged = false; // 重置标志，下次进入时重新输出
        current_state = STATE_MAIN_MENU;
    }
}

static void handle_settings_state() {
    static bool settings_logged = false;

    // 首次进入时，输出所有设置项和当前值
    if (!settings_logged) {
        LOG_INFO("Interactive", "=== Settings Menu ===");

        hydro_config_t& config = ConfigManager::instance().getConfig();

        // 输出所有设置项
        LOG_INFO("Interactive", "[%d] %s: %u %s",
                0, setting_names[SETTING_THRESHOLD],
                config.watering.threshold, setting_ranges[SETTING_THRESHOLD].unit);
        LOG_INFO("Interactive", "[%d] %s: %u %s",
                1, setting_names[SETTING_POWER],
                config.watering.power, setting_ranges[SETTING_POWER].unit);
        LOG_INFO("Interactive", "[%d] %s: %u %s",
                2, setting_names[SETTING_DURATION],
                config.watering.duration_ms, setting_ranges[SETTING_DURATION].unit);
        LOG_INFO("Interactive", "[%d] %s: %u %s",
                3, setting_names[SETTING_INTERVAL],
                config.watering.min_interval_s, setting_ranges[SETTING_INTERVAL].unit);
        LOG_INFO("Interactive", "[%d] %s: %u %s",
                4, setting_names[SETTING_HUMIDITY_WET],
                config.watering.humidity_wet, setting_ranges[SETTING_HUMIDITY_WET].unit);
        LOG_INFO("Interactive", "[%d] %s: %u %s",
                5, setting_names[SETTING_HUMIDITY_DRY],
                config.watering.humidity_dry, setting_ranges[SETTING_HUMIDITY_DRY].unit);
        LOG_INFO("Interactive", "===================");

        // TODO: P2阶段实现UI显示
        // ui_manager_show_settings_menu(...)

        settings_logged = true;
    }

    // 旋转编码器选择设置项
    int8_t delta = input_manager_get_encoder_delta();
    if (delta != 0) {
        settings_menu_index += delta;

        // 循环导航
        if (settings_menu_index < 0) {
            settings_menu_index = SETTING_COUNT - 1;
        } else if (settings_menu_index >= SETTING_COUNT) {
            settings_menu_index = 0;
        }

        LOG_DEBUG("Interactive", "Settings menu: selected [%d] %s",
                 settings_menu_index, setting_names[settings_menu_index]);
    }

    // 单击进入编辑状态
    if (input_manager_get_button_clicked()) {
        current_editing_setting = (setting_item_t)settings_menu_index;

        // 初始化预览值为当前值
        hydro_config_t& config = ConfigManager::instance().getConfig();
        switch (current_editing_setting) {
            case SETTING_THRESHOLD:
                setting_preview_value = config.watering.threshold;
                break;
            case SETTING_POWER:
                setting_preview_value = config.watering.power;
                break;
            case SETTING_DURATION:
                setting_preview_value = config.watering.duration_ms;
                break;
            case SETTING_INTERVAL:
                setting_preview_value = config.watering.min_interval_s;
                break;
            case SETTING_HUMIDITY_WET:
                setting_preview_value = config.watering.humidity_wet;
                break;
            case SETTING_HUMIDITY_DRY:
                setting_preview_value = config.watering.humidity_dry;
                break;
            default:
                break;
        }

        LOG_INFO("Interactive", "Entering edit mode for: %s", setting_names[current_editing_setting]);
        current_state = STATE_SETTING_EDIT;
    }

    // 双击返回主菜单
    if (input_manager_get_button_double_clicked()) {
        LOG_INFO("Interactive", "Returning to main menu from SETTINGS");
        settings_logged = false; // 重置标志，下次进入时重新输出
        current_state = STATE_MAIN_MENU;
    }
}

static void handle_setting_edit_state() {
    static bool edit_logged = false;

    // 首次进入时，输出当前值和范围
    if (!edit_logged) {
        const setting_range_t& range = setting_ranges[current_editing_setting];

        // 获取当前config中的值
        hydro_config_t& config = ConfigManager::instance().getConfig();
        int current_value = 0;
        switch (current_editing_setting) {
            case SETTING_THRESHOLD:
                current_value = config.watering.threshold;
                break;
            case SETTING_POWER:
                current_value = config.watering.power;
                break;
            case SETTING_DURATION:
                current_value = config.watering.duration_ms;
                break;
            case SETTING_INTERVAL:
                current_value = config.watering.min_interval_s;
                break;
            case SETTING_HUMIDITY_WET:
                current_value = config.watering.humidity_wet;
                break;
            case SETTING_HUMIDITY_DRY:
                current_value = config.watering.humidity_dry;
                break;
            default:
                break;
        }

        LOG_INFO("Interactive", "=== Editing: %s ===", setting_names[current_editing_setting]);
        LOG_INFO("Interactive", "Current: %d %s", current_value, range.unit);
        LOG_INFO("Interactive", "Preview: %d %s", setting_preview_value, range.unit);
        LOG_INFO("Interactive", "Range: %d-%d, Step: %d", range.min, range.max, range.step);
        LOG_INFO("Interactive", "Rotate to adjust, Click to save, Double-click to cancel");
        LOG_INFO("Interactive", "===================");

        // TODO: P2阶段实现UI显示
        // ui_manager_show_setting_edit(...)

        edit_logged = true;
    }

    // 旋转编码器调整预览值
    int8_t delta = input_manager_get_encoder_delta();
    if (delta != 0) {
        const setting_range_t& range = setting_ranges[current_editing_setting];

        setting_preview_value += delta * range.step;

        // 限制在合法范围内
        if (setting_preview_value < range.min) {
            setting_preview_value = range.min;
        } else if (setting_preview_value > range.max) {
            setting_preview_value = range.max;
        }

        LOG_DEBUG("Interactive", "Preview adjusted: %d %s", setting_preview_value, range.unit);
    }

    // 单击保存到config
    if (input_manager_get_button_clicked()) {
        hydro_config_t& config = ConfigManager::instance().getConfig();

        // 更新config
        switch (current_editing_setting) {
            case SETTING_THRESHOLD:
                config.watering.threshold = setting_preview_value;
                break;
            case SETTING_POWER:
                config.watering.power = setting_preview_value;
                break;
            case SETTING_DURATION:
                config.watering.duration_ms = setting_preview_value;
                break;
            case SETTING_INTERVAL:
                config.watering.min_interval_s = setting_preview_value;
                break;
            case SETTING_HUMIDITY_WET:
                config.watering.humidity_wet = setting_preview_value;
                break;
            case SETTING_HUMIDITY_DRY:
                config.watering.humidity_dry = setting_preview_value;
                break;
            default:
                break;
        }

        // 保存到NVS
        bool save_ok = ConfigManager::instance().saveConfig();
        if (save_ok) {
            LOG_INFO("Interactive", "Setting saved: %s = %d",
                    setting_names[current_editing_setting], setting_preview_value);
        } else {
            LOG_ERROR("Interactive", "Failed to save setting to NVS");
        }

        edit_logged = false; // 重置标志
        current_state = STATE_SETTINGS; // 返回设置菜单
    }

    // 双击取消并返回
    if (input_manager_get_button_double_clicked()) {
        LOG_INFO("Interactive", "Edit cancelled, returning to settings menu");
        edit_logged = false; // 重置标志
        current_state = STATE_SETTINGS;
    }
}

static void handle_watering_state() {
    // TODO: P1.5 实现浇水逻辑
    // - 显示浇水进度
    // - 完成后返回主菜单

    // 临时：双击返回主菜单
    if (input_manager_get_button_double_clicked()) {
        LOG_INFO("Interactive", "Returning to main menu from WATERING");
        current_state = STATE_MAIN_MENU;
    }
}

static void handle_chat_state() {
    // TODO: P1.6 实现聊天界面逻辑
    // - 显示LLM回复和选项
    // - 单击选择，双击返回

    // 临时：双击返回主菜单
    if (input_manager_get_button_double_clicked()) {
        LOG_INFO("Interactive", "Returning to main menu from CHAT");
        current_state = STATE_MAIN_MENU;
    }
}

bool interactive_mode_manager_should_exit(void) {
    return exit_requested;
}
