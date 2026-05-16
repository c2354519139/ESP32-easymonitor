# 学习日志

记录每次提问、回答要点及学习建议，便于复盘。

---

## 2026-05-15

### Q1: `idf.py flash` 报错 "build directory configured for dht11 not ESP32-easymonitor"，为什么不能烧录？

**原因**：项目从 `dht11` 重命名为 `ESP32-easymonitor` 后，`build/` 目录里缓存着旧的 CMake 配置，路径指向 `/home/cyj/esp32/esp32-board/dht11`。CMake 检测到项目路径不匹配就直接拒绝。

**解决**：`idf.py fullclean` 删除整个 `build/` 目录，让它重新配置。

**延伸**：也可以用 `idf.py reconfigure` 或手动删 `build/` 再重新构建。

### 学习建议

- **理解 CMake 缓存机制**：ESP-IDF 的 `build/` 目录存储了 cmake 缓存、sdkconfig 生成的头文件、编译产物等。当你重命名项目目录时，缓存里的绝对路径就失效了。花 10 分钟读一下 `build/CMakeCache.txt` 的前几十行，看看里面都存了什么——这能帮你以后遇到类似问题时快速定位。
- **养成习惯**：重命名项目目录后第一时间跑 `idf.py fullclean`，省得后面踩坑。
- **区分 clean 层级**：
  - `idf.py clean` — 只删编译产物，保留 cmake 配置
  - `idf.py fullclean` — 删整个 build 目录，连 cmake 配置一起清掉
  - 手动 `rm -rf build` — 效果同 fullclean，但不会清理 managed_components

---

### Q2: 把 ST7735S 原始驱动替换成 LVGL，保持屏幕显示内容不变

**做了什么**：
1. 软链接复用 `lvgl_display/components/lvgl/`（v8.3.10）
2. 新建 `lv_port.c`：初始化显示（双缓冲 DMA）、注册 flush 回调、创建 5ms 心跳定时器。无触摸驱动。
3. 给 `st7735s.c` 添加 `st7735s_flush()` 供 LVGL 调用
4. `main.c` 里用 `lv_label` 控件替代 `st7735s_show_string()`
5. 修改 MADCTL 从 0x40 → 0xC0（加 MY 位，使 y=0 在顶部，对齐 LVGL 坐标系）
6. 创建自定义分区表 `partitions.csv`（factory 扩大到 1.25MB）
7. 裁剪 sdkconfig 禁用不需要的 LVGL 控件

**架构变化**：
```
原来: main.c → st7735s_show_string() → SPI polling → ST7735S
现在: main.c → lv_label_set_text() → lv_task_handler() → flush_cb → st7735s_flush() → SPI DMA → ST7735S
```

### 学习建议

- **LVGL 移植三要素**：任何屏幕接入 LVGL 只需要三样东西——`lv_init()`、一个 flush 回调（告诉 LVGL 怎么把像素刷到屏幕）、一个心跳定时器（`lv_tick_inc`，5ms 周期）。理解了这三个，任何屏幕都能接 LVGL。
- **理解 MADCTL**：ST7735S 的 0x36 寄存器控制显示方向。MX 翻转 X，MY 翻转 Y，MV 交换 XY。你可以用不同组合得到 0/90/180/270 四种旋转。改这个寄存器比在软件里翻坐标高效得多。
- **Kconfig 语法**：ESP-IDF 的 sdkconfig 中，禁用项必须写成 `# CONFIG_XXX is not set`，写成 `CONFIG_XXX=n` 是不生效的（会被 Kconfig 默认值覆盖）。这是一个常见的坑。
- **分区表**：默认 `partitions_singleapp.csv` 只给 factory 1MB。加了 LVGL 后固件超了 55KB，创建自定义分区表把 factory 扩到 1.25MB（2MB Flash 足够）。遇到 `app partition is too small` 就检查 `partitions.csv`。
- **LVGL 瘦身**：禁用不需要的控件（`LV_USE_ARC=n` 等）能减少编译进固件的 .c 文件数量。对于 128x160 这种小屏，用较小字体（Montserrat 10/12）比默认 14 更合适，也更省空间。

---

### Q3: LVGL 屏幕显示异常——字只显示下半部分

**现象**：WiFi OK 等文字完整但只有下半部分可见，上半部分空白。

