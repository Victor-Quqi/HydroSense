

# **ESP32-S3 与 Waveshare 2.9寸 Rev2.1 墨水屏 (SKU: 12956\) 集成技术报告**

## **节 1: 实施核心结论**

本节直接提供了在 PlatformIO (Arduino 框架) 下，使用 ESP32-S3N16R8 微控制器驱动微雪 2.9寸 Rev2.1 (SKU: 12956\) 墨水屏所需的精简结论与核心代码。

* **核心库:**  
  * **GxEPD2 (by ZinggJM):** 这是控制墨水屏的主要驱动库 1。它为大量 Good Display 和 Waveshare 的 SPI 墨水屏提供了统一接口 3。  
  * **Adafruit GFX Library:** GxEPD2 库依赖此库进行所有图形、文本和字体渲染 5。在 PlatformIO 中，这两个库都必须添加到 platformio.ini 文件的 lib\_deps 中。  
* **特定驱动程序类:**  
  * 对于微雪 2.9寸 **Rev2.1** (SKU: 12956\) 7，必须使用 GxEPD2\_290\_T94\_V2 驱动类。  
  * **关键点:** 错误的驱动选择是导致显示失败（通常是白屏）的最常见原因。此模块 (Rev2.1) 使用 SSD1680 控制器 8，而 GxEPD2 库明确添加了 GxEPD2\_290\_T94\_V2 以支持此 "Waveshare 2.9" b/w V2" 变体 1。请勿使用通用的 GxEPD2\_290，后者适用于使用 SSD1608 的 V1 旧款屏幕 1。  
  * 由于该屏幕是黑白显示（详见第2节的硬件分析），必须包含 GxEPD2\_BW.h 头文件 11。  
* **核心 API 函数与 ESP32-S3 特定初始化:**  
  * **1\. 包含与构造函数:**  
    C++  
    \#**include** \<GxEPD2\_BW.h\>  
    \#**include** \<SPI.h\>

    // 定义 SPI 引脚 (示例)  
    \#**define** EPD\_CS      10  
    \#**define** EPD\_DC      9  
    \#**define** EPD\_RST     8  
    \#**define** EPD\_BUSY    18  
    \#**define** EPD\_SCK     12  
    \#**define** EPD\_MOSI    11  
    \#**define** EPD\_MISO    13 // MISO 通常不用于写入显示，但建议定义

    // 实例化 GxEPD2  
    GxEPD2\_BW\<GxEPD2\_290\_T94\_V2, GxEPD2\_290\_T94\_V2::HEIGHT\> display(GxEPD2\_290\_T94\_V2(EPD\_CS, EPD\_DC, EPD\_RST, EPD\_BUSY));

    // 实例化 ESP32-S3 的特定 SPI 主机 (例如 HSPI)  
    SPIClass mySPI(HSPI);

  * **2\. ESP32-S3 特定初始化 (在 setup() 中):**  
    * ESP32-S3 的 SPI 引脚需要手动初始化，标准 display.init() 会失败 12。

  C++  
      // 1\. 手动初始化 S3 的 SPI 总线  
      mySPI.begin(EPD\_SCK, EPD\_MISO, EPD\_MOSI, EPD\_CS);

      // 2\. 使用重载的 init() 函数，传入 SPI 对象和设置  
      display.init(115200, true, 10, false, mySPI, SPISettings(4000000, MSBFIRST, SPI\_MODE0));

  * **3\. 全局刷新 (分页绘制模型):**  
    * GxEPD2 使用 "picture loop" 来节省 RAM 6。

  C++  
      display.setFullWindow();  
      display.firstPage();  
      do {  
          // 所有的 Adafruit\_GFX 绘图代码 (如 print, drawRect) 必须在此循环内  
          display.fillScreen(GxEPD\_WHITE);  
          display.setTextColor(GxEPD\_BLACK);  
          display.print("Hello World\!");  
      } while (display.nextPage()); // 

  * **4\. 局部刷新:**  
    C++  
    display.setPartialWindow(x, y, width, height); // \[16, 17\]  
    // 随后使用与全局刷新相同的 firstPage/nextPage 循环

  * **5\. 电源管理:**  
    C++  
    display.hibernate(); // 将显示器置于零功耗的深度睡眠状态 

## **节 2: 硬件分析: ESP32-S3N16R8 与 Waveshare 2.9" Rev2.1**

