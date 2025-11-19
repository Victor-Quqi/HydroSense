本任务旨在实现主 CPU（指挥官）对 ULP 的控制逻辑，并集成正确的深度睡眠机制，解决关键的功耗问题。

-   **任务清单:**
    1.  在 `ulp_manager.cpp` 中完整实现 `ulp_manager_load_and_run()`，调用 `ulp_riscv_load_binary` 和 `ulp_riscv_run`。
    2.  在 `main.cpp` 的 `RUN` 模式逻辑中，调用 `ulp_manager` 加载并运行 ULP 程序。
    3.  实现 `hal_rtc_enter_deep_sleep()` 函数，在其中设置 ULP 作为唤醒源，并设置唤醒周期。
    4.  **关键:** 在进入睡眠前，必须调用 `esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);` 来修复 2mA 的功耗 BUG。
    5.  在 `main.cpp` 的 `setup()` 之后，增加对唤醒原因的判断逻辑。

-   **验收标准:**
    1.  在测试模式下添加 CLI 命令 `sleep <seconds>`，执行后设备进入深度睡眠，并在指定时间后由 RTC 定时器唤醒。
    2.  在深度睡眠期间，理论电流应稳定在 **15uA 以下**。
    3.  设备被 ULP 唤醒后，应能正确识别唤醒原因。