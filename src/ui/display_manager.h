/**
 * @file display_manager.h
 * @brief 显示渲染引擎
 * @details 负责墨水屏的底层驱动和图形绘制，作为UI管理器的渲染后端
 */
#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <stdint.h>

#ifdef ARDUINO
#include <Arduino.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 显示管理器操作结果
 */
typedef enum {
    DISPLAY_OK = 0,              ///< 操作成功
    DISPLAY_ERROR_NOT_INIT,      ///< 未初始化
    DISPLAY_ERROR_POWER_FAILED,  ///< 供电失败
    DISPLAY_ERROR_HW_FAILED,     ///< 硬件/驱动失败
    DISPLAY_ERROR_INVALID_PARAM  ///< 参数无效
} display_result_t;

/**
 * @brief 初始化显示管理器
 * @details
 *  - 打开墨水屏电源 (power_manager)
 *  - 初始化 SPI 总线 (hal_spi)
 *  - 初始化 GxEPD2 显示驱动
 *  - 执行一次全屏清屏建立干净基线
 */
display_result_t display_manager_init();

/**
 * @brief 查询显示是否已初始化
 */
bool display_manager_is_initialized();

/**
 * @brief 清空画面到白色（仅操作缓冲区，不立即刷新）
 */
display_result_t display_manager_clear();

/**
 * @brief 在缓冲区位置绘制文本（不立即刷新）
 * @param text 文本（UTF-8 中英文均可，使用默认字体）
 * @param x X坐标（像素）
 * @param y Y坐标（像素，基线）
 */
display_result_t display_manager_draw_text(const char* text, int16_t x, int16_t y);

/**
 * @brief 刷新显示
 * @param full_refresh true=全屏刷新; false=局部刷新（比较缓冲差异）
 */
display_result_t display_manager_refresh(bool full_refresh);

/**
 * @brief 将显示器置于深度睡眠以达成近零功耗
 */
display_result_t display_manager_sleep();
/**
 * @brief LVGL 'flush_cb' 桥接函数
 * @details 将LVGL渲染的缓冲区内容刷新到屏幕的指定区域。
 * @param x 区域左上角X坐标
 * @param y 区域左上角Y坐标
 * @param width 区域宽度
 * @param height 区域高度
 * @param color_p 指向颜色缓冲区的指针
 */
void display_manager_flush_lvgl(int16_t x, int16_t y, int16_t width, int16_t height, const uint8_t* color_p);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // DISPLAY_MANAGER_H