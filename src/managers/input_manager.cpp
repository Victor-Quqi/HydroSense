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
static uint8_t last_encoder_state = 0;
static int encoder_counter = 0;
#define ENCODER_THRESHOLD 4 // 可调参数：累积多少步才算一次有效旋转
static unsigned long last_debounce_time = 0;
static const unsigned long debounce_delay = 50; // ms

// --- 编码器增量事件（消费型API用） ---
static int8_t encoder_delta = 0; // 旋转增量：+1=顺时针，-1=逆时针，0=无旋转

// --- 按键状态变量 ---
static int last_button_state = HIGH; // 用于检测抖动
static int button_stable_state = HIGH; // 记录稳定的按键状态

// --- 按键事件（消费型API用） ---
static bool button_clicked_flag = false; // 单击事件标志
static bool button_double_clicked_flag = false; // 双击事件标志
static unsigned long last_click_time = 0; // 上次单击时间
static const unsigned long double_click_interval = 233; // 双击间隔阈值（ms）
static bool button_pending = false; // 等待确认是单击还是双击的pending状态


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
    last_encoder_state = (digitalRead(PIN_ENCODER_A) << 1) | digitalRead(PIN_ENCODER_B);
}

system_mode_t input_manager_get_mode() {
    // 读取两个开关引脚的电平
    // 开关公共端接地，因此拨到某个位置时，对应引脚为LOW
    bool state_a_low = (digitalRead(PIN_MODE_SWITCH_A) == LOW);
    bool state_b_low = (digitalRead(PIN_MODE_SWITCH_B) == LOW);

    // 根据新的模式定义进行判断
    // OFF: A接地 (A=LOW, B=HIGH)
    // RUN: B接地 (A=HIGH, B=LOW)
    // INTERACTIVE: 中间 (A=HIGH, B=HIGH)
    if (state_a_low) {
        return SYSTEM_MODE_OFF;
    } else if (state_b_low) {
        return SYSTEM_MODE_RUN;
    } else {
        return SYSTEM_MODE_INTERACTIVE;
    }
}

void input_manager_loop() {
    // --- 编码器旋转处理 (积分-阈值算法) ---
    static const int8_t lookup_table[] = { 0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0 };

    uint8_t current_encoder_state = (digitalRead(PIN_ENCODER_A) << 1) | digitalRead(PIN_ENCODER_B);

    if (current_encoder_state != last_encoder_state) {
        int8_t direction = lookup_table[(last_encoder_state << 2) | current_encoder_state];
        encoder_counter += direction;

        if (encoder_counter >= ENCODER_THRESHOLD) {
            encoder_delta = 1; // 顺时针：设置增量为+1
            encoder_counter = 0;
        } else if (encoder_counter <= -ENCODER_THRESHOLD) {
            encoder_delta = -1; // 逆时针：设置增量为-1
            encoder_counter = 0;
        }

        last_encoder_state = current_encoder_state;
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
            // 如果这个新的稳定状态是"按下"
            if (button_stable_state == LOW) {
                unsigned long current_time = millis();
                // 检测双击：如果当前是pending状态且在双击间隔内
                if (button_pending && (current_time - last_click_time) < double_click_interval) {
                    button_double_clicked_flag = true; // 确认为双击
                    button_pending = false; // 清除pending状态
                    last_click_time = 0;
                } else {
                    // 第1次点击：进入pending状态，等待可能的第2次点击
                    button_pending = true;
                    last_click_time = current_time;
                }
            }
        }
    }

    // 检查pending超时：如果等待时间超过双击间隔，确认为单击
    if (button_pending && (millis() - last_click_time) >= double_click_interval) {
        button_clicked_flag = true; // 确认为单击
        button_pending = false; // 清除pending状态
    }

    // 始终更新上一次的读数，用于检测下一次变化
    last_button_state = reading;
}

int8_t input_manager_get_encoder_delta() {
    int8_t delta = encoder_delta;
    encoder_delta = 0; // 读取后清零（消费型）
    return delta;
}

bool input_manager_get_button_clicked() {
    if (button_clicked_flag) {
        button_clicked_flag = false; // 读取后清除标志（消费型）
        return true;
    }
    return false;
}

bool input_manager_get_button_double_clicked() {
    if (button_double_clicked_flag) {
        button_double_clicked_flag = false; // 读取后清除标志（消费型）
        return true;
    }
    return false;
}

void input_manager_clear_events() {
    encoder_delta = 0;
    button_clicked_flag = false;
    button_double_clicked_flag = false;
    button_pending = false; // 也清除pending状态
    last_click_time = 0;
    encoder_counter = 0; // 也清零编码器累积值
}