/*
 * AlphaPi_PageTurner.ino - AlphaPi 蓝牙翻页器（Arduino IDE 版本）
 * 
 * 硬件：
 * - ESP32-C3
 * - 5x5 LED 点阵（UART1，波特率 460929，TX=8, RX=9）
 * - SC7A20 加速度计（SoftI2C，地址 0x18，SDA=6, SCL=7）
 * - 按键 A=GPIO10, B=GPIO1, C=GPIO3（下拉输入，按下高电平）
 * 
 * 注意：串口调试可能无法使用，所有测试结果通过点阵屏幕显示。
 */

#include <Arduino.h>
#include <Wire.h>
#include "esp_sleep.h"
#include "MatrixDisplay.h"
#include "SC7A20.h"
#include "Buttons.h"
#include "BleHid.h"
#include "WebConfig.h"

// 点阵显示
MatrixDisplay display;

// 加速度计
SC7A20 accel;

// 按键
Buttons buttons;

// BLE HID
BleHid bleHid;

// Web 配置
WebConfig webConfig;

// 测试模式：
// 0 = 点阵测试（循环显示所有图标）
// 1 = 加速度计读数测试（通过屏幕显示变化方向）
// 2 = 摇晃检测测试（摇晃显示箭头和圆圈）
// 3 = 按键测试（短按/长按显示不同图标）
// 4 = BLE HID 测试（按键发送键盘键和媒体键）
// 5 = Web 配置测试（启动 Web 服务器，可通过网页配置参数）
// 6 = 完整功能测试（短按A切换屏幕显示，长按A切换模式，长按B开关摇晃，长按C切换方向）
int testMode = 6;

// 摇晃计数
int shakeCount = 0;

// 加速度计初始化状态
bool accelReady = false;

// BLE 连接状态
bool bleConnected = false;

// 屏幕超时管理
uint32_t screenOffDeadline = 0;  // 0 表示常亮，否则为自动熄灭的时间戳

// WiFi AP 管理
bool wifiAPEnabled = false;
uint32_t apNoClientTimer = 0;  // AP 无设备连接计时
const uint32_t AP_NO_CLIENT_TIMEOUT = 60000;  // 1 分钟无设备连接自动关闭热点

// BLE 状态
bool bleEnabled = false;

// 上一个非 koreader 模式（用于从 koreader 模式返回）
uint8_t lastNonKoreaderMode = MODE_PAGE;

// 获取下一个启用的模式（跳过关闭的模式）
uint8_t getNextEnabledMode(uint8_t currentMode) {
    DeviceConfig* config = webConfig.getConfig();
    bool modeEnabled[] = {
        config->modePageEnable,
        config->modeArrowEnable,
        config->modeMediaEnable,
        config->modeMusicEnable,
        config->modePlayEnable
    };
    
    // 最多循环 5 次，找到下一个启用的模式
    for (int i = 0; i < 5; i++) {
        currentMode = (currentMode + 1) % 5;
        if (modeEnabled[currentMode]) {
            return currentMode;
        }
    }
    
    // 如果所有模式都关闭了，返回当前模式
    return currentMode;
}

// 同时长按 A+B 检测
bool abBothDown = false;
uint32_t abBothDownStartTime = 0;
bool abLongPressTriggered = false;
bool abSuppressSingle = false;  // 抑制 A/B 单键事件

// 休眠管理
uint32_t lastActivityTime = 0;
bool isSleeping = false;

// 更新活动时间（有操作时调用）
void updateActivity() {
    lastActivityTime = millis();
}

