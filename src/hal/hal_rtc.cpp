/**
 * @file hal_rtc.cpp
 * @brief RTC和深度睡眠硬件抽象实现
 */
#include "hal_rtc.h"
#include "hal_config.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"

void hal_rtc_init() {
    // 目前无需特殊初始化
}

void hal_rtc_enter_deep_sleep() {
    // 新模式定义:
    // OFF:         A=LOW,  B=HIGH
    // INTERACTIVE: A=HIGH, B=HIGH
    // RUN:         A=HIGH, B=LOW
    //
    // 从OFF模式离开，意味着A引脚会从LOW变为HIGH。
    // 因此，正确的唤醒策略是监听A引脚的上升沿，即高电平唤醒。
    esp_sleep_enable_ext1_wakeup((1ULL << PIN_MODE_SWITCH_A), ESP_EXT1_WAKEUP_ANY_HIGH);

    // (可选但推荐) 禁用所有非必要的 RTC 外设，进一步降低功耗
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);

    // 进入深度睡眠
    esp_deep_sleep_start();
}