# HydroSense 代码架构

## 四层架构

```
┌─────────────────────────────────────┐
│   main.cpp (系统协调层)              │  模式切换、生命周期管理
├─────────────────────────────────────┤
│   Services 服务层                    │  高级业务服务
│   WiFi | LLM | 配置 | 时间 | 云 | 历史 │  (单例模式)
├─────────────────────────────────────┤
│   Managers 管理器层                  │  业务逻辑核心
│   运行模式 | 交互模式 | 电源 | 传感器  │
│   执行器 | 输入 | 日志 | UI           │
├─────────────────────────────────────┤
│   HAL 硬件抽象层                     │  硬件接口封装
│   GPIO | ADC | LEDC | SPI | RTC     │
└─────────────────────────────────────┘
```

## 目录结构

```
src/
├── main.cpp                          # 系统协调器
├── hal/                              # 硬件抽象层
│   ├── hal_config.h                  # ⭐ 引脚定义集中管理
│   └── hal_gpio/adc/ledc/spi/rtc    # 硬件接口封装
├── data/                             # 数据结构与常量
│   ├── data_models.h                 # 核心数据类型
│   ├── timing_constants.h            # ⭐ 时序常量集中管理
│   └── hydro_config.h                # 配置结构体
├── managers/                         # 管理器层（业务逻辑）
│   ├── run_mode_manager.*           # 自动浇水模式
│   ├── interactive_mode_manager.*   # 交互模式协调器
│   ├── interactive_mode/            # 交互子状态（5个）
│   │   ├── interactive_main_menu.*  # 主菜单
│   │   ├── interactive_status.*     # 状态显示
│   │   ├── interactive_settings.*   # 设置编辑
│   │   ├── interactive_watering.*   # 手动浇水
│   │   └── interactive_chat.*       # LLM对话
│   ├── power_manager.*              # 电源门控（MOSFET）
│   ├── sensor_manager.*             # 传感器采集
│   ├── actuator_manager.*           # 水泵控制
│   ├── input_manager.*              # 物理输入
│   ├── log_manager.*                # 日志系统
│   └── ui_manager.*                 # LVGL界面
├── services/                         # 服务层（高级功能）
│   ├── wifi_manager.*               # WiFi连接
│   ├── llm_connector.*              # LLM通信
│   ├── config_manager.*             # 配置持久化（NVS）
│   ├── time_manager.*               # NTP时间同步
│   ├── cloud_connector.*            # 云端集成
│   └── history_manager.*            # 对话历史
├── ui/                               # UI层
│   ├── display_manager.*            # 墨水屏驱动
│   └── ui_manager.*                 # LVGL集成
└── test/                             # 测试模式（12+文件）
    └── test_commands_*.*            # CLI测试命令
```

## 三种系统模式

| 模式 | 管理器 | 功能 |
|------|--------|------|
| **OFF** | 无 | 深度睡眠 |
| **RUN** | run_mode_manager | 自动浇水（每5秒检测湿度） |
| **INTERACTIVE** | interactive_mode_manager | 用户交互UI（状态机） |

### 交互模式状态机

```
STATE_MAIN_MENU (主菜单)
    ├─> STATE_STATUS (系统状态)
    ├─> STATE_SETTINGS (参数设置)
    ├─> STATE_WATERING (手动浇水)
    └─> STATE_CHAT (LLM对话)
```

## 核心设计原则

### 1. 集中化配置
- 所有引脚定义：`hal/hal_config.h`
- 所有时序常量：`data/timing_constants.h`
- **禁止在其他文件硬编码**

### 2. 物理电源门控
通过 `power_manager` 控制3个MOSFET（active-LOW）：
- 水泵12V升压模块
- 传感器电源
- 墨水屏电源

### 3. 中断-标志模式
- 中断仅设置标志位
- 决策逻辑在 `loop()` 执行

### 4. 结果枚举
管理器函数返回 `*_result_t` 枚举类型

### 5. 单例服务
Services层使用单例模式：
```cpp
WiFiManager::instance()
ConfigManager::instance()
LLMConnector::instance()
TimeManager::instance()
HistoryManager::instance()
```

### 6. 非阻塞操作
使用时间戳和标志位实现非阻塞等待

## 关键数据流

### 自动浇水流程
```
run_mode_manager_loop()
  → sensor_manager_read_all()
    → power_manager (开启传感器)
    → hal_adc_read()
  → 判断: 湿度 < 阈值?
  → actuator_manager_run_pump_for()
    → hal_ledc_set_duty() (PWM控制)
  → ui_manager_show_run_dashboard()
```

### LLM对话流程
```
interactive_chat_handle()
  → WiFiManager::isConnected()
  → HistoryManager::buildContextMessages()
  → LLMConnector::chatWithOptions() (HTTPS)
  → ui_manager_show_chat_screen()
```

## 持久化存储

| 类型 | 内容 | 位置 |
|------|------|------|
| **NVS** | 系统配置 | ESP32 NVS分区 |
| **SPIFFS** | 日志文件 | `/spiffs/system.log` (500KB轮转) |
| **SPIFFS** | 对话历史 | JSON格式 |

## 测试模式

编译测试环境：`pio run -e test`
- 启用 `TEST_MODE` 宏
- 提供串口CLI（JSON响应）
- 12+ 测试命令模块