// 进入休眠
void enterSleep() {
    if (isSleeping) return;
    
    // 进入休眠前闪烁2次 X 图标
    for (int i = 0; i < 2; i++) {
        display.showIcon("shake_off");  // X 图标
        delay(200);
        display.showIcon("clear");
        delay(200);
    }
    
    isSleeping = true;
    
    // 关闭屏幕
    display.showIcon("clear");
    screenOffDeadline = 0;
    
    // 关闭蓝牙
    if (bleEnabled) {
        bleHid.end();
        bleEnabled = false;
        bleConnected = false;
    }
    
    // 关闭 WiFi
    if (wifiAPEnabled) {
        webConfig.stopWiFi();
        wifiAPEnabled = false;
    }
    
    Serial.println("Entering light sleep mode");
    
    // 配置 GPIO 唤醒（按键是下拉输入，按下高电平，所以用高电平触发）
    gpio_wakeup_enable(GPIO_NUM_10, GPIO_INTR_HIGH_LEVEL);  // A 键
    gpio_wakeup_enable(GPIO_NUM_1, GPIO_INTR_HIGH_LEVEL);   // B 键
    gpio_wakeup_enable(GPIO_NUM_3, GPIO_INTR_HIGH_LEVEL);   // C 键
    esp_sleep_enable_gpio_wakeup();
    
    // 进入轻量级睡眠（唤醒后会从这里继续执行）
    esp_light_sleep_start();
    
    // ===== 从休眠中唤醒 =====
    Serial.println("Woke up from light sleep");
    
    isSleeping = false;
    updateActivity();
    
    // 重置同时长按状态标志位
    abBothDown = false;
    abLongPressTriggered = false;
    abSuppressSingle = false;
    
    DeviceConfig* config = webConfig.getConfig();
    
    // 先初始化 BLE（最重要，确保翻页功能正常）
    if (config->currentMode != MODE_KOREADER) {
        enableBLE();
        delay(100);  // 等待 BLE 初始化
    }
    
    // 重新初始化屏幕（UART 在轻量级睡眠中可能被关闭）
    display.begin();
    
    // 重新初始化加速度计（I2C 在轻量级睡眠中可能被关闭）
    accel.begin();
    accel.setShakeSens(config->shakeSens);
    accel.setShakeMinDurationMs(config->shakeMinDurationMs);
    accel.setQuietHoldMs(config->quietHoldMs);
    accel.setShakeCooldownMs(config->shakeCooldownMs);
    
    // 恢复 WiFi（koreader 模式下）
    if (config->currentMode == MODE_KOREADER) {
        enableWiFi();
    }
    
    // 等待按键松开，避免立即触发操作
    while (buttons.A.isDown() || buttons.B.isDown() || buttons.C.isDown()) {
        buttons.update();
        delay(10);
    }
    
    // 显示当前模式图标 2 秒
    const char* modeIcons[] = {"page", "arrow", "media", "music", "play", "koreader"};
    showIconWithTimeout(modeIcons[config->currentMode], 2000);
    
    Serial.println("Exiting sleep mode (light sleep wakeup)");
}

// 显示图标并设置自动熄灭时间
void showIconWithTimeout(const char* iconName, uint32_t durationMs) {
    display.showIcon(iconName);
    if (durationMs > 0) {
        screenOffDeadline = millis() + durationMs;
    } else {
        screenOffDeadline = 0;  // 常亮
    }
}

// 启用 WiFi（AP 热点 + STA 连接 + Web 服务器）
void enableWiFi() {
    if (wifiAPEnabled) return;
    
    // 通过 WebConfig 启动 WiFi 和 Web 服务器
    webConfig.startWiFi();
    wifiAPEnabled = true;
    apNoClientTimer = millis();
    
    Serial.println("WiFi enabled (AP + STA + Web server)");
}

// 禁用 WiFi
void disableWiFi() {
    if (!wifiAPEnabled && WiFi.getMode() == WIFI_OFF) return;
    
    // 通过 WebConfig 停止 WiFi 和 Web 服务器
    webConfig.stopWiFi();
    wifiAPEnabled = false;
    
    Serial.println("WiFi disabled");
}

// 启用蓝牙
void enableBLE() {
    if (bleEnabled) return;
    bleHid.begin("AlphaPi Turner");
    bleEnabled = true;
    Serial.println("BLE enabled");
}

