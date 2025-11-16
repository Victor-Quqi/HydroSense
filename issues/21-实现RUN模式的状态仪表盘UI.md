本任务旨在为 `RUN` 模式设计并实现一个简洁的状态仪表盘，用于在墨水屏上快照显示系统的核心状态。

-   **任务清单:**
    1.  在 `ui_manager.cpp` 中创建一个新函数 `ui_manager_show_run_dashboard()`。
    2.  UI 需显示至少以下信息：
        -   当前土壤湿度 (%)
        -   设定的浇水阈值 (%)
        -   当前电池电压 (V)
        -   上次浇水时间（可暂时显示为 "N/A"）
        -   系统状态（如 "Sleeping..."）
    3.  在 `main.cpp` 的 `RUN` 模式逻辑中，当系统因“手动刷新”（编码器按键单击）或“浇水完成”而被唤醒时，调用此函数更新屏幕。
    4.  更新屏幕的流程应为：`power_screen_enable(true)` -> `ui_manager_show_run_dashboard()` -> `delay` -> `power_screen_enable(false)`。

-   **验收标准:**
    1.  在 `RUN` 模式下，单击编码器按键，设备被唤醒，墨水屏刷新显示最新的传感器数据和预设阈值，然后设备返回睡眠。
    2.  执行 `force water` 测试命令后，屏幕应在浇水完成后刷新一次仪表盘。