**根因**：两个问题叠加——
1. **字体太大**：Montserrat 14（14px 高 × ~9px 宽），"BTC/USDT 95000.00 " 约 171px 宽，超出 128px 屏幕，被截断
2. **Y 轴映射错误**：我改了 MADCTL 从 0x40 → 0xC0 试图翻转 Y，但 flush 函数直接透传坐标，导致数据写入位置偏移

**修复**：
1. MADCTL 恢复 0x40（保持面板原始坐标，y=0 在底部）
2. flush 内做逐行 Y 翻转：`phys_y = 159 - lvgl_y`，且逐行 set_window + 发送，保证数据行顺序正确
3. 字体换成 Montserrat 10，label 间距调整为 3/25/50/75/100/125

### 学习建议

- **MADCTL vs 软件翻转的取舍**：改 MADCTL 寄存器看起来干净，但前提是你完全理解面板的物理坐标系和你驱动的映射关系。如果不确定，在 flush 里做逐行 Y 翻转虽然多几次 SPI 事务，但逻辑清晰、不容易出错，调试成本更低。
- **逐行 SPI vs 批量 DMA**：逐行发送效率低（10 行 × 256 字节/行 ≈ 18ms），但对于这种 500ms 刷新一次的低频场景完全够用。后面想优化可以加一个行缓冲区做 in-place reverse 再一次性 DMA。
- **LVGL 字体选择**：128px 宽屏配 Montserrat 10（每字符约 6-7px）能显示约 18 字符/行。选字体前先估算：屏幕宽度 / (字体高度 × 0.6) ≈ 每行字符数。不够就换小一号。

---

### Q4: 屏幕时不时黑屏重启

**根因**：主任务栈溢出。ESP-IDF 默认 `CONFIG_ESP_MAIN_TASK_STACK_SIZE=3584`（3.5KB），但加了 LVGL 后 `lv_task_handler()` 的渲染调用链（字体 glyph 处理、样式计算、draw task、flush 回调 SPI 操作）需要大量栈空间，栈溢出导致随机崩溃。

**修复**：sdkconfig 中把主任务栈从 3584 → 8192 字节。

### 学习建议

- **LVGL 任务栈经验值**：调用 `lv_task_handler()` 的任务至少需要 6-8KB 栈。ESP-IDF 默认 3.5KB 只够跑简单逻辑，一旦引入 LVGL 或 lwIP socket 就很容易崩。
- **栈诊断工具**：`uxTaskGetStackHighWaterMark(NULL)` 可以查询当前任务的剩余栈空间（单位是 word，乘以 4 就是字节）。在 `lv_task_handler()` 前后各调一次，看看还剩多少。
- **随机崩溃排查优先级**：栈溢出 > 堆碎片 > 看门狗 > 并发/中断冲突。ESP32 的栈溢出检测（`CONFIG_FREERTOS_CHECK_STACKOVERFLOW`）默认开启，但它只能在栈顶被完全踩烂时才能检测到，轻度溢出检测不到。
- **其他潜在隐患**：balance_task 和 price_task 各 4096 字节栈，里面有 HTTP + JSON 解析，也可能接近临界值。如果后续还有问题，把这两个也调到 6144。

---

### Q5: 加大栈后仍然黑屏重启

**根因**：`lv_obj_add_style()` 内存泄漏。主循环每 500ms 调用一次 `lv_obj_add_style(label, &style_xxx, 0)` 来切换文字颜色（绿/红/黑）。在 LVGL v8 中，`lv_obj_add_style` 会在对象内部链表**追加**一个新样式节点，不会自动删除旧节点。不同 style 指针来回切换 → 链表无限增长 → 堆耗尽 → 崩溃重启。

**修复**：用 `lv_obj_set_style_text_color(obj, lv_color_hex(0x...), 0)` 替代 `lv_obj_add_style`。这是直接设置属性值，不产生链表节点。

**教训**：`lv_obj_add_style` 是「绑定样式对象」，适合初始化时一次性设置。运行时动态改颜色/大小/边框等，应该用 `lv_obj_set_style_xxx()` 系列函数。

### 学习建议

- **LVGL 样式系统两条路径**：
  - `lv_obj_add_style(obj, &style, selector)` — 绑定一个 lv_style_t 实例，适合静态样式。重复调用会累积。
  - `lv_obj_set_style_xxx(obj, value, selector)` — 直接设属性值，适合动态变化。无内存副作用。