// 禁用蓝牙
void disableBLE() {
    if (!bleEnabled) return;
    bleHid.end();
    bleEnabled = false;
    bleConnected = false;
    Serial.println("BLE disabled");
}

// 根据模式设置 WiFi 和蓝牙状态
void applyModeWireless(uint8_t mode) {
    if (mode == MODE_KOREADER) {
        // koreader 模式：关闭蓝牙，打开 WiFi
        disableBLE();
        enableWiFi();
    } else {
        // 其他模式：关闭 WiFi，打开蓝牙
        disableWiFi();
        enableBLE();
    }
    
    // 切换模式后更新活动时间，避免立即进入休眠
    updateActivity();
    
    // 注意：不要在这里重置 abSuppressSingle 和 abBothDown 标志位
    // 这些标志位由主循环中的按键检测逻辑管理，只有当两个键都松开后才会重置
    // 否则会导致同时长按 A+B 触发后，单键事件被误触发
}

// BLE 连接状态回调
void onBleConnection(bool connected) {
    bleConnected = connected;
    if (connected) {
        // 连接成功：显示连接图标，2 秒后自动熄灭
        showIconWithTimeout("connected", 2000);
    } else {
        // 断开连接：显示等待图标，常亮
        showIconWithTimeout("waiting", 0);
    }
}

void setup() {
    // 初始化串口（即使无法使用也初始化，不影响功能）
    Serial.begin(115200);
    delay(100);
    
    // 初始化点阵显示
    display.begin();
    
    // 先显示等待图标
    display.showIcon("waiting");
    delay(500);
    
    // 初始化按键
    buttons.begin();
    
    // 初始化加速度计（参考 test.cpp 的实现）
    accelReady = accel.begin();
    
    if (accelReady) {
        // 设置摇晃参数（按照 Python 版逻辑）
        accel.setShakeSens(3000);              // 摇晃阈值（三轴差值之和超过此值算摇晃中）
        accel.setShakeMinDurationMs(100);       // 最小摇晃时长（摇晃至少持续这么久才算有效摇晃）
        accel.setQuietDurationMs(300);          // 静止时长（摇晃结束后静止这么久才触发翻页）
        accel.setShakeCooldownMs(1000);         // 冷却时间
        
        // 初始化成功：显示 media 图标（大加号，表示成功）
        display.showIcon("media");
        delay(1000);
        display.showIcon("waiting");
    } else {
        // 初始化失败：显示叉号
        display.showIcon("shake_off");
        while (1) {
            delay(1000);  // 卡住，显示失败
        }
    }
    
    // 初始化 BLE HID（先设置回调，不启动）
    bleHid.setConnectionCallback(onBleConnection);
    
    // 只加载配置，不启动 WiFi
    webConfig.loadConfigOnly();
    
    // 从配置加载加速度计参数（按照 Python 版逻辑）
    DeviceConfig* config = webConfig.getConfig();
    accel.setShakeSens(config->shakeSens);              // 摇晃阈值
    accel.setShakeMinDurationMs(config->shakeMinDurationMs); // 最小摇晃时长
    accel.setQuietHoldMs(config->quietHoldMs);            // 静止时长（映射到 quietDurationMs）
    accel.setShakeCooldownMs(config->shakeCooldownMs);    // 冷却时间
    
    // 从配置加载按键长按时间
    buttons.A.setLongPressTime(config->longPressMs);
    buttons.B.setLongPressTime(config->longPressMs);
    buttons.C.setLongPressTime(config->longPressMs);
    
    // 检查当前模式是否被关闭，如果是则切换到第一个启用的模式
    if (config->currentMode != MODE_KOREADER) {
        bool modeEnabled[] = {
            config->modePageEnable,
            config->modeArrowEnable,
            config->modeMediaEnable,
            config->modeMusicEnable,
            config->modePlayEnable
        };
        if (!modeEnabled[config->currentMode]) {
            // 当前模式被关闭了，找到第一个启用的模式
            for (int i = 0; i < 5; i++) {
                if (modeEnabled[i]) {
                    config->currentMode = i;
                    break;
                }
            }
        }
    }
    
    // 根据模式设置 WiFi 和蓝牙状态
    applyModeWireless(config->currentMode);
    
    // 初始化上一个非 koreader 模式
    if (config->currentMode != MODE_KOREADER) {
        lastNonKoreaderMode = config->currentMode;
    }
    
    // 显示当前模式图标 2 秒后自动熄灭
    const char* modeIcons[] = {"page", "arrow", "media", "music", "play", "koreader"};
    showIconWithTimeout(modeIcons[config->currentMode], 2000);
    
    // 初始化活动时间
    lastActivityTime = millis();
}

