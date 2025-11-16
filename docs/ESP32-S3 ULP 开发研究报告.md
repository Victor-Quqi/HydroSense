### 1. 开发环境配置 (`platformio.ini`)
这是让 Arduino 框架支持 ULP RISC-V 编译的前提。
*   **开启 ULP 编译：** 必须添加 `board_build.ulp_enable = yes`。
*   **指定源码目录：** `custom_ulp_source_dir = ulp`（通常在项目根目录下新建 `ulp` 文件夹）。
*   **头文件路径：** 添加 `-I.pio/build/<你的环境名>/ulp_main`，否则主程序找不到自动生成的 `ulp_main.h`。
*   **工具链：** 如果报错，可能需要强制更新 `platform_packages` 中的 `toolchain-riscv32-esp`。

### 2. 哨兵代码编写 (`ulp/ulp_main.c`)
这是在 RTC 域运行的 C 代码。
*   **数学运算限制：** **严禁使用浮点数 (`float`/`double`) 和 `<math.h>`**。ULP 没有 FPU，软模拟会导致代码体积爆炸或运行极慢。所有电压/湿度计算必须用**定点整数**完成（如 3300mV 而非 3.3V）。
*   **GPIO 控制 (P-MOSFET)：**
    *   **开电源：** `ulp_riscv_gpio_output_level(PIN, 0)` (拉低导通)。
    *   **关电源：** `ulp_riscv_gpio_output_level(PIN, 1)` (拉高截止)，或者设为输入模式让外部 2kΩ 电阻拉高（更省电）。
*   **防止寄生供电：** 在关断传感器电源（P-MOS）前，必须将连接传感器的 ADC 数据脚设为**低电平**或**高阻态**。
*   **延时防溢出：** `ulp_riscv_delay_cycles()` 在长延时下可能有溢出 bug，建议用循环分段延时来等待传感器上电稳定。
*   **共享变量：** 定义全局变量（如 `volatile uint32_t humidity_raw;`）供主 CPU 读取。

### 3. 指挥官代码编写 (`src/main.cpp`)
这是在主 CPU 运行的 Arduino 代码。
*   **引用 ULP 变量：** 必须 `#include "ulp_main.h"`。ULP 中的变量在主程序中会自动加上 `ulp_` 前缀（例如 `ulp_humidity_raw`）。
*   **加载与启动：**
    1.  `ulp_riscv_load_binary(...)` 加载固件。
    2.  `ulp_riscv_run()` 启动协处理器。
    3.  `ulp_set_wakeup_period(...)` 设置哨兵巡逻周期。
*   **致命功耗坑 (2mA Bug)：**
    *   **现象：** 开启 ADC 后睡眠电流飙升至 2mA+。
    *   **解决：** 必须在 sleep 配置中显式设置：
        `esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);`
        这让 RTC 外设电源仅在 ULP 工作时开启，ULP 停机时自动切断。

### 4. 调试策略
*   **没有 `Serial.print`：** ULP 无法直接打印日志。
*   **调试方法：** 在 ULP 中定义一个 `run_counter` 变量，每次运行 `++`。主 CPU 唤醒后打印 `ulp_run_counter`。
    *   如果值为 0：ULP 没启动。
    *   如果值巨大：ULP 在疯狂重启（可能看门狗复位）。
    *   如果值符合预期：运行正常。