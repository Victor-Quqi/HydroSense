/**
 * @file hal_rtc.h
 * @brief RTC和深度睡眠硬件抽象
 */
#ifndef HAL_RTC_H
#define HAL_RTC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化RTC相关功能
 */
void hal_rtc_init();

/**
 * @brief 配置唤醒源并进入深度睡眠
 */
void hal_rtc_enter_deep_sleep();

#ifdef __cplusplus
}
#endif

#endif // HAL_RTC_H