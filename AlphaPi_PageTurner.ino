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
#include "SnakeGame.h"
#include "CatchGame.h"
#include "DiceGame.h"
#include "StopwatchGame.h"
#include "GomokuGame.h"
#include "FlappyGame.h"
#include "RacingGame.h"
#include "TetrisGame.h"

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

// 贪吃蛇游戏
SnakeGame snakeGame(display);

// 俄罗斯方块游戏
TetrisGame tetrisGame(display);

// 接球游戏
CatchGame catchGame(display);

// 摇色子游戏
DiceGame diceGame(display, accel);

// 秒表游戏
StopwatchGame stopwatchGame(display);

// 井字棋游戏
GomokuGame gomokuGame(display);

// 像素鸟游戏
FlappyGame flappyGame(display);

// 赛车避障游戏
RacingGame racingGame(display);

// 游戏模式
bool gameMode = false;
uint8_t currentGame = 0;  // 0=贪吃蛇, 1=接球, 2=摇色子, 3=秒表, 4=井字棋, 5=像素鸟, 6=赛车避障, 7=俄罗斯方块

// 同时长按 B+C 检测
bool bcBothDown = false;
uint32_t bcBothDownStartTime = 0;
bool bcLongPressTriggered = false;
bool bcSuppressSingle = false;  // 抑制 B/C 单键事件

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
bool pendingApplyModeWireless = false;  // 待执行的模式切换（延迟到loop里执行，避免按键处理函数卡顿）

uint32_t lastKRefreshTime = 0;  // 上次刷新K图标的时间
// WiFi AP 管理
uint32_t pendingApplyTime = 0;  // 待执行切换的开始时间（等K图标显示一会儿再执行）
bool wifiAPEnabled = false;
uint32_t apNoClientTimer = 0;  // AP 无设备连接计时
const uint32_t AP_NO_CLIENT_TIMEOUT = 60000;  // STA模式下1分钟无设备连接自动关闭AP热点

// BLE 状态
bool bleEnabled = false;

// 上一个非 koreader 模式（用于从 koreader 模式返回）
uint8_t lastNonKoreaderMode = MODE_PAGE;