void loop() {
    // 更新按键状态（所有模式都需要）
    buttons.update();
    
    // 休眠管理
    DeviceConfig* config = webConfig.getConfig();
    
    // 检测是否超时进入休眠（AP 有设备连接时不进入休眠）
    // enterSleep() 会阻塞直到按键唤醒，唤醒后继续执行后面的代码
    bool apHasClient = wifiAPEnabled && (WiFi.softAPgetStationNum() > 0);
    if (!apHasClient && config->sleepTimeoutMs > 0 && millis() - lastActivityTime > config->sleepTimeoutMs) {
        enterSleep();
        // 从休眠中唤醒后，继续执行后面的代码
    }
    
    // 定期检查 BLE 连接状态（只有蓝牙启用时才检查）
    if (bleEnabled) {
        bleHid.isConnected();
    }
    
    // 处理 Web 客户端请求（只有 WiFi 启用时才处理）
    if (wifiAPEnabled) {
        webConfig.handleClient();
        
        // 检查 AP 无设备连接，1 分钟后自动关闭热点
        uint8_t clientCount = WiFi.softAPgetStationNum();
        if (clientCount > 0) {
            // 有设备连接，重置计时
            apNoClientTimer = millis();
        } else {
            // 无设备连接，检查是否超时
            if (millis() - apNoClientTimer > AP_NO_CLIENT_TIMEOUT) {
                Serial.println("AP no client for 1 minute, closing AP");
                // 关闭 AP 热点，但保留 STA 连接
                WiFi.softAPdisconnect(true);
                wifiAPEnabled = false;
            }
        }
    }
    
    // 检查屏幕自动熄灭
    if (screenOffDeadline > 0 && millis() >= screenOffDeadline) {
        display.clear();
        screenOffDeadline = 0;
    }
    
    if (testMode == 0) {
        // ========== 点阵测试：循环显示所有图标 ==========
        const char* icons[] = {
            "page", "arrow", "media", "music", "play",
            "stop", "play_pause", "connected", "waiting",
            "arrow_left", "arrow_right", "volume_up", "volume_down",
            "shake_on", "shake_off", "direction_swap", "clear"
        };
        int numIcons = sizeof(icons) / sizeof(icons[0]);
        
        for (int i = 0; i < numIcons; i++) {
            display.showIcon(icons[i]);
            delay(800);
        }
        
        // 全灭 1 秒
        display.clear();
        delay(1000);
    }
    else if (testMode == 1) {
        // ========== 加速度计测试：通过屏幕显示数值 ==========
        static int16_t lastX = 0, lastY = 0, lastZ = 0;
        static bool firstRead = true;
        
        int16_t x, y, z;
        accel.readRaw(x, y, z);
        
        if (firstRead) {
            lastX = x;
            lastY = y;
            lastZ = z;
            firstRead = false;
            display.showIcon("waiting");
            delay(100);
            return;
        }
        
        int16_t dx = x - lastX;
        int16_t dy = y - lastY;
        int16_t dz = z - lastZ;
        
        lastX = x;
        lastY = y;
        lastZ = z;
        
        int16_t mag = max(max(abs(dx), abs(dy)), abs(dz));
        
        if (mag > 500) {
            if (abs(dx) >= abs(dy) && abs(dx) >= abs(dz)) {
                if (dx > 0) {
                    display.showIcon("arrow_right");
                } else {
                    display.showIcon("arrow_left");
                }
            } else if (abs(dy) >= abs(dx) && abs(dy) >= abs(dz)) {
                if (dy > 0) {
                    display.showIcon("volume_up");
                } else {
                    display.showIcon("volume_down");
                }
            } else {
                display.showIcon("media");
            }
            delay(200);
            display.showIcon("waiting");
        }
        
        delay(30);
    }
    else if (testMode == 2) {
        // ========== 摇晃检测测试 ==========
        if (accel.checkShakeEvent()) {
            shakeCount++;
            
            display.showIcon("arrow_right");
            delay(500);
            
            display.showIcon("direction_swap");
            delay(300);
            
            display.showIcon("waiting");
        }
        
        delay(50);
    }
    else if (testMode == 3) {
        // ========== 按键测试 ==========
        if (buttons.A.pressed()) {
            display.showIcon("page");
            delay(500);
            display.showIcon("waiting");
        }
        if (buttons.B.pressed()) {
            display.showIcon("media");
            delay(500);
            display.showIcon("waiting");
        }
        if (buttons.C.pressed()) {
            display.showIcon("music");
            delay(500);
            display.showIcon("waiting");
        }
        
        if (buttons.A.longPressed()) {
            display.showIcon("arrow_left");
            delay(500);
            display.showIcon("waiting");
        }
        if (buttons.B.longPressed()) {
            display.showIcon("shake_on");
            delay(500);
            display.showIcon("waiting");
        }
        if (buttons.C.longPressed()) {
            display.showIcon("direction_swap");
            delay(500);
            display.showIcon("waiting");
        }
        
        delay(10);
    }
    else if (testMode == 4) {
        // ========== BLE HID 测试 ==========
        // 短按：
        //   A -> Page Up（键盘键），显示右箭头
        //   B -> 音量加（媒体键），显示小加号
        //   C -> 播放暂停（媒体键），显示播放暂停
        // 长按：
        //   A -> Page Down（键盘键），显示左箭头
        //   B -> 音量减（媒体键），显示小减号
        //   C -> 下一曲（媒体键），显示大加号
        
        if (buttons.A.pressed()) {
            display.showIcon("arrow_right");
            bleHid.sendKey(KEY_PAGE_UP);
            delay(300);
            display.showIcon(bleConnected ? "connected" : "waiting");
        }
        if (buttons.B.pressed()) {
            display.showIcon("volume_up");
            bleHid.sendMedia(MEDIA_VOLUME_UP);
            delay(300);
            display.showIcon(bleConnected ? "connected" : "waiting");
        }
        if (buttons.C.pressed()) {
            display.showIcon("play_pause");
            bleHid.sendMedia(MEDIA_PLAY_PAUSE);
            delay(300);
            display.showIcon(bleConnected ? "connected" : "waiting");
        }
        
        if (buttons.A.longPressed()) {
            display.showIcon("arrow_left");
            bleHid.sendKey(KEY_PAGE_DOWN);
            delay(300);
            display.showIcon(bleConnected ? "connected" : "waiting");
        }
        if (buttons.B.longPressed()) {
            display.showIcon("volume_down");
            bleHid.sendMedia(MEDIA_VOLUME_DOWN);
            delay(300);
            display.showIcon(bleConnected ? "connected" : "waiting");
        }
        if (buttons.C.longPressed()) {
            display.showIcon("media");
            bleHid.sendMedia(MEDIA_NEXT_TRACK);
            delay(300);
            display.showIcon(bleConnected ? "connected" : "waiting");
        }
        
        delay(10);
    }
    else if (testMode == 5) {
        // ========== Web 配置测试 ==========
        // Web 服务器已在 setup() 中启动
        // 连接热点 AlphaPi-Config，访问 192.168.4.1 配置参数
        // 按键功能同 BLE HID 测试
        
        if (buttons.A.pressed()) {
            display.showIcon("arrow_right");
            bleHid.sendKey(KEY_PAGE_UP);
            delay(300);
            display.showIcon(bleConnected ? "connected" : "waiting");
        }
        if (buttons.B.pressed()) {
            display.showIcon("volume_up");
            bleHid.sendMedia(MEDIA_VOLUME_UP);
            delay(300);
            display.showIcon(bleConnected ? "connected" : "waiting");
        }
        if (buttons.C.pressed()) {
            display.showIcon("play_pause");
            bleHid.sendMedia(MEDIA_PLAY_PAUSE);
            delay(300);
            display.showIcon(bleConnected ? "connected" : "waiting");
        }
        
        if (buttons.A.longPressed()) {
            display.showIcon("arrow_left");
            bleHid.sendKey(KEY_PAGE_DOWN);
            delay(300);
            display.showIcon(bleConnected ? "connected" : "waiting");
        }
        if (buttons.B.longPressed()) {
            display.showIcon("volume_down");
            bleHid.sendMedia(MEDIA_VOLUME_DOWN);
            delay(300);
            display.showIcon(bleConnected ? "connected" : "waiting");
        }
        if (buttons.C.longPressed()) {
            display.showIcon("media");
            bleHid.sendMedia(MEDIA_NEXT_TRACK);
            delay(300);
            display.showIcon(bleConnected ? "connected" : "waiting");
        }
        
        delay(10);
    }
    else if (testMode == 6) {
        // ========== 完整功能版（和 Python 版一致）==========
        // 短按A：切换翻页箭头显示开关
        // 长按A：切换键位模式
        // 短按B：下一页/下一曲/音量+（受方向对调影响）
        // 长按B：开关摇晃翻页功能
        // 短按C：上一页/上一曲/音量-（受方向对调影响）
        // 长按C：对调翻页方向（play 模式下不生效）
        
        DeviceConfig* config = webConfig.getConfig();
        const char* modeIcons[] = {"page", "arrow", "media", "music", "play", "koreader"};
        
        // 同时长按 A+B 检测（更灵敏的方式）
        bool aDown = buttons.A.isDown();
        bool bDown = buttons.B.isDown();
        
        if (aDown && bDown) {
            if (!abBothDown) {
                // 刚开始同时按下，抑制单键事件
                abBothDown = true;
                abBothDownStartTime = millis();
                abLongPressTriggered = false;
                abSuppressSingle = true;
            } else if (!abLongPressTriggered && millis() - abBothDownStartTime >= config->longPressMs) {
                // 同时按下超过长按时间，触发同时长按
                abLongPressTriggered = true;
                updateActivity();
                
                // 切换到/离开 koreader 模式
                if (config->currentMode == MODE_KOREADER) {
                    // 从 koreader 模式返回上一个非 koreader 模式
                    // 如果上一个模式被关闭了，找到第一个启用的模式
                    bool modeEnabled[] = {
                        config->modePageEnable,
                        config->modeArrowEnable,
                        config->modeMediaEnable,
                        config->modeMusicEnable,
                        config->modePlayEnable
                    };
                    if (modeEnabled[lastNonKoreaderMode]) {
                        config->currentMode = lastNonKoreaderMode;
                    } else {
                        // 找到第一个启用的模式
                        for (int i = 0; i < 5; i++) {
                            if (modeEnabled[i]) {
                                config->currentMode = i;
                                break;
                            }
                        }
                    }
                } else {
                    // 保存当前模式，切换到 koreader 模式
                    lastNonKoreaderMode = config->currentMode;
                    config->currentMode = MODE_KOREADER;
                }
                webConfig.saveConfig();
                // 切换模式时同时切换 WiFi 和蓝牙状态
                applyModeWireless(config->currentMode);
                showIconWithTimeout(modeIcons[config->currentMode], 2000);
            }
        } else {
            // 有一个按键松开了，重置同时按下状态
            // 延迟一点再解除抑制，确保所有单键事件都被消耗掉
            if (abBothDown) {
                abBothDown = false;
                // 保持抑制状态一小段时间，确保松开时的单键事件也被抑制
            } else if (abSuppressSingle && !aDown && !bDown) {
                // 两个键都松开了，解除抑制
                abSuppressSingle = false;
            }
        }
        
        // 短按A：切换翻页箭头显示开关
        if (buttons.A.pressed() && !abSuppressSingle) {
            updateActivity();
            config->pageDisplayEnable = !config->pageDisplayEnable;
            webConfig.saveConfig();
            showIconWithTimeout(config->pageDisplayEnable ? "arrow_on" : "arrow_off", 2000);
        }
        
        // 长按A：切换模式（koreader模式下如果AP已关闭则重新打开AP）
        if (buttons.A.longPressed() && !abSuppressSingle) {
            updateActivity();
            if (config->currentMode == MODE_KOREADER) {
                if (!wifiAPEnabled) {
                    // koreader 模式下 AP 已关闭，长按 A 重新打开 AP
                    enableWiFi();
                    showIconWithTimeout("koreader", 2000);
                } else {
                    // AP 已开启，显示 koreader 图标
                    showIconWithTimeout("koreader", 2000);
                }
            } else {
                // 其他模式：切换模式
                config->currentMode = getNextEnabledMode(config->currentMode);
                webConfig.saveConfig();
                // 切换模式时同时切换 WiFi 和蓝牙状态
                applyModeWireless(config->currentMode);
                showIconWithTimeout(modeIcons[config->currentMode], 2000);
            }
        }
        
        // 短按B：下一页/下一曲/音量+（受方向对调影响）
        if (buttons.B.pressed() && !abSuppressSingle) {
            updateActivity();
            // B 键默认是 next，如果方向对调则变为 prev
            bool isNext = !config->directionSwap;
            
            if (config->currentMode == MODE_KOREADER) {
                // KOReader 模式：发送 HTTP 请求
                if (isNext) {
                    webConfig.sendKoreaderNext();
                } else {
                    webConfig.sendKoreaderPrev();
                }
            } else {
                // 其他模式：发送 BLE 键
                uint8_t action = isNext ? 
                    config->keyActions[config->currentMode][2] :  // B短按（next）
                    config->keyActions[config->currentMode][4];   // C短按（prev）
                if (action != ACTION_NONE) {
                    webConfig.executeAction(action);
                }
            }
            
            // 根据模式显示对应图标
            if (config->pageDisplayEnable) {
                const char* icon;
                if (config->currentMode == MODE_MEDIA) {
                    icon = isNext ? "volume_up" : "volume_down";
                } else if (config->currentMode == MODE_PLAY) {
                    icon = isNext ? "stop" : "play_pause";
                } else {
                    icon = isNext ? "arrow_right" : "arrow_left";
                }
                showIconWithTimeout(icon, 500);
            }
        }
        
        // 长按B：开关摇晃翻页功能
        if (buttons.B.longPressed() && !abSuppressSingle) {
            updateActivity();
            config->shakeEnable = !config->shakeEnable;
            webConfig.saveConfig();
            showIconWithTimeout(config->shakeEnable ? "shake_on" : "shake_off", 2000);
        }
        
        // 短按C：上一页/上一曲/音量-（受方向对调影响）
        if (buttons.C.pressed()) {
            updateActivity();
            // C 键默认是 prev，如果方向对调则变为 next
            bool isNext = config->directionSwap;
            
            if (config->currentMode == MODE_KOREADER) {
                // KOReader 模式：发送 HTTP 请求
                if (isNext) {
                    webConfig.sendKoreaderNext();
                } else {
                    webConfig.sendKoreaderPrev();
                }
            } else {
                // 其他模式：发送 BLE 键
                uint8_t action = isNext ? 
                    config->keyActions[config->currentMode][2] :  // B短按（next）
                    config->keyActions[config->currentMode][4];   // C短按（prev）
                if (action != ACTION_NONE) {
                    webConfig.executeAction(action);
                }
            }
            
            // 根据模式显示对应图标
            if (config->pageDisplayEnable) {
                const char* icon;
                if (config->currentMode == MODE_MEDIA) {
                    icon = isNext ? "volume_up" : "volume_down";
                } else if (config->currentMode == MODE_PLAY) {
                    icon = isNext ? "stop" : "play_pause";
                } else {
                    icon = isNext ? "arrow_right" : "arrow_left";
                }
                showIconWithTimeout(icon, 500);
            }
        }
        
        // 长按C：对调翻页方向（play模式下不生效）
        if (buttons.C.longPressed()) {
            updateActivity();
            if (config->currentMode != MODE_PLAY) {
                config->directionSwap = !config->directionSwap;
                webConfig.saveConfig();
                // 先显示对调成功的圆圈图标
                display.showIcon("direction_swap");
                delay(500);
                // 然后显示当前模式的图标
                showIconWithTimeout(modeIcons[config->currentMode], 2000);
            }
        }
        
        // 摇晃检测（和按键模式同步切换）
        if (config->shakeEnable && accel.checkShakeEvent()) {
            updateActivity();
            bool isNext = !config->directionSwap;  // 摇晃默认触发 next，方向对调则触发 prev
            
            if (config->currentMode == MODE_KOREADER) {
                // KOReader 模式：发送 HTTP 请求
                if (isNext) {
                    webConfig.sendKoreaderNext();
                } else {
                    webConfig.sendKoreaderPrev();
                }
            } else {
                // 其他模式：直接根据当前模式执行动作（避免 webConfig.executeAction 间接调用可能的问题）
                if (bleHid.isConnected()) {
                    switch (config->currentMode) {
                        case MODE_PAGE:
                            if (isNext) {
                                bleHid.sendKey(KEY_PAGE_DOWN);
                            } else {
                                bleHid.sendKey(KEY_PAGE_UP);
                            }
                            break;
                        case MODE_ARROW:
                            if (isNext) {
                                bleHid.sendKey(KEY_RIGHT);
                            } else {
                                bleHid.sendKey(KEY_LEFT);
                            }
                            break;
                        case MODE_MEDIA:
                            if (isNext) {
                                bleHid.sendMedia(MEDIA_VOLUME_UP);
                            } else {
                                bleHid.sendMedia(MEDIA_VOLUME_DOWN);
                            }
                            break;
                        case MODE_MUSIC:
                            if (isNext) {
                                bleHid.sendMedia(MEDIA_NEXT_TRACK);
                            } else {
                                bleHid.sendMedia(MEDIA_PREV_TRACK);
                            }
                            break;
                        case MODE_PLAY:
                            // play模式下摇晃只触发播放暂停
                            bleHid.sendMedia(MEDIA_PLAY_PAUSE);
                            break;
                        default:
                            break;
                    }
                }
            }
            
            // 翻页屏幕显示（所有模式都显示箭头）
            if (config->pageDisplayEnable) {
                const char* icon;
                if (config->currentMode == MODE_KOREADER) {
                    // koreader 模式显示箭头
                    icon = isNext ? "arrow_right" : "arrow_left";
                } else if (!bleHid.isConnected()) {
                    // BLE 未连接时显示等待图标，提醒用户
                    icon = "waiting";
                } else if (config->currentMode == MODE_PLAY) {
                    icon = "play_pause";
                } else if (config->currentMode == MODE_MEDIA) {
                    icon = isNext ? "volume_up" : "volume_down";
                } else {
                    // page、arrow、music 模式显示箭头
                    icon = isNext ? "arrow_right" : "arrow_left";
                }
                showIconWithTimeout(icon, 500);
            }
        }
        
        delay(10);
    }
}
