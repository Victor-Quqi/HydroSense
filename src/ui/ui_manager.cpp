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
 * @brief 将土壤湿度ADC值转换为百分比
 * @details ADC值越高表示越干燥，需要反向映射：
 *          - 1000 ADC (全湿) → 100%
 *          - 2600 ADC (干燥) → 0%
 * @param adc_value 原始ADC读数 (0-4095)
 * @return 湿度百分比 (0.0-100.0)
 */
static float humidity_adc_to_percent(uint16_t adc_value) {
    if (adc_value >= SOIL_ADC_DRY) return 0.0f;
    if (adc_value <= SOIL_ADC_WET) return 100.0f;
    return 100.0f - ((float)(adc_value - SOIL_ADC_WET) * 100.0f) / (SOIL_ADC_DRY - SOIL_ADC_WET);
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