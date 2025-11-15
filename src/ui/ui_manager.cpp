/**
 * @file ui_manager.cpp
 * @brief UI管理器实现
 */
#include "ui_manager.h"
#include "display_manager.h"
#include "managers/log_manager.h"
#include <lvgl.h>

// LVGL 显示缓冲区
// 墨水屏尺寸: 296x128. 缓冲区可以设置为屏幕大小的1/10，以平衡内存和性能
#define LV_DISP_BUF_SIZE (296 * 128 / 10)
static lv_disp_draw_buf_t s_disp_buf;
static lv_color_t s_buf1[LV_DISP_BUF_SIZE];
static lv_color_t s_buf2[LV_DISP_BUF_SIZE];

static bool s_initialized = false;

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