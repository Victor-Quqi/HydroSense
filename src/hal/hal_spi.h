/**
 * @file hal_spi.h
 * @brief SPI通信硬件抽象层
 * @details 提供SPI通信的硬件抽象接口，主要用于墨水屏显示
 */
#ifndef HAL_SPI_H
#define HAL_SPI_H

#include <Arduino.h>
#include <SPI.h>

/**
 * @brief 初始化用于显示器的 SPI 总线
 * @details
 *  - 基于 ESP32-S3 的 HSPI (SPI3) 主机
 *  - 使用 hal_config.h 中定义的引脚
 *  - 必须在使用任何显示相关 API 前调用
 */
void hal_spi_init_display();

/**
 * @brief 获取显示器所用的 SPI 总线实例
 * @return SPIClass* 指向 SPI 实例的指针（不可为 nullptr）
 */
SPIClass* hal_spi_get_display_bus();

#endif // HAL_SPI_H