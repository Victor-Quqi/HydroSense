/**
 * @file display_manager.cpp
 * @brief 显示管理器实现
 */
#include "display_manager.h"

#include <SPI.h>
#include <GxEPD2_BW.h>

#include "hal/hal_config.h"
#include "hal/hal_spi.h"
#include "managers/power_manager.h"
#include "managers/log_manager.h"

// 使用 Waveshare 2.9" Rev2.1 (SSD1680, GDEM029T94) 对应的 GxEPD2 驱动类
static GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> s_display(
    GxEPD2_290_T94_V2(
        PIN_DISPLAY_CS,
        PIN_DISPLAY_DC,
        PIN_DISPLAY_RST,
        PIN_DISPLAY_BUSY
    )
);

static bool s_initialized = false;

extern "C" {

bool display_manager_is_initialized() {
    return s_initialized;
}

display_result_t display_manager_init() {
    if (s_initialized) {
        return DISPLAY_OK;
    }

    // 1) 打开墨水屏电源
    power_result_t pwr = power_screen_enable(true);
    if (pwr != POWER_OK) {
        LOG_ERROR("Display", "Failed to enable screen power: %d", pwr);
        return DISPLAY_ERROR_POWER_FAILED;
    }

    // 2) 初始化 SPI 总线 (HSPI/SPI3)
    hal_spi_init_display();
    SPIClass* spi = hal_spi_get_display_bus();
    if (spi == nullptr) {
        LOG_ERROR("Display", "SPI bus is null");
        return DISPLAY_ERROR_HW_FAILED;
    }

    // 3) 初始化 GxEPD2 显示 (使用全帧缓冲模型)
    // 参数: serial_bitrate, init_serial, rst_duration_ms, pulldown, spi, spi_settings
    s_display.init(
        115200,
        true,
        10,
        false,
        *spi,
        SPISettings(4000000, MSBFIRST, SPI_MODE0)
    );

    // 基本渲染设置
    s_display.setRotation(1);         // 横屏
    s_display.setTextColor(GxEPD_BLACK);

    // 首次全屏清屏，建立干净基线，避免局部刷新伪影
    s_display.fillScreen(GxEPD_WHITE);
    s_display.display(false); // false = 全局刷新

    s_initialized = true;
    LOG_INFO("Display", "Display initialized");
    return DISPLAY_OK;
}

display_result_t display_manager_clear() {
    if (!s_initialized) return DISPLAY_ERROR_NOT_INIT;
    s_display.fillScreen(GxEPD_WHITE);
    return DISPLAY_OK;
}

display_result_t display_manager_draw_text(const char* text, int16_t x, int16_t y) {
    if (!s_initialized) return DISPLAY_ERROR_NOT_INIT;
    if (text == nullptr) return DISPLAY_ERROR_INVALID_PARAM;

    s_display.setCursor(x, y);
    s_display.print(text); // 注意: 默认字体为单字节；中文需自定义字库或位图方式
    return DISPLAY_OK;
}

display_result_t display_manager_refresh(bool full_refresh) {
    if (!s_initialized) return DISPLAY_ERROR_NOT_INIT;

    // GxEPD2: display.display(false) = 全局刷新; display.display(true) = 局部刷新
    s_display.display(full_refresh ? false : true);
    return DISPLAY_OK;
}

display_result_t display_manager_sleep() {
    if (!s_initialized) return DISPLAY_ERROR_NOT_INIT;

    s_display.hibernate();
    // 选择性关闭电源以实现零功耗
    power_screen_enable(false);
    s_initialized = false;
    LOG_INFO("Display", "Display hibernated and power off");
    return DISPLAY_OK;
}

} // extern "C"