// 获取下一个启用的蓝牙模式（跳过关闭的模式和KO模式）
uint8_t getNextEnabledMode(uint8_t currentMode) {
    DeviceConfig* config = webConfig.getConfig();
    // 蓝牙模式列表（注意：跳过MODE_KOREADER=5）
    const uint8_t btModes[] = {MODE_PAGE, MODE_ARROW, MODE_MEDIA, MODE_MUSIC, MODE_PLAY, MODE_CUSTOM};
    const bool modeEnabled[] = {
        config->modePageEnable,
        config->modeArrowEnable,
        config->modeMediaEnable,
        config->modeMusicEnable,
        config->modePlayEnable,
        config->modeCustomEnable
    };
    const int BT_MODE_COUNT = 6;
    
    // 找到当前模式在btModes中的索引
    int currentIndex = -1;
    for (int i = 0; i < BT_MODE_COUNT; i++) {
        if (btModes[i] == currentMode) {
            currentIndex = i;
            break;
        }
    }
    
    // 如果当前模式不在蓝牙模式列表中（比如在KO模式），从0开始
    if (currentIndex == -1) currentIndex = 0;
    
    // 循环找到下一个启用的模式
    for (int i = 0; i < BT_MODE_COUNT; i++) {
        currentIndex = (currentIndex + 1) % BT_MODE_COUNT;
        if (modeEnabled[currentIndex]) {
            return btModes[currentIndex];
        }
    }
    
    // 如果所有蓝牙模式都关闭了，返回当前模式
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
bool wasInGameModeBeforeSleep = false;  // 记录休眠前是否在游戏模式

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
    
    // 记录休眠前是否在游戏模式
    wasInGameModeBeforeSleep = gameMode;
    
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
    bcBothDown = false;
    bcLongPressTriggered = false;
    bcSuppressSingle = false;
    
    DeviceConfig* config = webConfig.getConfig();
    
    // 先初始化 BLE（最重要，确保翻页功能正常）
    if (config->currentMode != MODE_KOREADER) {
        enableBLE();
        delay(100);  // 等待 BLE 初始化
    }
    
    // 重新初始化屏幕（UART 在轻量级睡眠中可能被关闭）
    display.begin();
    
    // 初始化按键
    buttons.begin();
    
    accelReady = accel.begin();
    
    if (accelReady) {
        // 设置摇晃参数
        accel.setShakeSens(3000);
        accel.setShakeMinDurationMs(100);
        accel.setQuietDurationMs(300);
        accel.setShakeCooldownMs(1000);
    }
    
    // 显示当前模式图标 2 秒
    showCurrentModeIcon(2000);
    
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

// 显示当前模式图标（KO模式下根据AP模式显示不同图标）
void showCurrentModeIcon(uint32_t durationMs) {
    DeviceConfig* config = webConfig.getConfig();
    if (config->currentMode == MODE_KOREADER && webConfig.isKOModeAP()) {
        showIconWithTimeout("ko_ap", durationMs);
    } else {
        const char* modeIcons[] = {"page", "arrow", "media", "music", "play", "koreader", "custom"};
        showIconWithTimeout(modeIcons[config->currentMode], durationMs);
    }
}

// 读取加速度数值（供Web页实时显示使用）
void readAccelData(int16_t &x, int16_t &y, int16_t &z) {
    if (accelReady) {
        accel.readRaw(x, y, z);
    } else {
        x = 0;
        y = 0;
        z = 0;
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
// BLE 连接状态回调
void onBleConnection(bool connected) {
    bleConnected = connected;
    DeviceConfig* config = webConfig.getConfig();
    
    // KO 模式下不显示 BLE 连接状态图标（因为我们主动关闭了 BLE）
    if (config->currentMode == MODE_KOREADER) {
        return;
    }
    
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
    display.showIcon("clear");  // 清空屏幕，避免显示默认的P图标
    
    // 初始化按键
    buttons.begin();
    
    accelReady = accel.begin();
    
    
    if (accelReady) {
        // 设置摇晃参数（按照 Python 版逻辑）
        accel.setShakeSens(3000);              // 摇晃阈值
        accel.setShakeMinDurationMs(100);       // 最小摇晃时长
        accel.setQuietDurationMs(300);          // 静止时长
        accel.setShakeCooldownMs(1000);         // 冷却时间
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
    accel.setShakeMaxDurationMs(config->shakeMaxDurationMs); // 最大摇晃时长
    accel.setQuietHoldMs(config->quietHoldMs);            // 静止时长（映射到 quietDurationMs）
    accel.setShakeCooldownMs(config->shakeCooldownMs);    // 冷却时间
    accel.setShakeMode(config->shakeMode);                 // 摇晃检测模式
    accel.setShakeThresholdXYZ(config->shakeThresholdX, config->shakeThresholdY, config->shakeThresholdZ); // 各轴阈值
    
    // 设置加速度读取函数（供Web页实时显示使用）
    webConfig.setAccelReader(readAccelData);
    
    // 从配置加载按键长按时间
    buttons.A.setLongPressTime(config->longPressMs);
    buttons.B.setLongPressTime(config->longPressMs);
    buttons.C.setLongPressTime(config->longPressMs);
    
    // 检查当前模式是否被关闭，如果是则切换到第一个启用的蓝牙模式
    if (config->currentMode != MODE_KOREADER) {
        const uint8_t btModes[] = {MODE_PAGE, MODE_ARROW, MODE_MEDIA, MODE_MUSIC, MODE_PLAY, MODE_CUSTOM};
        const bool modeEnabled[] = {
            config->modePageEnable,
            config->modeArrowEnable,
            config->modeMediaEnable,
            config->modeMusicEnable,
            config->modePlayEnable,
            config->modeCustomEnable
        };
        // 检查当前模式是否被关闭
        bool currentEnabled = false;
        for (int i = 0; i < 6; i++) {
            if (btModes[i] == config->currentMode && modeEnabled[i]) {
                currentEnabled = true;
                break;
            }
        }
        if (!currentEnabled) {
            // 当前模式被关闭了，找到第一个启用的蓝牙模式
            for (int i = 0; i < 6; i++) {
                if (modeEnabled[i]) {
                    config->currentMode = btModes[i];
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
    showCurrentModeIcon(2000);
    
    // 初始化活动时间
    lastActivityTime = millis();
}

void loop() {
    // 更新按键状态（所有模式都需要）
    buttons.update();
    
    // ========== 同时长按 B+C 检测（进入/退出游戏模式）==========
    bool bDown = buttons.B.isDown();
    bool cDown = buttons.C.isDown();
    
    if (bDown && cDown) {
        if (!bcBothDown) {
            // 刚开始同时按下，抑制单键事件
            bcBothDown = true;
            bcBothDownStartTime = millis();
            bcLongPressTriggered = false;
            bcSuppressSingle = true;
        } else if (!bcLongPressTriggered && millis() - bcBothDownStartTime >= 800) {
            // 同时按下超过 800ms，触发同时长按（进入/退出游戏模式）
            bcLongPressTriggered = true;
            updateActivity();
            
            gameMode = !gameMode;
            
            if (gameMode) {
                // 进入游戏模式：关闭蓝牙和 WiFi，初始化游戏
                disableBLE();
                disableWiFi();
                screenOffDeadline = 0;  // 屏幕常亮
                
                // 检查当前游戏是否被关闭，如果是则切换到第一个启用的游戏
                DeviceConfig* config = webConfig.getConfig();
                bool gameEnabled[8] = {
                    config->gameSnakeEnable,
                    config->gameCatchEnable,
                    config->gameDiceEnable,
                    config->gameStopwatchEnable,
                    config->gameGomokuEnable,
                    config->gameFlappyEnable,
                    config->gameRacingEnable,
                    config->gameTetrisEnable
                };
                if (!gameEnabled[currentGame]) {
                    // 找到第一个启用的游戏
                    bool found = false;
                    for (int i = 0; i < 8; i++) {
                        if (gameEnabled[i]) {
                            currentGame = i;
                            found = true;
                            break;
                        }
                    }
                    // 如果全部关闭，默认贪吃蛇
                    if (!found) {
                        currentGame = 0;
                    }
                }
                
                switch (currentGame) {
                    case 0: snakeGame.begin(); break;
                    case 1: catchGame.begin(); break;
                    case 2: diceGame.begin(); break;
                    case 3: stopwatchGame.begin(); break;
                    case 4: gomokuGame.begin(); break;
                    case 5: flappyGame.begin(); break;
                    case 6: racingGame.begin(); break;
                    case 7: tetrisGame.begin(); break;
                }
                Serial.println("Entered game mode");
            } else {
                // 退出游戏模式：恢复原来的模式
                snakeGame.exit();
                DeviceConfig* config = webConfig.getConfig();
                applyModeWireless(config->currentMode);
                showCurrentModeIcon(2000);
                Serial.println("Exited game mode");
            }
            
            // 等待按键松开
            while (buttons.B.isDown() || buttons.C.isDown()) {
                buttons.update();
                delay(10);
            }
        }
    } else {
        // 有一个按键松开了，重置同时按下状态
        if (bcBothDown) {
            bcBothDown = false;
        } else if (bcSuppressSingle && !bDown && !cDown) {
            // 两个键都松开了，解除抑制
            bcSuppressSingle = false;
        }
    }
    
    // ========== 休眠管理（游戏模式和普通模式都遵守）==========
    DeviceConfig* config = webConfig.getConfig();
    
    // 检测是否超时进入休眠（AP 有设备连接时不进入休眠）
    // enterSleep() 会阻塞直到按键唤醒，唤醒后继续执行后面的代码
    bool apHasClient = wifiAPEnabled && (WiFi.softAPgetStationNum() > 0);
    if (!apHasClient && config->sleepTimeoutMs > 0 && millis() - lastActivityTime > config->sleepTimeoutMs) {
        enterSleep();
        // 从休眠中唤醒后，继续执行后面的代码
    }
    
    // ========== 游戏模式处理 ==========
    if (gameMode) {
        // 更新游戏
        switch (currentGame) {
            case 0: snakeGame.update(); break;
            case 1: catchGame.update(); break;
            case 2: diceGame.update(); break;
            case 3: stopwatchGame.update(); break;
            case 4: gomokuGame.update(); break;
            case 5: flappyGame.update(); break;
            case 6: racingGame.update(); break;
            case 7: tetrisGame.update(); break;
        }
        
        // 短按 A
        if (buttons.A.pressed() && !bcSuppressSingle) {
            updateActivity();
            switch (currentGame) {
                case 0: snakeGame.onButtonA(); break;
                case 1: catchGame.onButtonA(); break;
                case 2: diceGame.onButtonA(); break;
                case 3: stopwatchGame.onButtonA(); break;
                case 4: gomokuGame.onButtonA(); break;
                case 5: flappyGame.onButtonA(); break;
                case 6: racingGame.onButtonA(); break;
                case 7: tetrisGame.onButtonA(); break;
            }
        }
        
        // 短按 B
        if (buttons.B.pressed() && !bcSuppressSingle) {
            updateActivity();
            switch (currentGame) {
                case 0: snakeGame.onButtonB(); break;
                case 1: catchGame.onButtonB(); break;
                case 2: diceGame.onButtonB(); break;
                case 3: stopwatchGame.onButtonB(); break;
                case 4: gomokuGame.onButtonB(); break;
                case 5: flappyGame.onButtonB(); break;
                case 6: racingGame.onButtonB(); break;
                case 7: tetrisGame.onButtonB(); break;
            }
        }
        
        // 短按 C
        if (buttons.C.pressed() && !bcSuppressSingle) {
            updateActivity();
            switch (currentGame) {
                case 0: snakeGame.onButtonC(); break;
                case 1: catchGame.onButtonC(); break;
                case 2: diceGame.onButtonC(); break;
                case 3: stopwatchGame.onButtonC(); break;
                case 4: gomokuGame.onButtonC(); break;
                case 5: flappyGame.onButtonC(); break;
                case 6: racingGame.onButtonC(); break;
                case 7: tetrisGame.onButtonC(); break;
            }
        }
        
        // 长按 A：切换游戏（跳过被关闭的游戏）
        if (buttons.A.longPressed() && !bcSuppressSingle) {
            updateActivity();
            DeviceConfig* cfg = webConfig.getConfig();
            
            bool gameEnabled[8] = {
                cfg->gameSnakeEnable, cfg->gameCatchEnable, cfg->gameDiceEnable,
                cfg->gameStopwatchEnable, cfg->gameGomokuEnable, cfg->gameFlappyEnable,
                cfg->gameRacingEnable, cfg->gameTetrisEnable
            };
            
            uint8_t nextGame = currentGame;
            bool found = false;
            for (int i = 0; i < 8; i++) {
                nextGame = (nextGame + 1) % 8;
                if (gameEnabled[nextGame]) {
                    found = true;
                    break;
                }
            }
            if (!found) nextGame = 0;
            
            currentGame = nextGame;
            switch (currentGame) {
                case 0: snakeGame.begin(); break;
                case 1: catchGame.begin(); break;
                case 2: diceGame.begin(); break;
                case 3: stopwatchGame.begin(); break;
                case 4: gomokuGame.begin(); break;
                case 5: flappyGame.begin(); break;
                case 6: racingGame.begin(); break;
                case 7: tetrisGame.begin(); break;
            }
        }
        
        // 长按 B：重新开始当前游戏
        if (buttons.B.longPressed() && !bcSuppressSingle) {
            updateActivity();
            switch (currentGame) {
                case 0: snakeGame.begin(); break;
                case 1: catchGame.begin(); break;
                case 2: diceGame.begin(); break;
                case 3: stopwatchGame.begin(); break;
                case 4: gomokuGame.begin(); break;
                case 5: flappyGame.begin(); break;
                case 6: racingGame.begin(); break;
                case 7: tetrisGame.begin(); break;
            }
        }
        
        delay(10);
        return;  // 游戏模式下不执行原来的处理
    }
    
    // 定期检查 BLE 连接状态（只有蓝牙启用时才检查）
    if (bleEnabled) {
        bleHid.isConnected();
    }
    
    // 处理 Web 客户端请求（只有 WiFi 启用时才处理）
    if (wifiAPEnabled) {
        webConfig.handleClient();
        delay(10);  // 给WiFi协议栈更多处理时间，特别是STA模式下避免Web页面卡死
        
        // KO模式 + STA子模式：AP热点无设备连接1分钟后自动关闭AP（但保持STA连接）
        // KO模式 + AP子模式：保持AP热点开启
        // 非KO模式：WiFi已被applyModeWireless直接关闭
        if (config->currentMode == MODE_KOREADER && !webConfig.isKOModeAP()) {
            // STA模式：检查AP热点是否有设备连接
            uint8_t apClientCount = WiFi.softAPgetStationNum();
            if (apClientCount > 0) {
                // 有设备连接，重置计时
                apNoClientTimer = millis();
            } else {
                // 无设备连接，检查是否超时
                if (millis() - apNoClientTimer > AP_NO_CLIENT_TIMEOUT) {
                    Serial.println("KO STA mode: AP no client for 1 minute, closing AP hotspot (STA stays connected)");
                    // 只关闭AP热点，保持STA连接
                    WiFi.softAPdisconnect(true);
                    apNoClientTimer = millis();  // 重置计时，避免重复关闭
                }
            }
        } else if (config->currentMode == MODE_KOREADER && webConfig.isKOModeAP()) {
            // AP模式：确保AP热点开启（如果之前被STA模式关闭了）
            if (WiFi.getMode() == WIFI_STA) {
                Serial.println("KO AP mode: AP was closed, restarting AP hotspot");
                WiFi.mode(WIFI_AP_STA);
                IPAddress apIP(192, 168, 4, 1);
                IPAddress netMsk(255, 255, 255, 0);
                WiFi.softAPConfig(apIP, apIP, netMsk);
                WiFi.softAP("AlphaPi-Config");
                apNoClientTimer = millis();
            }
        }
    }
    
    // 检查是否有待执行的模式切换
    if (pendingApplyModeWireless) {
        pendingApplyModeWireless = false;
        DeviceConfig* config = webConfig.getConfig();
        applyModeWireless(config->currentMode);
        // 切换完成后再显示K图标（完全避免闪烁）
        showCurrentModeIcon(2000);
    }
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
                    // 如果上一个模式被关闭了，找到第一个启用的蓝牙模式
                    const uint8_t btModes[] = {MODE_PAGE, MODE_ARROW, MODE_MEDIA, MODE_MUSIC, MODE_PLAY, MODE_CUSTOM};
                    const bool modeEnabled[] = {
                        config->modePageEnable,
                        config->modeArrowEnable,
                        config->modeMediaEnable,
                        config->modeMusicEnable,
                        config->modePlayEnable,
                        config->modeCustomEnable
                    };
                    // 检查上一个模式是否被启用
                    bool lastEnabled = false;
                    for (int i = 0; i < 6; i++) {
                        if (btModes[i] == lastNonKoreaderMode && modeEnabled[i]) {
                            lastEnabled = true;
                            break;
                        }
                    }
                    if (lastEnabled) {
                        config->currentMode = lastNonKoreaderMode;
                    } else {
                        // 找到第一个启用的蓝牙模式
                        for (int i = 0; i < 6; i++) {
                            if (modeEnabled[i]) {
                                config->currentMode = btModes[i];
                                break;
                            }
                        }
                    }
                } else {
                    // 保存当前模式，切换到 koreader 模式
                    lastNonKoreaderMode = config->currentMode;
                    config->currentMode = MODE_KOREADER;
                    // 进入KO模式时默认STA模式（连接路由器）
                    webConfig.setKOModeAP(false);
                }
                webConfig.saveConfig();
                
                // 先显示图标（快速响应）
                // KO模式下根据AP模式显示不同图标
                if (config->currentMode == MODE_KOREADER && webConfig.isKOModeAP()) {
                    showIconWithTimeout("ko_ap", 0);  // 常亮，避免自动熄灭
                } else {
                }
                
                // 设置标志位，等loop里执行完切换后再显示K（完全避免闪烁）
                pendingApplyModeWireless = true;
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
        
        // 长按A：KO模式下切换AP/STA模式，其他模式下切换模式
        if (buttons.A.longPressed() && !abSuppressSingle) {
            updateActivity();
            if (config->currentMode == MODE_KOREADER) {
                // KO模式下：切换AP模式和STA模式
                bool newAPMode = !webConfig.isKOModeAP();
                webConfig.setKOModeAP(newAPMode);
                // 修复：如果WiFi之前因为无连接被关闭了，切换模式时重新启动WiFi
                if (!wifiAPEnabled) {
                    Serial.println("WiFi was closed, restarting WiFi for KO mode");
                    enableWiFi();
                }
                if (newAPMode) {
                    // AP模式：显示小K图标
                    showIconWithTimeout("ko_ap", 2000);
                    Serial.println("KO mode: AP mode enabled, connect to AlphaPi-Config WiFi");
                } else {
                    // STA模式：显示正常K图标
                    showIconWithTimeout("koreader", 2000);
                    Serial.println("KO mode: STA mode enabled, connecting to router");
                }
            } else {
                // 其他模式：切换模式
                config->currentMode = getNextEnabledMode(config->currentMode);
                webConfig.saveConfig();
                // 切换模式时同时切换 WiFi 和蓝牙状态
                applyModeWireless(config->currentMode);
                showCurrentModeIcon(2000);
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
                if (config->currentMode == MODE_CUSTOM) {
                    // 自定义键值模式：B键显示字母B
                    display.showPattern(ICON_LETTER_B);
                    screenOffDeadline = millis() + 500;
                } else {
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
                if (config->currentMode == MODE_CUSTOM) {
                    // 自定义键值模式：C键显示字母C
                    display.showPattern(ICON_LETTER_C);
                    screenOffDeadline = millis() + 500;
                } else {
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
                showCurrentModeIcon(2000);
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