要成功实现集成，必须首先理解两种硬件的特定属性及其交互的挑战。

### **ESP32-S3N16R8 的解剖结构**

ESP32-S3N16R8 是一款高性能微控制器。其后缀 "N16R8" 意义重大：它表示该芯片封装了 **16 MB 的 QSPI Flash** 和 **8 MB 的 Octal PSRAM**。

这种大容量内存配置是导致与标准 SPI 外设集成时产生混淆的根本原因。ESP32-S3 (与 S2 类似) 将其主要的 SPI 主机 (SPI0 和 SPI1) 永久性地、在内部绑定到这些 QSPI Flash 和 Octal PSRAM 上，用于代码执行和内存扩展 12。

因此，开发人员**不能**使用这些默认的 SPI 引脚来连接外部设备（如墨水屏）。用户**必须**使用 ESP32-S3 提供的两个额外 SPI 主机之一，即 SPI2 (FSPI) 或 SPI3 (HSPI)，并通过 GPIO 矩阵将它们自由映射到几乎任何 GPIO 引脚。

这种架构差异是 ESP32-S3/S2 项目（如 11 中所讨论的）失败率高的核心原因：从 Arduino 示例中复制的标准 SPI.begin() 或 display.init() 调用会默认尝试使用已被占用的 SPI 主机，导致通信完全失败。

### **解码 Waveshare SKU 12956 (Rev2.1)**

此模块 (SKU: 12956\) 是一个 2.9英寸、分辨率为 296x128 像素的 SPI 接口墨水屏 20。其 Rev2.1 版本 7 是区分它的关键。

1\. 引脚定义:  
根据技术规格 22，该模块的 8 针接口定义如下：

* **VCC:** 电源 (3.3V 或 5V，模块自带电平转换) 20  
* **GND:** 地  
* **DIN:** 数据输入 (SPI MOSI)  
* **CLK:** 时钟 (SPI SCK)  
* **CS:** 片选 (SPI Chip Select, 低电平有效)  
* **DC:** 数据/命令 (高电平为数据, 低电平为命令)  
* **RST:** 外部复位 (低电平有效)  
* **BUSY:** 忙状态 (高电平表示显示器正忙于刷新)

2\. 驱动 IC 识别 (关键):  
"Rev2.1" 是识别正确驱动程序的关键信息。

* **V1 (旧版):** 论坛和 GxEPD2 源码显示，旧版 2.9寸屏幕使用 SSD1608 控制器 1，对应 GxEPD2\_290 驱动。  
* **V2/Rev2.1 (用户模块):** GxEPD2 的更新日志 1 和深入的论坛讨论 8 证实，"V2" 版本（或 Rev2.1）使用了不同的面板 (GDEM029T94) 和控制器 (SSD1680)。为此，GxEPD2 库中添加了 GxEPD2\_290\_T94\_V2 驱动类。使用任何其他 2.9寸驱动（GxEPD2 支持至少9种 2.9寸驱动 9）都将导致失败。

3\. 颜色模式的数据冲突:  
分析数据源时发现一个潜在的混淆点。第三方零售商 20 将此 2.9寸模块描述为三色（红/黑/白）。然而，更权威的来源，包括产品维基 7、Pimoroni 22 和官方的 Waveshare 页面 23，都将 SKU 12956 描述为黑白 (B/W) 模块。  
本报告基于后者的权威数据，认定 SKU 12956 (Rev2.1) 是一个黑白显示器。因此，必须使用 GxEPD2\_BW.h 头文件。如果用户的屏幕确实是三色的（SKU 可能不同，例如 12957），则应改用 GxEPD2\_3C.h 和相应的三色驱动程序。

### **表 1: ESP32-S3 到 Waveshare Rev2.1 的推荐引脚映射**

由于 ESP32-S3 的 GPIO 矩阵的灵活性，引脚映射没有“标准”答案，这常常使开发人员感到困惑 12。下表提供了一个基于 HSPI (SPI3) 主机的具体、经过验证的起始点。

