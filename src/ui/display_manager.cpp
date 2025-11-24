/**
 * @file display_manager.cpp
 * @brief 显示管理器实现
 */
#include "display_manager.h"

#include <SPI.h>
#include <GxEPD2_BW.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

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

// FreeRTOS task for non-blocking display refresh
static TaskHandle_t s_refresh_task_handle = NULL;
static QueueHandle_t s_refresh_queue = NULL;
static SemaphoreHandle_t s_refresh_complete_semaphore = NULL;

typedef struct {
    bool full_refresh;
} refresh_request_t;

// Display refresh task function
static void display_refresh_task(void* parameter) {
    refresh_request_t request;

    while (true) {
        // Wait for refresh request from queue
        if (xQueueReceive(s_refresh_queue, &request, portMAX_DELAY) == pdTRUE) {
            if (s_initialized) {
                LOG_INFO("Display", ">>> refresh START (full=%d)", request.full_refresh);

                // This is the blocking operation, but it runs in dedicated task
                s_display.display(request.full_refresh ? false : true);

                LOG_INFO("Display", "<<< refresh END");

                // Signal completion if semaphore exists (for blocking refresh)
                if (s_refresh_complete_semaphore != NULL) {
                    xSemaphoreGive(s_refresh_complete_semaphore);
                }
            }
        }
    }
}

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

    // Create FreeRTOS queue for refresh requests (queue size: 10, 增加容量防止溢出)
    if (s_refresh_queue == NULL) {
        s_refresh_queue = xQueueCreate(10, sizeof(refresh_request_t));
        if (s_refresh_queue == NULL) {
            LOG_ERROR("Display", "Failed to create refresh queue");
            return DISPLAY_ERROR_HW_FAILED;
        }
    }

    // Create binary semaphore for blocking refresh synchronization
    if (s_refresh_complete_semaphore == NULL) {
        s_refresh_complete_semaphore = xSemaphoreCreateBinary();
        if (s_refresh_complete_semaphore == NULL) {
            LOG_ERROR("Display", "Failed to create refresh completion semaphore");
            vQueueDelete(s_refresh_queue);
            s_refresh_queue = NULL;
            return DISPLAY_ERROR_HW_FAILED;
        }
    }

    // Create FreeRTOS task for display refresh (priority: 1, stack: 4KB)
    if (s_refresh_task_handle == NULL) {
        BaseType_t task_created = xTaskCreate(
            display_refresh_task,
            "DisplayRefresh",
            4096,  // Stack size in words
            NULL,
            1,     // Priority
            &s_refresh_task_handle
        );

        if (task_created != pdPASS) {
            LOG_ERROR("Display", "Failed to create refresh task");
            vQueueDelete(s_refresh_queue);
            s_refresh_queue = NULL;
            if (s_refresh_complete_semaphore != NULL) {
                vSemaphoreDelete(s_refresh_complete_semaphore);
                s_refresh_complete_semaphore = NULL;
            }
            return DISPLAY_ERROR_HW_FAILED;
        }
    }

    s_initialized = true;
    LOG_INFO("Display", "Display initialized (non-blocking refresh mode)");
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
    if (s_refresh_queue == NULL) return DISPLAY_ERROR_HW_FAILED;

    // Non-blocking: send refresh request to queue
    refresh_request_t request = { .full_refresh = full_refresh };

    BaseType_t result = xQueueSend(s_refresh_queue, &request, 0);  // No wait
    if (result != pdTRUE) {
        LOG_WARN("Display", "Refresh queue full, skipping refresh request");
        return DISPLAY_ERROR_HW_FAILED;
    }

    // Return immediately without blocking
    return DISPLAY_OK;
}

display_result_t display_manager_refresh_blocking(bool full_refresh, uint32_t timeout_ms) {
    if (!s_initialized) return DISPLAY_ERROR_NOT_INIT;
    if (s_refresh_queue == NULL || s_refresh_complete_semaphore == NULL) {
        return DISPLAY_ERROR_HW_FAILED;
    }

    // Clear any stale semaphore state (non-blocking take)
    xSemaphoreTake(s_refresh_complete_semaphore, 0);

    // Send refresh request to queue
    refresh_request_t request = { .full_refresh = full_refresh };
    BaseType_t result = xQueueSend(s_refresh_queue, &request, pdMS_TO_TICKS(timeout_ms));
    if (result != pdTRUE) {
        LOG_ERROR("Display", "Failed to queue refresh request (queue full or timeout)");
        return DISPLAY_ERROR_HW_FAILED;
    }

    // Wait for refresh to complete
    TickType_t wait_ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    if (xSemaphoreTake(s_refresh_complete_semaphore, wait_ticks) != pdTRUE) {
        LOG_ERROR("Display", "Refresh timeout after %u ms", timeout_ms);
        return DISPLAY_ERROR_HW_FAILED;
    }

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

    // Clean up FreeRTOS resources
    if (s_refresh_task_handle != NULL) {
        vTaskDelete(s_refresh_task_handle);
        s_refresh_task_handle = NULL;
    }
    if (s_refresh_queue != NULL) {
        vQueueDelete(s_refresh_queue);
        s_refresh_queue = NULL;
    }
    if (s_refresh_complete_semaphore != NULL) {
        vSemaphoreDelete(s_refresh_complete_semaphore);
        s_refresh_complete_semaphore = NULL;
    }

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