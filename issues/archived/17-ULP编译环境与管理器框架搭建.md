本任务是实现“哨兵-指挥官”架构的第一步，旨在搭建 ULP 协处理器的编译环境，并创建其在主程序中的管理器框架。

-   **任务清单:**
    1.  在 `platformio.ini` 中，为开发和测试环境添加 `board_build.ulp_enable = yes` 以启用 ULP 编译。
    2.  在项目根目录创建 `ulp/` 文件夹，并添加一个空的 `ulp_main.c` 文件作为哨兵代码的起点。
    3.  在 `platformio.ini` 中添加 `custom_ulp_source_dir = ulp` 指向该目录。
    4.  在 `src/managers/ulp_manager.cpp` 和 `.h` 文件中，实现 `ulp_manager_init()` 和 `ulp_manager_load_and_run()` 的基本函数框架。

-   **验收标准:**
    1.  项目在添加了 ULP 配置后能够成功编译，没有链接错误。
    2.  在 `main.cpp` 的 `setup()` 函数中调用 `ulp_manager_init()` 不会引发编译或运行时错误。
    3.  编译后，`.pio/build/<环境名>/` 目录下应能找到 `ulp_main.bin` 文件。