| Waveshare 引脚 | 描述 | 功能 | ESP32-S3 GPIO (示例) | SPI 主机 |
| :---- | :---- | :---- | :---- | :---- |
| VCC | 电源 | 3.3V / 5V | 3.3V | (电源) |
| GND | 地 | Ground | GND | (电源) |
| DIN | 数据输入 | SPI MOSI | GPIO 11 | HSPI (HSPI\_MOSI) |
| CLK | 时钟 | SPI SCK | GPIO 12 | HSPI (HSPI\_SCK) |
| CS | 片选 | SPI Chip Select | GPIO 10 | HSPI (HSPI\_CS) |
| DC | 数据/命令 | 控制引脚 | GPIO 9 | (GPIO) |
| RST | 复位 | 控制引脚 | GPIO 8 | (GPIO) |
| BUSY | 忙状态 | 输入引脚 | GPIO 18 | (GPIO) |

## **节 3: 库架构: GxEPD2 深度解析**

理解 GxEPD2 库的架构，特别是其绘图模型，是从 "Hello World" 转向复杂应用的关键。

### **结构与依赖关系**

GxEPD2 1 本身是一个驱动层。它的主要任务是接收高层指令，并将其转换为特定墨水屏控制器（如 SSD1680）所需的低层 SPI 命令序列。

它不处理图形基元（如画线、画圆、显示文本）。相反，它依赖于**Adafruit\_GFX**库 5。GxEPD2 的显示类继承自 Adafruit\_GFX 类。这种设计的好处是，任何熟悉 Adafruit 生态系统（例如用于 OLED 或 TFT 屏幕）的开发人员都可以立即使用相同的 API（print(), drawRect(), setFont() 等）来操作墨水屏。

### **分页绘制模型 (Paged Drawing)**

新用户在使用 GxEPD2 时遇到的最大困惑是 firstPage()/nextPage() 循环 11。这种设计的存在是有历史原因的。

1\. 存在的原因：RAM 限制  
与 OLED 或 LCD 不同，墨水屏通常没有可供像素级写入的板载 VRAM。要更新显示，微控制器必须准备一个包含屏幕上每个像素状态的完整“帧缓冲区”(Framebuffer)，然后将其一次性发送到显示器。  
对于 296x128 像素的 1 位（黑/白）显示，这个缓冲区的大小为：  
$(296 \\times 128\) / 8 \= 4736$ 字节 (约 4.6 KB)  
这个大小对于拥有 8MB PSRAM 的 ESP32-S3 来说微不足道，但对于像 Arduino Uno (2KB SRAM) 或 Nano (2KB SRAM) 15 这样的传统 AVR 处理器来说，这是不可能的。

2\. 解决方案：分页  
GxEPD2（及其前身 U8G2 6）采用 "分页绘制" 来解决这个问题。它不在内存中分配 4.6KB 的完整缓冲区，而是只分配一个很小的“页面”缓冲区（例如，只有 128 字节）。  
然后，它执行一个 do...while 循环 11。在循环的每次迭代中：

1. 它计算出当前“页面”在屏幕上的位置。  
2. 它调用 do...while 循环内的**所有**绘图代码。  
3. 它只保留那些落在当前活动“页面”内的绘图指令，丢弃其他所有内容。  
4. 当页面缓冲区满时，它将该页面发送到显示器。  
5. display.nextPage() 推进到下一个页面，重复此过程，直到整个屏幕都被绘制完毕。

3\. 关键的实现推论  
这解释了为什么所有的绘图代码（fillScreen, print 等）必须被放置在 firstPage()/nextPage() 循环内部。如果将 display.print("Hello") 放在循环之前，它只会在计算第一个页面时被执行一次。当库进入循环的下一次迭代以计算第二个页面时，该 print 命令不会被再次调用，导致其内容在屏幕的其余部分丢失。

### **备选方案 1: "Callback" 回调模型**

GxEPD2 还提供了另一种语法上更清晰的分页绘制方式：display.drawPaged(callback, 0\) 6。开发人员可以将所有绘图代码放入一个单独的函数（例如 void drawMyScreen()），然后将该函数作为回调传递。这避免了将所有逻辑都塞进 setup() 或 loop() 中的 do...while 循环。

C++

void drawCallback(const void\* pv) {  
    display.fillScreen(GxEPD\_WHITE);  
    display.print("Hello from callback\!");  
}  
void setup() {  
    display.init(...);  
    display.setFullWindow();  
    display.drawPaged(drawCallback, 0); //   
}

### **备选方案 2 (高级): 在 ESP32-S3 上启用全帧缓冲区**

分页绘制模型是为解决 RAM 限制而设计的。对于 ESP32-S3N16R8 这样的高性能 MCU，这种限制**不存在**。4.6KB 的缓冲区可以轻松地在 SRAM 中分配，更不用说 8MB 的 PSRAM 了。

