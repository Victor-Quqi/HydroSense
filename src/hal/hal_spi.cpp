/**
 * @file hal_spi.cpp
 * @brief SPI通信硬件抽象层实现
 */
#include "hal_spi.h"
#include "hal_config.h"
#include <SPI.h>

// 使用 ESP32-S3 的 HSPI (SPI3) 作为显示总线
static SPIClass s_displaySPI(HSPI);
static bool s_spi_inited = false;

void hal_spi_init_display() {
    if (s_spi_inited) return;

    // 对于墨水屏，MISO 不使用，传 -1
    // 参数顺序: SCK, MISO, MOSI, SS
    s_displaySPI.begin(PIN_DISPLAY_SCK, -1, PIN_DISPLAY_MOSI, PIN_DISPLAY_CS);
    s_spi_inited = true;
}

SPIClass* hal_spi_get_display_bus() {
    return &s_displaySPI;
}