# AlphaPi Zero 蓝牙翻页器（Arduino IDE 版本）

## 项目说明

这是 AlphaPi Zero蓝牙翻页器的 Arduino IDE 版本，基于 ESP32-C3 开发。支持 5 种蓝牙键位模式和 1 种 KOReader WiFi 控制模式，可通过摇晃设备翻页，支持 Web 配置和休眠省电。


## 功能特性

- ✅ 5 种蓝牙键位模式（PageUp/Down、方向键、音量、上下曲、播放控制）
- ✅ KOReader 模式（通过 WiFi HTTP 请求控制 KOReader 电子书阅读器）
- ✅ WiFi/蓝牙按模式智能切换
- ✅ 摇晃翻页（3次取中位数 + total算法 + 3状态机）
- ✅ 长按 B 开关摇晃翻页功能
- ✅ 长按 C 对调翻页方向（play 模式下不生效）
- ✅ play 模式下摇晃只触发播放暂停
- ✅ 短按 A 切换翻页箭头显示开关
- ✅ 模式启用/关闭（Web 页面配置，关闭后切换时跳过）
- ✅ 休眠功能（可配置时长，单击任意按键唤醒）
- ✅ Web 配置页面（摇晃参数、模式启用、休眠时间、WiFi STA、KOReader 配置）
- ✅ AP 热点 1 分钟无设备连接自动关闭
- ✅ AP 有设备连接时不进入休眠
- ✅ 5x5 LED 点阵屏幕显示各模式图标和状态

## 硬件参数

| 组件 | 参数 |
|------|------|
| 主控 | ESP32-C3 |
| 点阵屏幕 | 5x5 LED，UART1，波特率 460929，TX=GPIO8, RX=GPIO9 |
| 加速度计 | SC7A20，SoftI2C，地址 0x18，SDA=GPIO6, SCL=GPIO7 |
| 按键 A | GPIO10，下拉输入，按下高电平 |
| 按键 B | GPIO1，下拉输入，按下高电平 |
| 按键 C | GPIO3，下拉输入，按下高电平 |

## 按键功能

| 按键 | 短按 | 长按 |
|------|------|------|
| A 键 | 切换翻页箭头显示开关 | 切换键位模式（蓝牙模式间循环） |
| B 键 | 下一页/下一曲/音量+（受方向对调影响） | 开关摇晃翻页功能 |
| C 键 | 上一页/上一曲/音量-（受方向对调影响） | 对调翻页方向（play 模式下不生效） |
| A+B 同时长按 | - | 切换到/离开 koreader 模式 |

## 模式说明

### 蓝牙模式（5种，可在 Web 页面启用/关闭）

| 模式 | B 键（短按） | C 键（短按） | 摇晃触发 |
|------|-------------|-------------|---------|
| page | PageDown（下一页） | PageUp（上一页） | 同按键 |
| arrow | Right（右） | Left（左） | 同按键 |
| media | 音量+ | 音量- | 同按键 |
| music | 下一曲 | 上一曲 | 同按键 |
| play | 停止 | 播放暂停 | 只触发播放暂停 |

### KOReader 模式（WiFi 控制）

- 通过 WiFi HTTP 请求控制 KOReader 电子书阅读器
- 进入此模式后自动关闭蓝牙，开启 WiFi 热点
- 模式图标：居中的字母 K
- B 键下一页，C 键上一页（受方向对调影响）
- 摇晃也可翻页
- 长按 A 键显示 koreader 图标（字母 K）
- 同时长按 A+B 返回到之前的蓝牙模式
- AP 热点名称："AlphaPi-Config"，访问地址：192.168.4.1
- AP 热点 1 分钟无设备连接自动关闭，短按 A 键可重新打开

## 摇晃检测

### 算法
- 3 次读取取中位数过滤噪声
- total = abs(dx) + abs(dy) + abs(dz)（三轴差值之和）
- 3 状态机：IDLE → SHAKING → WAIT_QUIET
- 摇晃结束后静止一段时间才触发翻页，避免一次摇晃触发多次

### 默认参数
| 参数 | 默认值 | 说明 |
|------|--------|------|
| 摇晃阈值 | 3000 | 三轴差值之和超过此值算摇晃中 |
| 最小摇晃时长 | 100ms | 摇晃至少持续这么久才算有效摇晃 |
| 静止时长 | 300ms | 摇晃结束后静止这么久才触发翻页 |
| 冷却时间 | 1000ms | 触发翻页后经过冷却时间才能再次触发 |

### 状态显示
- 摇晃开启：波浪线图标
- 摇晃关闭：叉号图标
- 方向对调：圆圈图标
- BLE 未连接时摇晃：显示等待图标（菱形）

## 休眠功能

- 默认 2 分钟无操作进入休眠（可在 Web 页面配置，最低 30 秒，0=不休眠）
- 进入休眠前闪烁 2 次 X 图标提示
- 使用 ESP32 真正的轻量级睡眠（`esp_light_sleep_start()`），最大程度省电
- 休眠时关闭屏幕、蓝牙、WiFi，系统完全暂停
- 通过 GPIO 唤醒：单击 A/B/C 任意按键即可唤醒（高电平触发）
- 唤醒后自动重新初始化屏幕（UART）、加速度计（I2C）、蓝牙/WiFi
- 唤醒后显示当前模式图标 2 秒，然后正常工作
- AP 有设备连接时不进入休眠（确保 Web 配置稳定）

## Web 配置

### 连接方式
1. 设备开启后（koreader 模式或手动开启 AP），手机连接 WiFi 热点 "AlphaPi-Config"
2. 浏览器访问 192.168.4.1

