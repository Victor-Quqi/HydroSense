/**
 * @file ui_manager.cpp
 * @brief UI管理器实现
 */
#include "ui_manager.h"
#include "display_manager.h"
#include "managers/log_manager.h"
#include <lvgl.h>
#include <stdio.h>

// 土壤湿度传感器校准常量（硬编码，未来可通过持久化存储实现用户校准）
#define SOIL_ADC_DRY  2600  // 干燥土壤的ADC值
#define SOIL_ADC_WET  1000  // 湿润土壤的ADC值

// LVGL 显示缓冲区
// 墨水屏尺寸: 296x128. 缓冲区可以设置为屏幕大小的1/10，以平衡内存和性能
#define LV_DISP_BUF_SIZE (296 * 128 / 10)
static lv_disp_draw_buf_t s_disp_buf;
static lv_color_t s_buf1[LV_DISP_BUF_SIZE];
static lv_color_t s_buf2[LV_DISP_BUF_SIZE];

static bool s_initialized = false;

/**
 * @brief 智能刷新策略：局刷或全刷（简化版）
 * @param force_full 是否强制全刷
 */
static void smart_refresh(bool force_full) {
    display_manager_refresh(force_full);
}

// LVGL 显示驱动的回调函数
static void disp_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
    int32_t width = lv_area_get_width(area);
    int32_t height = lv_area_get_height(area);

    // 调用底层的桥接函数
    display_manager_flush_lvgl(area->x1, area->y1, width, height, (const uint8_t*)color_p);

    // 通知LVGL刷新完成
    lv_disp_flush_ready(disp_drv);
}

extern "C" {

ui_result_t ui_manager_init() {
    if (s_initialized) {
        return UI_OK;
    }

    LOG_INFO("UI", "UI Manager initializing...");

    // 1. 初始化底层显示
    if (display_manager_init() != DISPLAY_OK) {
        LOG_ERROR("UI", "Display manager failed to init");
        return UI_ERROR_INIT_FAILED;
    }

    // 2. 初始化 LVGL
    lv_init();

    // 3. 初始化显示缓冲区
    lv_disp_draw_buf_init(&s_disp_buf, s_buf1, s_buf2, LV_DISP_BUF_SIZE);

    // 4. 创建并注册显示驱动
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 296;
    disp_drv.ver_res = 128;
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.draw_buf = &s_disp_buf;
    lv_disp_drv_register(&disp_drv);

    s_initialized = true;
    LOG_INFO("UI", "UI Manager initialized");
    return UI_OK;
}

void ui_manager_loop() {
    if (!s_initialized) {
        return;
    }
    // 驱动LVGL的内部任务
    lv_timer_handler();
}

void ui_manager_show_test_screen() {
    if (!s_initialized) {
        LOG_ERROR("UI", "UI Manager not initialized, cannot show test screen");
        return;
    }

    // 清理当前屏幕
    lv_obj_clean(lv_scr_act());

    // 创建一个标签控件
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "LVGL Test OK");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // 手动触发一次LVGL重绘
    lv_task_handler();
    
    // 刷新屏幕
    display_manager_refresh(true); // 全屏刷新
}

} // extern "C"

void ui_manager_show_shutdown_screen() {
    if (!s_initialized) {
        LOG_ERROR("UI", "UI Manager not initialized, cannot show shutdown screen");
        return;
    }

    LOG_INFO("UI", ">>> show_shutdown_screen START");

    lv_obj_clean(lv_scr_act());

    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "System is OFF.\nSafe to disconnect power.");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // 强制 LVGL 同步完成渲染
    lv_refr_now(NULL);

    // 刷新屏幕
    display_manager_refresh(true);

    LOG_INFO("UI", "<<< show_shutdown_screen END");
}