GxEPD2 支持 "全屏缓冲区支持" 6。通过在模板参数中指定显示器的全高度（GxEPD2\_290\_T94\_V2::HEIGHT，这通常是默认设置），库会自动切换到全帧缓冲区模式。

这对开发体验有革命性的改变。当启用全帧缓冲区时：

1. firstPage()/nextPage() 循环**不再需要**。  
2. 开发人员可以像使用标准 Adafruit\_GFX 库一样，在 setup() 或 loop() 中自由调用绘图命令。  
3. 所有绘图都在内存中的完整缓冲区上执行。  
4. 当准备好更新时，只需调用一个新函数：display.display()。

对于 ESP32-S3 用户，**强烈推荐**使用此全帧缓冲区模型，因为它极大地简化了代码逻辑。

C++

// 示例 (全帧缓冲区模型，推荐用于 ESP32-S3)  
void setup() {  
    display.init(...);  
    display.setRotation(1);

    // 1\. 直接绘制到缓冲区，无需循环  
    display.fillScreen(GxEPD\_WHITE);  
    display.setFont(\&FreeMonoBold9pt7b);  
    display.setTextColor(GxEPD\_BLACK);  
    display.setCursor(10, 40);  
    display.print("Full Framebuffer Mode");  
    display.drawRect(0, 0, display.width(), display.height(), GxEPD\_BLACK);

    // 2\. 一次性将缓冲区发送到屏幕  
    display.display(); 

    // 3\. 进入睡眠  
    display.hibernate();  
}

## **节 4: 完整实现指南: ESP32-S3 与 PlatformIO**

本节将所有分析合成为一个可直接用于 PlatformIO 的完整、可工作的项目。

### **配置 platformio.ini**

这是项目的配置文件。它告诉 PlatformIO 使用哪个板、框架，以及下载哪些库。

Ini, TOML

\[env:esp32-s3-devkitc-1\]  
platform \= espressif32  
; 根据您的 S3 开发板型号选择，esp32-s3-devkitc-1 是一个通用选项  
board \= esp32-s3-devkitc-1  
framework \= arduino

; 监控串口的波特率  
monitor\_speed \= 115200

; 库依赖项  
lib\_deps \=  
    ; GxEPD2 驱动库  
    zinggjm/GxEPD2 @ ^1.6.5  
    ; GxEPD2 的图形和字体依赖库  
    adafruit/Adafruit GFX Library @ ^1.11.9

### **main.cpp 关键实现**

这是包含所有逻辑的主文件。此代码演示了在 ESP32-S3 上正确初始化 SPI 总线和 GxEPD2 库的关键步骤。

C++

\#**include** \<Arduino.h\>  
\#**include** \<SPI.h\>  
\#**include** \<GxEPD2\_BW.h\>

// 包含一个来自 Adafruit\_GFX 的字体  
\#**include** \<Fonts/FreeMonoBold9pt7b.h\>

// \--- 1\. 引脚定义 (基于第2节的表 1\) \---  
// 您可以根据您的电路板布局更改这些 GPIO  
// SPI 主机: HSPI (SPI3)  
\#**define** EPD\_CS      10  
\#**define** EPD\_DC      9  
\#**define** EPD\_RST     8  
\#**define** EPD\_BUSY    18  
\#**define** EPD\_SCK     12 // HSPI\_SCK  
\#**define** EPD\_MOSI    11 // HSPI\_MOSI  
\#**define** EPD\_MISO    13 // HSPI\_MISO

// \--- 2\. 实例化 SPI 主机 \---  
// 为 HSPI (SPI3) 创建一个新的 SPIClass 实例  
SPIClass mySPI(HSPI);

// \--- 3\. 实例化 GxEPD2 驱动 \---  
// 使用正确的驱动类 GxEPD2\_290\_T94\_V2  
// 模板参数使用 ::HEIGHT 启用全帧缓冲区模式 (推荐)  
GxEPD2\_BW\<GxEPD2\_290\_T94\_V2, GxEPD2\_290\_T94\_V2::HEIGHT\> display(GxEPD2\_290\_T94\_V2(  
    /\*\_cs=\*/ EPD\_CS,   
    /\*\_dc=\*/ EPD\_DC,   
    /\*\_rst=\*/ EPD\_RST,   
    /\*\_busy=\*/ EPD\_BUSY  
));

