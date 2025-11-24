/**
 * @file ui_manager.h
 * @brief UI逻辑和状态管理器
 * @details 负责初始化和管理LVGL图形库，处理用户输入，并协调UI元素的显示。
 */
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UI管理器操作结果
 */
typedef enum {
    UI_OK = 0,              ///< 操作成功
    UI_ERROR_INIT_FAILED    ///< 初始化失败
} ui_result_t;

/**
 * @brief 初始化UI管理器
 * @details
 *  - 初始化底层的 display_manager
 *  - 初始化 LVGL 库
 *  - 注册显示驱动和输入设备驱动
 */
ui_result_t ui_manager_init();

/**
 * @brief UI管理器的主循环函数
 * @details 必须在主循环中周期性调用，以驱动LVGL的定时器和任务处理。
 */
void ui_manager_loop();

/**
 * @brief 显示一个LVGL测试界面
 * @details 用于验证LVGL是否正确集成并工作。
 */
void ui_manager_show_test_screen();

/**
 * @brief 显示关机界面
 * @details 用于在进入OFF模式时调用
 */
void ui_manager_show_shutdown_screen();

/**
 * @brief 显示RUN模式的状态仪表盘
 * @details 在墨水屏上显示系统核心状态信息
 * @param humidity_pct 当前土壤湿度百分比
 * @param threshold_pct 浇水阈值百分比
 * @param battery_v 当前电池电压 (V)
 * @param last_water_time 上次浇水时间字符串 (如 "2m ago" 或 "N/A")
 * @param system_status 系统状态字符串 (如 "Watering..." 或 "Sleeping...")
 */
void ui_manager_show_run_dashboard(float humidity_pct, float threshold_pct,
                                    float battery_v, const char* last_water_time,
                                    const char* system_status);

// ===== Interactive模式UI组件 (P2阶段) =====

/**
 * @brief 显示通用菜单
 * @param title 菜单标题
 * @param items 菜单项数组
 * @param item_count 菜单项数量
 * @param selected_index 当前选中的索引
 * @param hint 左上角提示文字（如"双击返回"）
 */
void ui_manager_show_menu(const char* title, const char* items[],
                          uint8_t item_count, uint8_t selected_index,
                          const char* hint);

/**
 * @brief 显示系统状态界面
 * @param humidity_pct 当前湿度百分比
 * @param battery_v 电池电压
 * @param threshold_pct 阈值百分比
 * @param power 浇水功率
 * @param duration_ms 浇水时长
 * @param interval_s 最短间隔
 * @param wifi_connected WiFi是否连接
 * @param time_synced 时间是否同步
 */
void ui_manager_show_status(float humidity_pct, float battery_v,
                            float threshold_pct, uint8_t power,
                            uint32_t duration_ms, uint32_t interval_s,
                            bool wifi_connected, bool time_synced);

/**
 * @brief 显示设置编辑界面
 * @param setting_name 设置项名称
 * @param current_value 当前值
 * @param preview_value 预览值
 * @param min_value 最小值
 * @param max_value 最大值
 * @param unit 单位字符串（如"ADC"、"ms"）
 */
void ui_manager_show_setting_edit(const char* setting_name,
                                   int32_t current_value, int32_t preview_value,
                                   int32_t min_value, int32_t max_value,
                                   const char* unit);

/**
 * @brief 显示浇水确认界面
 * @param power 功率
 * @param duration_ms 时长
 * @param humidity_before 浇水前湿度
 */
void ui_manager_show_watering_confirm(uint8_t power, uint32_t duration_ms,
                                       float humidity_before);

/**
 * @brief 显示浇水进度界面
 * @param elapsed_ms 已经过时间
 * @param total_ms 总时长
 * @param humidity_before 浇水前湿度
 */
void ui_manager_show_watering_progress(uint32_t elapsed_ms, uint32_t total_ms,
                                        float humidity_before);

/**
 * @brief 重置浇水进度界面状态
 * @details 在退出浇水界面时调用，重置静态对象指针，下次进入时重新创建UI
 */
void ui_manager_reset_watering_progress();

/**
 * @brief 显示浇水结果界面
 * @param humidity_before 浇水前湿度
 * @param humidity_after 浇水后湿度
 */
void ui_manager_show_watering_result(float humidity_before, float humidity_after);

/**
 * @brief 显示聊天界面
 * @param plant_message 植物的回复消息
 * @param options 选项数组（最多4个）
 * @param option_count 选项数量
 * @param selected_index 当前选中的选项索引
 */
void ui_manager_show_chat_screen(const char* plant_message,
                                  const char* options[], uint8_t option_count,
                                  uint8_t selected_index);

/**
 * @brief 显示加载提示
 * @param message 提示消息（如"思考中..."）
 */
void ui_manager_show_loading(const char* message);

/**
 * @brief 显示错误提示
 * @param error_message 错误消息
 */
void ui_manager_show_error(const char* error_message);

/**
 * @brief 手动触发全屏刷新
 * @details 用于需要清除残影或完整刷新屏幕时调用
 */
void ui_manager_trigger_full_refresh();


#ifdef __cplusplus
} // extern "C"
#endif

#endif // UI_MANAGER_H