void ui_manager_show_run_dashboard(float humidity_pct, float threshold_pct,
                                    float battery_v, const char* last_water_time,
                                    const char* system_status) {
    if (!s_initialized) {
        LOG_ERROR("UI", "UI Manager not initialized, cannot show RUN dashboard");
        return;
    }

    lv_obj_clean(lv_scr_act());
    lv_obj_t * scr = lv_scr_act();

    // Humidity
    lv_obj_t * label_humidity = lv_label_create(scr);
    char buf_humidity[32];
    snprintf(buf_humidity, sizeof(buf_humidity), "Humidity: %.1f%%", humidity_pct);
    lv_label_set_text(label_humidity, buf_humidity);
    lv_obj_align(label_humidity, LV_ALIGN_TOP_LEFT, 5, 5);

    // Threshold
    lv_obj_t * label_threshold = lv_label_create(scr);
    char buf_threshold[32];
    snprintf(buf_threshold, sizeof(buf_threshold), "Threshold: %.1f%%", threshold_pct);
    lv_label_set_text(label_threshold, buf_threshold);
    lv_obj_align(label_threshold, LV_ALIGN_TOP_LEFT, 5, 25);

    // Battery
    lv_obj_t * label_battery = lv_label_create(scr);
    char buf_battery[32];
    snprintf(buf_battery, sizeof(buf_battery), "Battery: %.2fV", battery_v);
    lv_label_set_text(label_battery, buf_battery);
    lv_obj_align(label_battery, LV_ALIGN_TOP_LEFT, 5, 45);

    // Last Water
    lv_obj_t * label_last_water = lv_label_create(scr);
    char buf_last_water[48];
    snprintf(buf_last_water, sizeof(buf_last_water), "Last Water: %s", last_water_time);
    lv_label_set_text(label_last_water, buf_last_water);
    lv_obj_align(label_last_water, LV_ALIGN_TOP_LEFT, 5, 65);

    // System Status
    lv_obj_t * label_status = lv_label_create(scr);
    char buf_status[64];
    snprintf(buf_status, sizeof(buf_status), "Status: %s", system_status);
    lv_label_set_text(label_status, buf_status);
    lv_obj_align(label_status, LV_ALIGN_TOP_LEFT, 5, 85);

    LOG_DEBUG("UI", "RUN dashboard objects created");
}

// ===== Interactive模式UI组件实现 (P2阶段) =====

void ui_manager_show_menu(const char* title, const char* items[],
                          uint8_t item_count, uint8_t selected_index,
                          const char* hint) {
    if (!s_initialized) {
        LOG_ERROR("UI", "UI Manager not initialized");
        return;
    }

    lv_obj_clean(lv_scr_act());
    lv_obj_t * scr = lv_scr_act();

    // 标题
    lv_obj_t * label_title = lv_label_create(scr);
    lv_label_set_text(label_title, title);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 2);

    // 菜单项（最多5行）
    int16_t y_offset = 22;
    for (uint8_t i = 0; i < item_count && i < 5; i++) {
        lv_obj_t * label_item = lv_label_create(scr);
        char buf[80];
        if (i == selected_index) {
            snprintf(buf, sizeof(buf), "> %s", items[i]);
        } else {
            snprintf(buf, sizeof(buf), "  %s", items[i]);
        }
        lv_label_set_text(label_item, buf);
        lv_obj_align(label_item, LV_ALIGN_TOP_LEFT, 5, y_offset);
        y_offset += 20;
    }

    lv_refr_now(NULL);
    smart_refresh(false);  // 菜单切换，强制全刷
}

void ui_manager_show_status(float humidity_pct, float battery_v,
                            float threshold_pct, uint8_t power,
                            uint32_t duration_ms, uint32_t interval_s,
                            bool wifi_connected, bool time_synced) {
    if (!s_initialized) {
        LOG_ERROR("UI", "UI Manager not initialized");
        return;
    }

    lv_obj_clean(lv_scr_act());
    lv_obj_t * scr = lv_scr_act();

    // 标题
    lv_obj_t * label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "System Status");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 2);

    // 传感器数据
    lv_obj_t * label_sensor = lv_label_create(scr);
    char buf_sensor[64];
    snprintf(buf_sensor, sizeof(buf_sensor), "Humid: %.0f%%  Bat: %.2fV", humidity_pct, battery_v);
    lv_label_set_text(label_sensor, buf_sensor);
    lv_obj_align(label_sensor, LV_ALIGN_TOP_LEFT, 5, 25);

    // 配置参数
    lv_obj_t * label_config = lv_label_create(scr);
    char buf_config[80];
    snprintf(buf_config, sizeof(buf_config), "Thresh: %.0f%%  Pwr: %d  Dur: %lums",
             threshold_pct, power, (unsigned long)duration_ms);
    lv_label_set_text(label_config, buf_config);
    lv_obj_align(label_config, LV_ALIGN_TOP_LEFT, 5, 48);

    lv_obj_t * label_interval = lv_label_create(scr);
    char buf_interval[48];
    snprintf(buf_interval, sizeof(buf_interval), "Min Interval: %lus", (unsigned long)interval_s);
    lv_label_set_text(label_interval, buf_interval);
    lv_obj_align(label_interval, LV_ALIGN_TOP_LEFT, 5, 71);

    // 网络状态
    lv_obj_t * label_network = lv_label_create(scr);
    char buf_network[64];
    snprintf(buf_network, sizeof(buf_network), "WiFi: %s  Time: %s",
             wifi_connected ? "Connected" : "Disconnected",
             time_synced ? "Synced" : "Not synced");
    lv_label_set_text(label_network, buf_network);
    lv_obj_align(label_network, LV_ALIGN_TOP_LEFT, 5, 94);

    lv_refr_now(NULL);
    smart_refresh(false);
}