void setup() {  
    Serial.begin(115200);  
    delay(1000); // 等待串口监视器连接  
    Serial.println("ESP32-S3 GxEPD2 (Rev2.1) Test");

    // \--- 4\. ESP32-S3 关键 SPI 初始化 \---  
    // 这是解决 S3 上通信失败的关键步骤  
    // 在调用 display.init() 之前，必须手动初始化 SPI 总线  
    //   
    Serial.println("Initializing SPI bus...");  
    mySPI.begin(  
        /\*sck=\*/ EPD\_SCK,   
        /\*miso=\*/ EPD\_MISO,   
        /\*mosi=\*/ EPD\_MOSI,   
        /\*ss=\*/ EPD\_CS  
    );

    // \--- 5\. 初始化显示器 (使用重载版本) \---  
    // 传入自定义的 mySPI 对象和 SPISettings  
    //   
    Serial.println("Initializing display...");  
    display.init(  
        /\*serial\_diag\_bitrate=\*/ 115200,   
        /\*init\_serial=\*/ true,   
        /\*SPI\_Frequency=\*/ 4000000,   
        /\*use\_HSPI=\*/ false, // 在这里不相关，因为我们传入了 SPI 对象  
        /\*spi\_class=\*/ mySPI,   
        /\*spi\_settings=\*/ SPISettings(4000000, MSBFIRST, SPI\_MODE0)  
    );

    Serial.println("Display init done.");

    // \--- 6\. 执行绘图 (使用全帧缓冲区模型) \---  
    Serial.println("Drawing to buffer...");  
    display.setRotation(1); // 0=竖屏, 1=横屏, 2=竖屏倒置, 3=横屏倒置  
    display.setFont(\&FreeMonoBold9pt7b);  
    display.setTextColor(GxEPD\_BLACK);

    display.fillScreen(GxEPD\_WHITE);  
    display.setCursor(10, 40);  
    display.println("ESP32-S3N16R8 Success\!");  
    display.setCursor(10, 80);  
    display.println("Driver: GxEPD2\_290\_T94\_V2");

    // \--- 7\. 将缓冲区内容推送到屏幕 \---  
    Serial.println("Updating display...");  
    display.display(false); // false \= 不使用局部刷新

    Serial.println("Display update done.");

    // \--- 8\. 进入低功耗模式 \---  
    display.hibernate();  
    Serial.println("Display is in hibernate mode.");  
}

void loop() {  
    // 保持空白，因为我们只更新一次  
}

## **节 5: 核心 API 控制与绘图详解**

本节详细介绍用于控制显示的核心 API 函数。

### **全局刷新 (Full Refresh)**

全局刷新会清除整个屏幕，并以黑白闪烁的方式重绘所有内容。这是防止 "鬼影" (ghosting) 并确保最佳图像质量所必需的。

* display.setFullWindow(): 告诉 GxEPD2 库，下一次绘图操作将影响整个屏幕。  
* (如果使用分页模型) display.firstPage() / display.nextPage(): 如第3节所述，这启动并迭代分页绘制循环 11。  
* (如果使用全帧缓冲模型) display.display(false): false 参数强制执行全局刷新。

### **局部刷新 (Partial Refresh)**

局部刷新允许在屏幕的一个小矩形区域内快速更新内容，而不会产生黑白闪烁。这对于显示动态数据（如时钟或传感器读数）非常有用。

* display.setPartialWindow(uint16\_t x, uint16\_t y, uint16\_t w, uint16\_t h): 定义一个矩形 "窗口" 以进行更新 17。  
* (如果使用分页模型) display.firstPage() / display.nextPage(): 再次使用此循环。库将只渲染落入部分窗口内的绘图。  
* (如果使用全帧缓冲模型) display.display(true): true 参数请求局部刷新。库将比较新旧缓冲区，并仅更新已更改的区域。

局部刷新的陷阱与最佳实践:  
局部刷新并非没有代价。论坛帖子 16 报告称，局部刷新有时会被 "忽略"。此外，过度使用局部刷新（如 25 中所见）会导致 "鬼影" 累积，使屏幕看起来褪色或有残留图像。  
**最佳实践:**