- **嵌入式内存泄漏排查思路**：周期性的崩溃（几分钟到几十分钟）大概率是泄漏。先检查循环里的内存分配/链表操作/资源创建调用，这些是泄漏高频来源。
- **LVGL 文档建议**：同一个 style 指针重复 add 到同一个 obj 是幂等的（不会重复添加），但**不同** style 指针会分别添加。所以如果要在红/绿/黑之间切换，正确做法是用一个 mutable style + `lv_style_set_text_color(&style, color)` 或者直接用 `lv_obj_set_style_text_color`。

---

### Q6: 字体颜色混乱，各种杂色像素

**根因**：RGB565 字节序不匹配。ESP32 是小端序，LVGL 的 `lv_color16_t` 在内存中是低字节在前（BGR 低位 + G 低位在前），但 ST7735S 期望高字节在前（R+G 高位先发）。SPI 直接发送 LVGL 原始缓冲区，字节序反了 → 红蓝通道错位、绿色被拆碎 → 杂色像素。

**修复**：sdkconfig 加 `CONFIG_LV_COLOR_16_SWAP=y`，让 LVGL 在写缓冲区时自动交换高低字节，输出大端序 RGB565。

### 学习建议

- **COLOR_16_SWAP 的使用场景**：取决于两件事——MCU 是大端还是小端、屏幕控制器期望什么序。ESP32 小端 + 大部分 SPI 屏期望大端 = 需要 swap。如果用了 esp_lcd 框架，它可能在驱动层帮你做了转换，但裸 SPI 传 LVGL buffer 就要自己注意。
- **判断方法**：白色(0xFFFF)无所谓，但红色(0xF800)如果显示成蓝色(0x001F)，就是字节反了。这是经典的 RGB565 swap 问题诊断信号。

---

### Q7: 启用 ESP32-S3 内置 8MB PSRAM

**背景**：ESP32-S3 芯片内部封装了 8MB PSRAM，但默认不启用，一直闲置。WiFi、HTTP、LVGL 全挤在 512KB 片内 SRAM。

**操作**：sdkconfig 加 6 行：

```
CONFIG_SPIRAM=y                      # 总开关
CONFIG_SPIRAM_MODE_OCT=y             # ESP32-S3 用 Octal 模式
CONFIG_SPIRAM_SPEED_80M=y            # 80MHz 时钟
CONFIG_SPIRAM_USE_CAPS_ALLOC=y       # 允许 heap_caps_malloc 用 PSRAM
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=y # malloc() 默认走内部，需要 PSRAM 时手动指定
CONFIG_SPIRAM_IGNORE_NOTFOUND=y      # 没检测到 PSRAM 也不报错
```

**效果**：片内 512KB SRAM 留给 LVGL 渲染 buffer 和任务栈（低延迟），WiFi/lwIP/HTTP/JSON 大块内存自动溢出到 8MB PSRAM。

**注意**：必须 `idf.py fullclean` 后重新编译，因为 PSRAM 会影响链接脚本和内存布局。

### 学习建议

- **ESP32-S3 的内存架构**：
  - 片内 SRAM (~512KB)：低延迟，DMA 可访问，给 LVGL buffer、任务栈
  - 片内 PSRAM (2MB/8MB)：容量大但稍慢，给 WiFi 协议栈、文件缓存、JSON 解析
  - Flash (2-16MB)：最慢，给固件代码、文件系统
- **分配策略**：
  - `malloc()` / `lv_mem_alloc()` → 默认内部，满了自动用 PSRAM（`MALLOC_ALWAYSINTERNAL=y` 时不会，需要指定 `MALLOC_CAP_SPIRAM`）
  - `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)` → 明确要 PSRAM
  - `heap_caps_malloc(size, MALLOC_CAP_INTERNAL)` → 明确要片内
  - `heap_caps_malloc(size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL)` → DMA buffer（必须片内）
- **验证 PSRAM**：启动后看 log 里 `esp_psram: Found 8MB PSRAM device` 或代码调用 `esp_psram_get_size()`。
- **配置生效条件**：改 PSRAM 相关 config 必须 fullclean，否则旧 build 里的链接脚本不会更新。

---