void ui_manager_show_setting_edit(const char* setting_name,
                                   int32_t current_value, int32_t preview_value,
                                   int32_t min_value, int32_t max_value,
                                   const char* unit) {
    if (!s_initialized) {
        LOG_ERROR("UI", "UI Manager not initialized");
        return;
    }

    lv_obj_clean(lv_scr_act());
    lv_obj_t * scr = lv_scr_act();

    // 设置项名称
    lv_obj_t * label_name = lv_label_create(scr);
    lv_label_set_text(label_name, setting_name);
    lv_obj_align(label_name, LV_ALIGN_TOP_MID, 0, 20);

    // 当前值
    lv_obj_t * label_current = lv_label_create(scr);
    char buf_current[64];
    snprintf(buf_current, sizeof(buf_current), "Current: %ld %s", (long)current_value, unit);
    lv_label_set_text(label_current, buf_current);
    lv_obj_align(label_current, LV_ALIGN_TOP_LEFT, 5, 45);

    // 预览值（高亮）
    lv_obj_t * label_preview = lv_label_create(scr);
    char buf_preview[64];
    snprintf(buf_preview, sizeof(buf_preview), "> Preview: %ld %s", (long)preview_value, unit);
    lv_label_set_text(label_preview, buf_preview);
    lv_obj_align(label_preview, LV_ALIGN_TOP_LEFT, 5, 68);

    // 范围
    lv_obj_t * label_range = lv_label_create(scr);
    char buf_range[64];
    snprintf(buf_range, sizeof(buf_range), "Range: %ld ~ %ld", (long)min_value, (long)max_value);
    lv_label_set_text(label_range, buf_range);
    lv_obj_align(label_range, LV_ALIGN_TOP_LEFT, 5, 91);

    lv_refr_now(NULL);
    smart_refresh(false);  // 编辑值频繁变化，使用局刷
}

void ui_manager_show_watering_confirm(uint8_t power, uint32_t duration_ms,
                                       float humidity_before) {
    if (!s_initialized) {
        LOG_ERROR("UI", "UI Manager not initialized");
        return;
    }

    lv_obj_clean(lv_scr_act());
    lv_obj_t * scr = lv_scr_act();

    // 标题
    lv_obj_t * label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "Water Now");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 20);

    // 当前湿度
    lv_obj_t * label_humidity = lv_label_create(scr);
    char buf_humidity[48];
    snprintf(buf_humidity, sizeof(buf_humidity), "Current: %.0f%%", humidity_before);
    lv_label_set_text(label_humidity, buf_humidity);
    lv_obj_align(label_humidity, LV_ALIGN_CENTER, 0, -10);

    // 浇水参数
    lv_obj_t * label_params = lv_label_create(scr);
    char buf_params[64];
    snprintf(buf_params, sizeof(buf_params), "Power: %d  Duration: %lums", power, (unsigned long)duration_ms);
    lv_label_set_text(label_params, buf_params);
    lv_obj_align(label_params, LV_ALIGN_CENTER, 0, 15);

    lv_refr_now(NULL);
    smart_refresh(false);
}

void ui_manager_show_watering_progress(uint32_t elapsed_ms, uint32_t total_ms,
                                        float humidity_before) {
    if (!s_initialized) {
        LOG_ERROR("UI", "UI Manager not initialized");
        return;
    }

    lv_obj_clean(lv_scr_act());
    lv_obj_t * scr = lv_scr_act();

    // 标题
    lv_obj_t * label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "Watering...");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);

    // 浇水前湿度
    lv_obj_t * label_humidity = lv_label_create(scr);
    char buf_humidity[48];
    snprintf(buf_humidity, sizeof(buf_humidity), "Before: %.0f%%", humidity_before);
    lv_label_set_text(label_humidity, buf_humidity);
    lv_obj_align(label_humidity, LV_ALIGN_CENTER, 0, -5);

    // 进度
    lv_obj_t * label_progress = lv_label_create(scr);
    char buf_progress[64];
    uint8_t pct = (uint8_t)((elapsed_ms * 100UL) / total_ms);
    if (pct > 100) pct = 100;
    snprintf(buf_progress, sizeof(buf_progress), "Progress: %d%% (%lu/%lums)",
             pct, (unsigned long)elapsed_ms, (unsigned long)total_ms);
    lv_label_set_text(label_progress, buf_progress);
    lv_obj_align(label_progress, LV_ALIGN_CENTER, 0, 20);

    lv_refr_now(NULL);
    smart_refresh(false);  // 进度频繁更新，使用局刷
}

