/**
 * @file hal_config.h
 * @brief 中心化硬件配置
 * @details
 *   该文件是硬件抽象层的基石。
 *   它将项目中所有硬件相关的引脚定义和关键参数集中于一处。
 */

#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

#include <Arduino.h>

// --- 1. 电源门控 (Power Gating) ---
constexpr bool POWER_ON = LOW;
constexpr bool POWER_OFF = HIGH;

#define PIN_POWER_GATE_PUMP    9   // 控制12V升压模块的电源
#define PIN_POWER_GATE_SENSOR  10  // 控制传感器的电源
#define PIN_POWER_GATE_DISPLAY 11  // 控制墨水屏的电源

// --- 2. 执行器控制 (Actuators) ---
#define PIN_ACTUATOR_PUMP      12  // 水泵驱动信号
// #define PIN_ACTUATOR_VALVE     13  // 电磁阀驱动信号 (原型中因供电不足, 暂时禁用)

// --- 3. 传感器与模拟输入 (Sensors & Analog) ---
#define PIN_SENSOR_HUMIDITY      4   // 土壤湿度传感器信号输入
#define PIN_SENSOR_BATTERY_ADC   7   // 电池电压监测ADC输入引脚
/**
 * @brief 电池电压检测所用的分压电阻比例。
 * 
 * V_actual = V_adc * VOLTAGE_DIVIDER_RATIO
 * RATIO = (R1 + R2) / R2
 * 
 * 根据实际使用的电阻计算：
 * R1 = 4.7 MΩ (4,700,000 Ω)
 * R2 = 0.33 MΩ (330,000 Ω)
 * RATIO = (4700000 + 330000) / 330000 ≈ 15.2424
 */
#define VOLTAGE_DIVIDER_RATIO  15.2424f
#define ADC_REFERENCE_VOLTAGE 4.50f

// --- 4. 用户输入 (User Inputs) ---
#define PIN_ENCODER_A          1
#define PIN_ENCODER_B          2
#define PIN_ENCODER_SW         21
#define PIN_MODE_SWITCH_A      5
#define PIN_MODE_SWITCH_B      6

// --- 5. 显示屏 SPI 接口 (Display SPI) ---
#define PIN_DISPLAY_SCK        40
#define PIN_DISPLAY_MOSI       41
#define PIN_DISPLAY_CS         42
#define PIN_DISPLAY_DC         38
#define PIN_DISPLAY_RST        14
#define PIN_DISPLAY_BUSY       15

#endif // HAL_CONFIG_H