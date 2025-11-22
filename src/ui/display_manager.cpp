/**
 * @file display_manager.cpp
 * @brief 显示管理器实现
 */
#include "display_manager.h"

#include <SPI.h>
#include <GxEPD2_BW.h>

#include "hal/hal_config.h"
#include "hal/hal_spi.h"
#include "hal/hal_gpio.h" // 引入GPIO HAL以设置引脚模式
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
    // 参数: serial_bitrate, initial, rst_duration_ms, pulldown, spi, spi_settings
    // initial=false: 用于深度睡眠唤醒后重新初始化（如果显示电源保持供电）
    // 避免 _initial_refresh 标志导致 partial refresh 被强制转换为 full refresh
    s_display.init(
        115200,
        false,  // 修改：initial=false 避免双重刷新问题
        10,
        false,
        *spi,
        SPISettings(4000000, MSBFIRST, SPI_MODE0)
    );

    // 基本渲染设置
    s_display.setRotation(1);         // 横屏
    s_display.setTextColor(GxEPD_BLACK);

    // 首次全屏清屏，建立干净基线，避免局部刷新伪影
    // s_display.fillScreen(GxEPD_WHITE);
    // s_display.display(false); // 注释掉：LVGL会立即接管并刷新，此步骤冗余

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

    LOG_INFO("Display", ">>> refresh START (full=%d)", full_refresh);

    // GxEPD2: display.display(false) = 全局刷新; display.display(true) = 局部刷新
    s_display.display(full_refresh ? false : true);

    LOG_INFO("Display", "<<< refresh END");
    return DISPLAY_OK;
}

display_result_t display_manager_sleep() {
    if (!s_initialized) return DISPLAY_ERROR_NOT_INIT;

    s_display.hibernate();
    // 选择性关闭电源以实现零功耗
    power_screen_enable(false);

    // 关键修复：将所有SPI引脚设置为输入模式，防止寄生供电
    hal_gpio_pin_mode(PIN_DISPLAY_SCK, INPUT);
    hal_gpio_pin_mode(PIN_DISPLAY_MOSI, INPUT);
    hal_gpio_pin_mode(PIN_DISPLAY_CS, INPUT);
    hal_gpio_pin_mode(PIN_DISPLAY_DC, INPUT);
    hal_gpio_pin_mode(PIN_DISPLAY_RST, INPUT);
    hal_gpio_pin_mode(PIN_DISPLAY_BUSY, INPUT);

    s_initialized = false;
    LOG_INFO("Display", "Display hibernated and power off");
    return DISPLAY_OK;
}

} // extern "C"
void display_manager_flush_lvgl(int16_t x, int16_t y, int16_t width, int16_t height, const uint8_t* color_p) {
    if (!s_initialized) {
        // 在调用刷新之前，LVGL会确保显示器已初始化
        return;
    }

    // 逐像素绘制，这是最通用的LVGL桥接方法
    int16_t i, j;
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            if (*color_p) {
                s_display.drawPixel(x + i, y + j, GxEPD_BLACK);
            } else {
                s_display.drawPixel(x + i, y + j, GxEPD_WHITE);
            }
            color_p++;
        }
    }
}