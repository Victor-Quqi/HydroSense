/**
 * @file input_manager.cpp
 * @brief 物理输入管理器实现
 */

#include "input_manager.h"
#include "hal/hal_config.h"
#include "hal/hal_gpio.h"
#include "managers/log_manager.h"
#include "data/timing_constants.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// --- FreeRTOS任务句柄 ---
static TaskHandle_t s_encoder_task_handle = NULL;
static portMUX_TYPE s_encoder_mux = portMUX_INITIALIZER_UNLOCKED;

// --- 旋转编码器状态变量 ---
static volatile uint8_t last_encoder_state = 0;
static volatile int encoder_counter = 0;
static unsigned long last_debounce_time = 0;
static const unsigned long debounce_delay = 50; // ms

// --- 编码器增量事件（消费型API用） - 循环队列 ---
#define ENCODER_QUEUE_SIZE 16
static int8_t encoder_queue[ENCODER_QUEUE_SIZE];
static uint8_t encoder_queue_head = 0;
static uint8_t encoder_queue_tail = 0;
static uint8_t encoder_queue_count = 0;

// --- 按键状态变量 ---
static int last_button_state = HIGH; // 用于检测抖动
static int button_stable_state = HIGH; // 记录稳定的按键状态
static unsigned long button_press_start_time = 0; // 记录按下开始时间（用于长按检测）

// --- 按键事件（消费型API用） ---
static bool button_clicked_flag = false; // 单击事件标志
static bool button_double_clicked_flag = false; // 双击事件标志
static bool button_long_pressed_flag = false; // 长按事件标志
static unsigned long last_click_time = 0; // 上次单击时间
static bool button_pending = false; // 等待确认是单击还是双击的pending状态

// --- 队列操作函数 ---
static void encoder_enqueue(int8_t delta) {
    portENTER_CRITICAL(&s_encoder_mux);
    if (encoder_queue_count < ENCODER_QUEUE_SIZE) {
        encoder_queue[encoder_queue_head] = delta;
        encoder_queue_head = (encoder_queue_head + 1) % ENCODER_QUEUE_SIZE;
        encoder_queue_count++;
    }
    portEXIT_CRITICAL(&s_encoder_mux);
    // 队列满时，静默丢弃（避免日志干扰性能）
}

static int8_t encoder_dequeue() {
    portENTER_CRITICAL(&s_encoder_mux);
    if (encoder_queue_count == 0) {
        portEXIT_CRITICAL(&s_encoder_mux);
        return 0;
    }
    int8_t delta = encoder_queue[encoder_queue_tail];
    encoder_queue_tail = (encoder_queue_tail + 1) % ENCODER_QUEUE_SIZE;
    encoder_queue_count--;
    portEXIT_CRITICAL(&s_encoder_mux);
    return delta;
}

// --- 编码器轮询任务 (FreeRTOS) ---
static void encoder_polling_task(void* parameter) {
    // 使用查找表进行编码器状态解码
    static const int8_t lookup_table[] = { 0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0 };

    while (true) {
        uint8_t current_encoder_state = (digitalRead(PIN_ENCODER_A) << 1) | digitalRead(PIN_ENCODER_B);

        if (current_encoder_state != last_encoder_state) {
            int8_t direction = lookup_table[(last_encoder_state << 2) | current_encoder_state];

            // 在临界区内完成counter的读取、判断、清零，避免竞态条件
            int8_t event = 0;
            portENTER_CRITICAL(&s_encoder_mux);
            encoder_counter += direction;
            if (encoder_counter >= INPUT_ENCODER_THRESHOLD) {
                event = 1;
                encoder_counter = 0;
            } else if (encoder_counter <= -INPUT_ENCODER_THRESHOLD) {
                event = -1;
                encoder_counter = 0;
            }
            portEXIT_CRITICAL(&s_encoder_mux);

            // 在临界区外入队（enqueue内部有自己的临界区保护）
            if (event != 0) {
                encoder_enqueue(event);
            }

            last_encoder_state = current_encoder_state;
        }

        // 短延迟轮询：足够快捕获事件，又不会阻塞其他任务
        vTaskDelay(1);  // 1 tick ≈ 1ms (configTICK_RATE_HZ = 1000)
    }
}

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

    // 创建高优先级编码器轮询任务
    BaseType_t task_created = xTaskCreate(
        encoder_polling_task,
        "EncoderPoll",
        2048,  // Stack size
        NULL,
        configMAX_PRIORITIES - 1,  // 高优先级
        &s_encoder_task_handle
    );

    if (task_created != pdPASS) {
        LOG_ERROR("InputManager", "Failed to create encoder polling task");
    } else {
        LOG_INFO("InputManager", "Input manager initialized (FreeRTOS task mode)");
    }
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
    // --- 编码器旋转由FreeRTOS任务处理，此处不再轮询 ---

    // --- 编码器按键处理 (带消抖 + 长按检测) ---
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
                button_press_start_time = millis(); // 记录按下开始时间
                unsigned long current_time = millis();
                // 检测双击：如果当前是pending状态且在双击间隔内
                if (button_pending && (current_time - last_click_time) < INPUT_DOUBLE_CLICK_INTERVAL_MS) {
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

    // 检测长按：如果按钮持续按下且超过阈值
    if (button_stable_state == LOW && button_press_start_time > 0) {
        if ((millis() - button_press_start_time) >= INPUT_LONG_PRESS_THRESHOLD_MS) {
            button_long_pressed_flag = true; // 触发长按事件
            button_pending = false; // 清除pending状态（长按不触发单击/双击）
            button_press_start_time = 0; // 清零，避免重复触发
        }
    }

    // 检查pending超时：仅在按钮已经松开时才确认为单击
    // 这样可以避免在用户长按过程中误触发单击事件
    if (button_pending && button_stable_state == HIGH) {
        if ((millis() - last_click_time) >= INPUT_DOUBLE_CLICK_INTERVAL_MS) {
            button_clicked_flag = true; // 确认为单击
            button_pending = false; // 清除pending状态
        }
    }

    // 始终更新上一次的读数，用于检测下一次变化
    last_button_state = reading;
}

int8_t input_manager_get_encoder_delta() {
    // 从中断安全的循环队列中取出一个增量值
    return encoder_dequeue();
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

bool input_manager_get_button_long_pressed() {
    if (button_long_pressed_flag) {
        button_long_pressed_flag = false; // 读取后清除标志（消费型）
        return true;
    }
    return false;
}

void input_manager_clear_events() {
    // 清空中断队列
    portENTER_CRITICAL(&s_encoder_mux);
    encoder_queue_head = 0;
    encoder_queue_tail = 0;
    encoder_queue_count = 0;
    encoder_counter = 0; // 也清零编码器累积值
    portEXIT_CRITICAL(&s_encoder_mux);

    // 清空按键事件
    button_clicked_flag = false;
    button_double_clicked_flag = false;
    button_long_pressed_flag = false;
    button_pending = false; // 也清除pending状态
    last_click_time = 0;
    button_press_start_time = 0;
}

void input_manager_clear_button_events() {
    // 只清空按键事件，保留编码器队列
    button_clicked_flag = false;
    button_double_clicked_flag = false;
    button_long_pressed_flag = false;
    button_pending = false;
    last_click_time = 0;
    button_press_start_time = 0;
    // 注意：不清空 encoder_queue，不清空 encoder_counter
}