void ui_manager_show_watering_result(float humidity_before, float humidity_after) {
    if (!s_initialized) {
        LOG_ERROR("UI", "UI Manager not initialized");
        return;
    }

    lv_obj_clean(lv_scr_act());
    lv_obj_t * scr = lv_scr_act();

    // 标题
    lv_obj_t * label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "Watering Complete");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 20);

    // 浇水前
    lv_obj_t * label_before = lv_label_create(scr);
    char buf_before[48];
    snprintf(buf_before, sizeof(buf_before), "Before: %.0f%%", humidity_before);
    lv_label_set_text(label_before, buf_before);
    lv_obj_align(label_before, LV_ALIGN_CENTER, 0, -10);

    // 浇水后
    lv_obj_t * label_after = lv_label_create(scr);
    char buf_after[48];
    snprintf(buf_after, sizeof(buf_after), "After: %.0f%%", humidity_after);
    lv_label_set_text(label_after, buf_after);
    lv_obj_align(label_after, LV_ALIGN_CENTER, 0, 15);

    lv_refr_now(NULL);
    smart_refresh(false);
}

void ui_manager_show_chat_screen(const char* plant_message,
                                  const char* options[], uint8_t option_count,
                                  uint8_t selected_index) {
    if (!s_initialized) {
        LOG_ERROR("UI", "UI Manager not initialized");
        return;
    }

    lv_obj_clean(lv_scr_act());
    lv_obj_t * scr = lv_scr_act();

    // 植物消息（截断为2行）
    lv_obj_t * label_message = lv_label_create(scr);
    lv_label_set_long_mode(label_message, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label_message, 286);
    lv_label_set_text(label_message, plant_message);
    lv_obj_align(label_message, LV_ALIGN_TOP_LEFT, 5, 22);

    // 选项（最多4个，从第55行开始）
    int16_t y_offset = 55;
    for (uint8_t i = 0; i < option_count && i < 4; i++) {
        lv_obj_t * label_option = lv_label_create(scr);
        char buf[68];
        if (i == selected_index) {
            snprintf(buf, sizeof(buf), "> %s", options[i]);
        } else {
            snprintf(buf, sizeof(buf), "  %s", options[i]);
        }
        lv_label_set_text(label_option, buf);
        lv_obj_align(label_option, LV_ALIGN_TOP_LEFT, 5, y_offset);
        y_offset += 18;
    }

    lv_refr_now(NULL);
    smart_refresh(false);  // 聊天选项移动，使用局刷
}

void ui_manager_show_loading(const char* message) {
    if (!s_initialized) {
        LOG_ERROR("UI", "UI Manager not initialized");
        return;
    }

    lv_obj_clean(lv_scr_act());
    lv_obj_t * scr = lv_scr_act();

    lv_obj_t * label = lv_label_create(scr);
    lv_label_set_text(label, message);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_refr_now(NULL);
    smart_refresh(false);
}

void ui_manager_show_error(const char* error_message) {
    if (!s_initialized) {
        LOG_ERROR("UI", "UI Manager not initialized");
        return;
    }

    lv_obj_clean(lv_scr_act());
    lv_obj_t * scr = lv_scr_act();

    // 错误标题
    lv_obj_t * label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "Error");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 20);

    // 错误消息
    lv_obj_t * label_msg = lv_label_create(scr);
    lv_label_set_long_mode(label_msg, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label_msg, 286);
    lv_label_set_text(label_msg, error_message);
    lv_obj_align(label_msg, LV_ALIGN_CENTER, 0, 10);

    lv_refr_now(NULL);
    smart_refresh(false);
}

void ui_manager_trigger_full_refresh() {
    if (!s_initialized) {
        LOG_ERROR("UI", "UI Manager not initialized");
        return;
    }
    LOG_INFO("UI", "Manual full refresh triggered");
    display_manager_refresh(true);
}