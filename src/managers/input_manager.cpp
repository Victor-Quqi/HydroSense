/**
 * @file input_manager.cpp
 * @brief 物理输入管理器实现
 */

#include "input_manager.h"
#include "hal/hal_config.h"
#include "hal/hal_gpio.h"
#include "managers/log_manager.h"
#include <Arduino.h>

// --- 旋转编码器状态变量 ---
static int last_encoder_a_state;
static int last_encoder_b_state;
static unsigned long last_debounce_time = 0;
static const unsigned long debounce_delay = 50; // ms

// --- 按键状态变量 ---
static int last_button_state = HIGH; // 用于检测抖动
static int button_stable_state = HIGH; // 记录稳定的按键状态


void input_manager_init() {
    // 模式开关引脚初始化为上拉输入
    // 当开关拨到对应位置时，会将引脚拉低
    hal_gpio_pin_mode(PIN_MODE_SWITCH_A, INPUT_PULLUP);
    hal_gpio_pin_mode(PIN_MODE_SWITCH_B, INPUT_PULLUP);

    // 编码器引脚初始化
    hal_gpio_pin_mode(PIN_ENCODER_A, INPUT_PULLUP);
    hal_gpio_pin_mode(PIN_ENCODER_B, INPUT_PULLUP);
    hal_gpio_pin_mode(PIN_ENCODER_SW, INPUT_PULLUP);

    // 初始化编码器初始状态
    last_encoder_a_state = digitalRead(PIN_ENCODER_A);
    last_encoder_b_state = digitalRead(PIN_ENCODER_B);
}

system_mode_t input_manager_get_mode() {
    // 读取两个开关引脚的电平
    // 开关公共端接地，因此拨到某个位置时，对应引脚为LOW
    bool state_a = (digitalRead(PIN_MODE_SWITCH_A) == LOW);
    bool state_b = (digitalRead(PIN_MODE_SWITCH_B) == LOW);

    // 根据双掷开关的组合状态判断模式
    // OFF: A=0, B=0 (开关在中间，两边都不通)
    // RUN: A=1, B=0 (开关拨到A)
    // INTERACTIVE: A=0, B=1 (开关拨到B)
    if (state_a && !state_b) {
        return SYSTEM_MODE_RUN;
    } else if (!state_a && state_b) {
        return SYSTEM_MODE_INTERACTIVE;
    } else if (!state_a && !state_b) {
        return SYSTEM_MODE_OFF;
    }

    // 其他组合为未知状态
    return SYSTEM_MODE_UNKNOWN;
}

void input_manager_loop() {
    // --- 编码器旋转处理 ---
    int encoder_a_state = digitalRead(PIN_ENCODER_A);
    int encoder_b_state = digitalRead(PIN_ENCODER_B);

    if ((encoder_a_state != last_encoder_a_state) || (encoder_b_state != last_encoder_b_state)) {
        if (encoder_a_state == LOW && last_encoder_a_state == HIGH) {
            if (encoder_b_state == LOW) {
                LOG_DEBUG("Input", "Encoder rotated: CCW"); // 逆时针
            } else {
                LOG_DEBUG("Input", "Encoder rotated: CW");  // 顺时针
            }
        }
        last_encoder_a_state = encoder_a_state;
        last_encoder_b_state = encoder_b_state;
    }

    // --- 编码器按键处理 (带消抖) ---
    int reading = digitalRead(PIN_ENCODER_SW);

    // 如果检测到电平变化，重置消抖计时器
    if (reading != last_button_state) {
        last_debounce_time = millis();
    }

    // 如果距离上次电平变化已经超过了消抖延迟
    if ((millis() - last_debounce_time) > debounce_delay) {
        // 并且当前的稳定读数与之前记录的稳定状态不同
        if (reading != button_stable_state) {
            button_stable_state = reading;
            // 如果这个新的稳定状态是“按下”
            if (button_stable_state == LOW) {
                LOG_DEBUG("Input", "Encoder button clicked.");
            }
        }
    }

    // 始终更新上一次的读数，用于检测下一次变化
    last_button_state = reading;
}