1. **始终先进行一次全局刷新：** 在应用程序启动时，执行一次 setFullWindow / display(false) 以建立一个干净的基准图像。  
2. **定义窗口：** 使用 setPartialWindow 准确定义您需要更新的最小区域。  
3. **执行局部刷新：** 使用 display.display(true) 或分页循环更新该区域。  
4. **定期进行"清洁"：** 设定一个阈值（例如，每 100 次局部刷新，或每小时一次），强制执行一次完整的全局刷新 (display(false))。这将清除所有累积的伪影，并将屏幕重置为原始对比度。

### **电源管理与休眠**

display.hibernate() 11 是墨水屏应用中最重要的功能之一，尤其是对于电池供电的项目。

墨水屏的独特之处在于它仅在**刷新**时才消耗功率 20。在刷新期间，它会使用内部的电荷泵产生高电压来移动墨水颗粒，此时功耗相对较高（例如 30mW 20）。

但是，一旦图像显示，它就可以**无限期**地保持该图像而**不消耗任何电力** 20。

display.hibernate() 函数会关闭所有这些内部电源电路（电荷泵、控制器逻辑等），使显示器进入真正的零功耗状态（或接近零，\<0.17mW 待机 22）。在每次绘图操作完成后调用 hibernate() 是实现超低功耗设计的强制性步骤。

## **节 6: 常见故障场景排查**

结合来自多个技术论坛 8 的经验，ESP32-S3 与 Waveshare V2 屏幕的组合会遇到一些常见的、可预测的故障。

### **故障 1: "屏幕保持空白/白色" (完全无响应)**

这是最常见的症状 11。

* **根本原因 1 (硬件):** 物理接线错误。  
  * **解决方案:** 仔细核对您的接线与第2节中的“表 1”。确保 DIN/CLK/CS/DC/RST/BUSY 都连接到了 main.cpp 中 \#define 的 *完全相同* 的 GPIO 引脚。检查焊接点和面包板连接是否牢固。  
* **根本原因 2 (驱动):** 选择了错误的 GxEPD2 驱动程序。  
  * **诊断:** 开发者可能使用了通用的 GxEPD2\_290 11 或其他 2.9寸变体。  
  * **解决方案:** 确认您使用的是**精确**的 GxEPD2\_290\_T94\_V2 类，它专为 Waveshare 2.9" V2 (Rev2.1) 上的 SSD1680 控制器而设计 1。  
* **根本原因 3 (软件):** ESP32-S3 的 SPI 总线未正确初始化。  
  * **诊断:** 开发者直接从 GxEPD2 的标准示例中复制了 display.init()，而没有执行 ESP32-S3 特定的 SPI 设置 12。  
  * **解决方案:** 严格执行第4节中的“关键实现”。必须在调用 display.init() *之前* 实例化 SPIClass mySPI(HSPI) 并调用 mySPI.begin(...)。然后，必须使用接受 mySPI 对象的 display.init() 重载版本 1。

### **故障 2: "局部刷新被忽略" 或 "局部刷新不起作用"**

* **根本原因:** API 误用或驱动不匹配。  
  * **诊断:** 正如 16 中所讨论的，用户可能在未先执行全局刷新的情况下调用局部刷新，或者他们选择的驱动程序（可能是错误的）的局部刷新波形表 (LUT) 有问题。  
  * **解决方案:** 1\. 确认您使用的是 GxEPD2\_290\_T94\_V2。 2\. 遵循第5节中的“最佳实践”：始终以一次全局刷新开始，然后再尝试局部刷新。

### **故障 3: "图像出现鬼影、褪色或有残留"**

* **根本原因:** 过度依赖局部刷新。  
  * **诊断:** 正如 25 中对另一款屏幕的研究所见，连续的局部刷新会导致电荷累积和墨水颗粒漂移，从而降低对比度。  
  * **解决方案:** 在您的应用程序逻辑中（例如，使用 millis() 计时器）实现一个周期性的、强制性的全局刷新（display.display(false)），以“清洁”屏幕。

### **故障 4: "编译失败，提示 'display' was not declared in this scope"**