### 可配置项
1. **摇晃检测设置**
   - 启用/关闭摇晃
   - 摇晃阈值（1000-10000）
   - 最小摇晃时长（50-500ms）
   - 静止时长（100-1000ms）
   - 摇晃冷却时间（>=500ms）

2. **休眠设置**
   - 休眠超时时间（>=30秒，0=不休眠）
   - 进入休眠前闪烁2次X图标提示
   - 单击A/B/C任意按键即可唤醒
   - AP有设备连接时不进入休眠

3. **模式启用设置**
   - Page 模式启用/关闭
   - Arrow 模式启用/关闭
   - Media 模式启用/关闭
   - Music 模式启用/关闭
   - Play 模式启用/关闭
   - 关闭的模式在长按 A 切换时会跳过

4. **WiFi STA 设置（用于 KOReader 模式）**
   - WiFi SSID
   - WiFi 密码

5. **KOReader 设置**
   - KOReader IP 地址
   - KOReader 端口（默认 8080）
   - 下一页 HTTP 指令（默认 GotoViewRel/1）
   - 上一页 HTTP 指令（默认 GotoViewRel/-1）

### KOReader 常用 HTTP 指令
- 翻页：GotoViewRel/1（下一页）、GotoViewRel/-1（上一页）
- 亮度：IncreaseFlIntensity/5（加亮度）、IncreaseFlIntensity/-5（减亮度）
- 夜间：ToggleNightMode（切换夜间模式）
- 书签：ToggleBookmark（添加/删除书签）
- 字体：IncreaseFontSize（增大字号）、DecreaseFontSize（减小字号）
- 查看所有指令：浏览器访问 http://<KOReader IP>:<端口>/koreader/event/

## 文件结构

```
AlphaPi_PageTurner/
├── AlphaPi_PageTurner.ino    # 主程序（完整功能版）
├── MatrixDisplay.h            # 点阵显示驱动头文件
├── MatrixDisplay.cpp          # 点阵显示驱动实现（20+种预定义图标）
├── SC7A20.h                   # 加速度计驱动头文件
├── SC7A20.cpp                 # 加速度计驱动实现（3次取中位数+total算法+3状态机）
├── Buttons.h                  # 按键处理头文件
├── Buttons.cpp                # 按键处理实现（20ms消抖、短按/长按检测）
├── BleHid.h                   # BLE HID 头文件（使用 HijelHID_BLEKeyboard 库，对象方式管理）
├── BleHid.cpp                 # BLE HID 实现（键盘键、媒体键、连接状态回调）
├── WebConfig.h                # Web 配置头文件
├── WebConfig.cpp              # Web 配置实现（AP模式、参数保存、KOReader HTTP请求）
└── README.md                  # 说明文档
```

## 编译上传

### 1. 安装 Arduino IDE
- 下载并安装 Arduino IDE：https://www.arduino.cc/en/software

### 2. 安装 ESP32 开发板支持
1. 打开 Arduino IDE
2. 文件 → 首选项
3. 在"附加开发板管理器网址"中添加：
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. 工具 → 开发板 → 开发板管理器
5. 搜索 "esp32"，安装 "esp32 by Espressif Systems"

### 3. 安装依赖库
- HijelHID_BLEKeyboard（BLE HID 键盘库，T-vK 的库有 bug 不要用）

### 4. 选择开发板
- 工具 → 开发板 → ESP32 Arduino → ESP32C3 Dev Module

### 5. 配置参数
- 工具 → 端口 → 选择对应的串口号（如 COM3）
- 工具 → Upload Speed → 115200（注意：必须设为 115200，否则上传失败）
- 工具 → Flash Size → 4MB (32Mb)
- 工具 → Partition Scheme → Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)

### 6. 编译上传
1. 打开 `AlphaPi_PageTurner.ino`
2. 点击"验证"按钮编译
3. 点击"上传"按钮上传到设备

## 点阵屏幕通信协议

### 帧格式
```
[0x90, addr, len(data), data..., checksum]
```

### 校验和
```
checksum = (0x90 + addr + len(data) + sum(data)) & 0xFF
```

### 响应
- 3 字节，第 3 字节 = 0x05 表示成功

### 数据格式（列优先）
- 5 列数据，每列 1 字节
- 每列 5 位（对应 5 行），左移 3 位后发送
- bit7 = 第 1 行，bit6 = 第 2 行，bit5 = 第 3 行，bit4 = 第 4 行，bit3 = 第 5 行

## 注意事项

1. 点阵屏幕使用非标准波特率 460929，Arduino 的 HardwareSerial 支持此波特率
2. 点阵屏幕的 HT 协控 MCU 不需要先激活，直接发送帧就能工作
3. 从 koreader 模式切回蓝牙模式后，需要等待手机重新连接 BLE（通常 2-5 秒），此时摇晃会显示等待图标
4. 同时长按 A+B 切换模式时，确保两个键都完全松开后再进行其他操作，避免误触单键功能
5. 配置保存在 NVS 分区中，重启后保留
6. 串口调试可能无法使用（硬件限制），测试结果通过点阵屏幕显示
7. 休眠使用 ESP32 真正的轻量级睡眠（`esp_light_sleep_start()`），唤醒后所有外设（UART/I2C/BLE/WiFi）会自动重新初始化
8. 休眠唤醒后会显示当前模式图标 2 秒，等待 BLE 连接完成后即可正常使用翻页功能
9. Upload Speed 必须设为 115200，否则上传失败

## 参考资料

- 原厂 MicroPython 版本程序：`original_firmware/` 目录
- 用户提供的参考固件：`test.cpp`
- KOReader HTTP Inspector API：http://<KOReader IP>:<端口>/koreader/event/
