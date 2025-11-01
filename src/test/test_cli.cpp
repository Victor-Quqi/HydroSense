/**
 * @file test_cli.cpp
 * @brief 测试命令行接口实现
 */

#include "test_cli.h"
#include "test_command_registry.h"

#ifdef TEST_MODE

// 存储串口输入的缓冲区
static String inputBuffer = "";

void send_eot() {
    Serial.println(EOT_BEACON);
}

void test_cli_init() {
    // 预留足够空间给输入缓冲区，防止溢出
    inputBuffer.reserve(128);
}

void test_cli_loop() {
    // 检查串口是否有可用数据
    while (Serial.available() > 0) {
        char incomingChar = Serial.read();

        // 如果是换行符，说明命令结束
        if (incomingChar == '\n') {
            // 去除首尾空白字符
            inputBuffer.trim();
            if (inputBuffer.length() > 0) {
                // 将完整命令转发给注册器处理
                test_registry_handle_command(inputBuffer);
            }
            // 清空缓冲区，准备接收下一条命令
            inputBuffer = "";
        } else if (incomingChar != '\r') { // 忽略回车符
            // 将有效字符添加到缓冲区
            inputBuffer += incomingChar;
        }
    }
}

#endif // TEST_MODE