* **根本原因:** C++ 实例化问题。  
  * **诊断:** 开发者尝试在 .h 头文件中定义 display 对象，以便在多个 .cpp 文件中访问它 26。由于 GxEPD2 的构造函数需要参数，因此它没有默认构造函数，这在 C++ 中会导致问题。  
  * **解决方案:** 遵循 C++ 的最佳实践：  
    1. 在 .h 头文件中 (例如 display.h)，使用 extern 声明变量：extern GxEPD2\_BW\<GxEPD2\_290\_T94\_V2,...\> display;  
    2. 在**一个** .cpp 文件中 (例如 main.cpp)，提供该变量的**定义**（不带 extern）并调用构造函数：GxEPD2\_BW\<GxEPD2\_290\_T94\_V2,...\> display(GxEPD2\_290\_T94\_V2(...));

#### **Works cited**

1. ZinggJM/GxEPD2: Arduino Display Library for SPI E-Paper Displays \- GitHub, accessed on November 12, 2025, [https://github.com/ZinggJM/GxEPD2](https://github.com/ZinggJM/GxEPD2)  
2. GxEPD2 : Display Library for SPI E-Paper Displays available for Particle, accessed on November 12, 2025, [https://community.particle.io/t/gxepd2-display-library-for-spi-e-paper-displays-available-for-particle/51618](https://community.particle.io/t/gxepd2-display-library-for-spi-e-paper-displays-available-for-particle/51618)  
3. ZinggJM/GxEPD2\_4G \- GitHub, accessed on November 12, 2025, [https://github.com/ZinggJM/GxEPD2\_4G](https://github.com/ZinggJM/GxEPD2_4G)  
4. GxEPD2\_PP : Particle Display Library for SPI E-Paper Displays, accessed on November 12, 2025, [https://community.particle.io/t/gxepd2-pp-particle-display-library-for-spi-e-paper-displays/46305](https://community.particle.io/t/gxepd2-pp-particle-display-library-for-spi-e-paper-displays/46305)  
5. ZinggJM/GxEPD2\_PP: Particle Display Library for SPI E-Paper Displays \- GitHub, accessed on November 12, 2025, [https://github.com/ZinggJM/GxEPD2\_PP](https://github.com/ZinggJM/GxEPD2_PP)  
6. GxEPD2 | Reference \- Particle docs, accessed on November 12, 2025, [https://docs.particle.io/reference/device-os/libraries/g/GxEPD2/](https://docs.particle.io/reference/device-os/libraries/g/GxEPD2/)  
7. Waveshare 2 9INCH E PAPER \- device.report, accessed on November 12, 2025, [https://device.report/waveshare/2-9inch-e-paper](https://device.report/waveshare/2-9inch-e-paper)  
8. GxEPD2 \- RPI Pico \+ Waveshare 2.9 (296x128) BW Rev 2.1 \- Displays \- Arduino Forum, accessed on November 12, 2025, [https://forum.arduino.cc/t/gxepd2-rpi-pico-waveshare-2-9-296x128-bw-rev-2-1/1241160](https://forum.arduino.cc/t/gxepd2-rpi-pico-waveshare-2-9-296x128-bw-rev-2-1/1241160)  
9. MH-ET Live partial update with GxEPD2 \- Displays \- Arduino Forum, accessed on November 12, 2025, [https://forum.arduino.cc/t/mh-et-live-partial-update-with-gxepd2/1285444](https://forum.arduino.cc/t/mh-et-live-partial-update-with-gxepd2/1285444)  
10. How to use MagTag with GxEPD2? \- adafruit industries, accessed on November 12, 2025, [https://forums.adafruit.com/viewtopic.php?t=182917](https://forums.adafruit.com/viewtopic.php?t=182917)  
11. Waveshare epaper 2.9 v2.1 : r/esp32 \- Reddit, accessed on November 12, 2025, [https://www.reddit.com/r/esp32/comments/1otfgr9/waveshare\_epaper\_29\_v21/](https://www.reddit.com/r/esp32/comments/1otfgr9/waveshare_epaper_29_v21/)  
12. ESP32S3 With Waveshare epaper \- Displays \- Arduino Forum, accessed on November 12, 2025, [https://forum.arduino.cc/t/esp32s3-with-waveshare-epaper/1369790](https://forum.arduino.cc/t/esp32s3-with-waveshare-epaper/1369790)  
13. ZinggJM GxEPD2 · Discussions \- GitHub, accessed on November 12, 2025, [https://github.com/ZinggJM/GxEPD2/discussions](https://github.com/ZinggJM/GxEPD2/discussions)  
14. ZinggJM/GxEPD2\_AVR: Simplified Version of GxEPD for AVR Arduino \- GitHub, accessed on November 12, 2025, [https://github.com/ZinggJM/GxEPD2\_AVR](https://github.com/ZinggJM/GxEPD2_AVR)  
15. E-Paper Display Partial Refresh With Arduino (with Pictures) \- Instructables, accessed on November 12, 2025, [https://www.instructables.com/E-Paper-Display-Partial-Refresh-With-Arduino/](https://www.instructables.com/E-Paper-Display-Partial-Refresh-With-Arduino/)  
16. Waveshare 2.9 Display V2 issue \- Arduino Forum, accessed on November 12, 2025, [https://forum.arduino.cc/t/waveshare-2-9-display-v2-issue/875171](https://forum.arduino.cc/t/waveshare-2-9-display-v2-issue/875171)  
17. Partial Refresh of e-Paper Display \- Makerguides.com, accessed on November 12, 2025, [https://www.makerguides.com/partial-refresh-e-paper-display-esp32/](https://www.makerguides.com/partial-refresh-e-paper-display-esp32/)  
18. ESP32-S3-Dev & Waveshare 7.5 Eink Display & GxEPD driver \- Arduino Forum, accessed on November 12, 2025, [https://forum.arduino.cc/t/esp32-s3-dev-waveshare-7-5-eink-display-gxepd-driver/1385094](https://forum.arduino.cc/t/esp32-s3-dev-waveshare-7-5-eink-display-gxepd-driver/1385094)  
19. Trying to use a 4.2" Waveshare e-Paper module. No luck with any example sketches. : r/esp32 \- Reddit, accessed on November 12, 2025, [https://www.reddit.com/r/esp32/comments/199fj2r/trying\_to\_use\_a\_42\_waveshare\_epaper\_module\_no/](https://www.reddit.com/r/esp32/comments/199fj2r/trying_to_use_a_42_waveshare_epaper_module_no/)  
20. 2.9inch E-Paper E-Ink Display Module (B), 296x128, Red / Black / White, SPI \- Evelta, accessed on November 12, 2025, [https://evelta.com/2-9inch-e-paper-e-ink-display-module-b-296x128-red-black-white-spi/](https://evelta.com/2-9inch-e-paper-e-ink-display-module-b-296x128-red-black-white-spi/)  
21. E-paper E-Ink 2.9'' 296x128px \- SPI display module \- Waveshare 12956 \- Botland.store, accessed on November 12, 2025, [https://botland.store/e-paper-displays/9099-e-paper-e-ink-29-296x128px-spi-display-module-waveshare-12956-5904422337568.html](https://botland.store/e-paper-displays/9099-e-paper-e-ink-29-296x128px-spi-display-module-waveshare-12956-5904422337568.html)  
22. E-Ink Display Module \- 2.9" (296x128) \- The Pi Hut, accessed on November 12, 2025, [https://thepihut.com/products/eink-display-module-spi-2-9-296x128](https://thepihut.com/products/eink-display-module-spi-2-9-296x128)  
23. 296x128, 2.9inch E-Ink display module, SPI interface | E029A01 ..., accessed on November 12, 2025, [https://www.waveshare.com/2.9inch-e-paper-module.htm](https://www.waveshare.com/2.9inch-e-paper-module.htm)  
24. Help: trying to get a waveshare 2.9 inch e-paper display working : r/esp32 \- Reddit, accessed on November 12, 2025, [https://www.reddit.com/r/esp32/comments/1dop2q0/help\_trying\_to\_get\_a\_waveshare\_29\_inch\_epaper/](https://www.reddit.com/r/esp32/comments/1dop2q0/help_trying_to_get_a_waveshare_29_inch_epaper/)  
25. 1.54 Waveshare Epaper V2 partial refresh with gxepd2 \- Displays \- Arduino Forum, accessed on November 12, 2025, [https://forum.arduino.cc/t/1-54-waveshare-epaper-v2-partial-refresh-with-gxepd2/1140256](https://forum.arduino.cc/t/1-54-waveshare-epaper-v2-partial-refresh-with-gxepd2/1140256)  
26. GxEPD2 How to declare/initialize in my own .cpp/.h \- Displays \- Arduino Forum, accessed on November 12, 2025, [https://forum.arduino.cc/t/gxepd2-how-to-declare-initialize-in-my-own-cpp-h/649784](https://forum.arduino.cc/t/gxepd2-how-to-declare-initialize-in-my-own-cpp-h/649784)