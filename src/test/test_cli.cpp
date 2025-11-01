/**
 * @file test_cli.cpp
 * @brief 测试命令行接口实现
 */

#include "test_cli.h"

#ifdef TEST_MODE

// 存储串口输入的缓冲区
String inputBuffer = "";

/**
 * @brief 发送信标并换行
 */
void send_eot() {
    Serial.println(EOT_BEACON);
}

/**
 * @brief 处理接收到的完整命令
 * @param command 接收到的命令字符串 (不含换行符)
 */
void handle_command(const String& command) {
    if (command.equals("ping")) {
        Serial.println("pong");
    } else if (command.startsWith("echo ")) {
        // 提取 "echo " 后面的内容并打印
        Serial.println(command.substring(5));
    } else {
        Serial.println("Error: Unknown command");
    }
    // 任何命令处理后都必须发送信标
    send_eot();
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
                handle_command(inputBuffer);
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