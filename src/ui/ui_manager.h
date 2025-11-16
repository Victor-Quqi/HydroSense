/**
 * @file ui_manager.h
 * @brief UI逻辑和状态管理器
 * @details 负责初始化和管理LVGL图形库，处理用户输入，并协调UI元素的显示。
 */
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

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


#ifdef __cplusplus
} // extern "C"
#endif

#endif // UI_MANAGER_H