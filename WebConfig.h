/*
 * WebConfig.h - Web 配置功能
 * 
 * 功能：
 * - WiFi AP 模式（热点）
 * - Web 服务器（配置页面）
 * - 参数配置和保存
 */

#ifndef WEBCONFIG_H
#define WEBCONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// 模式定义
#define MODE_PAGE   0
#define MODE_ARROW  1
#define MODE_MEDIA  2
#define MODE_MUSIC  3
#define MODE_PLAY   4
#define MODE_KOREADER 5
#define MODE_CUSTOM 6
#define MODE_AUTO 7   // 自动翻页模式

// 按键动作定义
#define ACTION_NONE             0
#define ACTION_PAGE_UP          1
#define ACTION_PAGE_DOWN        2
#define ACTION_ARROW_UP         3
#define ACTION_ARROW_DOWN       4
#define ACTION_ARROW_LEFT       5
#define ACTION_ARROW_RIGHT      6
#define ACTION_VOLUME_UP        7
#define ACTION_VOLUME_DOWN      8
#define ACTION_MEDIA_PREV       9
#define ACTION_MEDIA_NEXT       10
#define ACTION_MEDIA_PLAYPAUSE  11
#define ACTION_MEDIA_STOP       12
#define ACTION_ENTER            13
#define ACTION_SPACE            14
#define ACTION_ESC              15
#define ACTION_TAB              16
#define ACTION_CUSTOM_B         17  // 自定义B键（发送customKeyB）
#define ACTION_CUSTOM_C         18  // 自定义C键（发送customKeyC）

// 自定义键类型
#define KEY_TYPE_KEYBOARD       0   // 键盘键（Keyboard Report）
#define KEY_TYPE_MEDIA          1   // 媒体键（Consumer Report）

struct DeviceConfig {
    // 设备设置
    char deviceName[32];
    uint8_t currentMode;
    // KO 模式返回目标（内部状态，持久化到 NVS 但不在 Web 页展示）
    // 解决 KO 模式下重启（Web 保存重启/深度睡眠唤醒/断电）后丢失"进入 KO 前的模式"的问题
    uint8_t lastNonKOMode;
    
    // 各模式键位配置（A短按、A长按、B短按、B长按、C短按、C长按）
    // 7个模式：0=page,1=arrow,2=media,3=music,4=play,5=koreader,6=custom
    uint8_t keyActions[7][6];
    
    // 自定义模式组合键（每个按键最多3个键同时按，0表示不使用）
    uint8_t customKeyB[3];  // 自定义模式B短按发送的组合键
    uint8_t customKeyBType[3];  // B键每个键的类型（0=键盘键，1=媒体键）
    uint8_t customKeyC[3];  // 自定义模式C短按发送的组合键
    uint8_t customKeyCType[3];  // C键每个键的类型（0=键盘键，1=媒体键）
    
    // 摇晃检测参数
    bool shakeEnable;
    uint8_t shakeAction;  // 摇晃触发的动作
    int shakeSens;              // 摇晃阈值（三轴差值之和超过此值算摇晃中，模式0使用）
    uint32_t shakeMinDurationMs; // 最小摇晃时长（摇晃至少持续这么久才算有效摇晃）
    uint32_t shakeMaxDurationMs; // 最大摇晃时长（摇晃持续超过这么久就算误操作，不触发翻页）
    uint32_t quietHoldMs;       // 静止时长（摇晃结束后静止这么久才触发翻页）
    uint32_t shakeCooldownMs;   // 冷却时间
    uint8_t shakeMode;          // 摇晃检测模式：0=三轴差值之和（原模式），1=各轴独立阈值
    int shakeThresholdX;        // X轴差值阈值（模式1使用）
    int shakeThresholdY;        // Y轴差值阈值（模式1使用）
    int shakeThresholdZ;        // Z轴差值阈值（模式1使用）
    // 以下参数已不再使用，保留兼容
    uint8_t shakeNeedCnt;
    uint32_t shakeWinMs;
    int quietSens;
    
    // 其他设置
    uint32_t longPressMs;
    bool directionSwap;  // 翻页方向对调
    bool pageDisplayEnable;  // 翻页屏幕显示开关
    uint32_t sleepTimeoutMs;  // 休眠超时时间（毫秒），0=不休眠
    bool accelDisplayEnable;  // Web页实时加速度数值显示开关

    // 射频发射功率设置
    int8_t bleTxPower;   // 蓝牙发射功率 dBm（-12/-9/-6/-3/0/3/6/9），默认 -6
    int8_t wifiTxPowerAP;   // WiFi 发射功率-KO AP子模式（0.25dBm 单位存储，8~78，20=5dBm）
    int8_t wifiTxPowerSTA;  // WiFi 发射功率-KO STA子模式（0.25dBm 单位存储，8~78，60=15dBm）
    uint8_t bleAdvMode;  // 蓝牙广播间隔：0=秒连(20-40ms，默认) 1=平衡(250ms) 2=省电(500ms)
    uint8_t deepSleepEnable;  // 睡眠模式：0=轻度睡眠(默认) 1=深度睡眠(唤醒后重启+蓝牙重连，仅B/C键可唤醒)
    
    // 模式启用状态（true=启用，false=关闭，切换时跳过）
    bool modePageEnable;    // page 模式
    bool modeArrowEnable;   // arrow 模式
    bool modeMediaEnable;   // media 模式
    bool modeMusicEnable;   // music 模式
    bool modePlayEnable;    // play 模式
    bool modeCustomEnable;  // custom 自定义模式
    bool modeAutoEnable;    // auto 自动翻页模式

    // 自动翻页模式设置
    uint8_t autoTargetMode;      // 自动翻页执行的键位模式（0-6，不含koreader）
    uint32_t autoIntervalMs;     // 自动翻页间隔（毫秒）
    uint32_t autoRandomMs;       // 随机延时上限（毫秒），每次触发额外加 0~该值 的随机延时
    
    // 游戏启用状态（true=启用，false=关闭，切换时跳过）
    bool gameSnakeEnable;       // 贪吃蛇
    bool gameCatchEnable;       // 接球
    bool gameDiceEnable;        // 摇色子
    bool gameStopwatchEnable;   // 秒表
    bool gameGomokuEnable;      // 井字棋
    bool gameFlappyEnable;      // 像素鸟
    bool gameRacingEnable;      // 赛车避障
    bool gameTetrisEnable;      // 俄罗斯方块（2x2简化版）
    
    // KOReader 配置
    char koreaderIP[16];
    uint16_t koreaderPort;
    char koreaderNextCmd[64];  // 下一页 HTTP 指令
    char koreaderPrevCmd[64];  // 上一页 HTTP 指令
    uint16_t koApPort;  // AP模式下的KOReader端口（默认8080）
};

class WebConfig {
public:
    WebConfig();
    
    // 初始化（加载配置、启动 WiFi 和 Web 服务器）
    void begin();
    
    // 只加载配置，不启动 WiFi 和 Web 服务器
    void loadConfigOnly();
    
    // 启动 WiFi 和 Web 服务器
    void startWiFi();
    
    // 停止 WiFi 和 Web 服务器
    void stopWiFi();
    
    // 处理客户端请求（需要在 loop 中持续调用）
    void handleClient();
    
    // 获取配置
    DeviceConfig* getConfig();
    
    // 保存配置
    void saveConfig();
    
    // 加载配置
    void loadConfig();
    
    // 获取 WiFi 状态
    bool isWiFiConnected();
    String getAPIP();
    String getSTAIP();
    
    // 执行按键动作（发送 BLE 键）
    void executeAction(uint8_t action);
    
    // 发送 HTTP 请求到 KOReader
    bool sendKoreaderRequest(const char* path);
    
    // 发送 KOReader 下一页/上一页
    bool sendKoreaderNext();
    bool sendKoreaderPrev();
    
    // 检查 WiFi STA 是否连接
    bool isSTAConnected();
    
    // KOReader AP 模式（手机直接连接翻页器热点）
    void setKOModeAP(bool apMode);

    // 按当前 KO 子模式（AP/STA）应用对应的 WiFi 发射功率，立即生效不需要重启
    void applyWifiTxPower();
    bool isKOModeAP();
    static const char* KO_AP_SSID;
    static const char* KO_AP_IP;  // 手机连接后的IP
    
    // 加速度计实时数值显示
    typedef void (*AccelReaderFunc)(int16_t &x, int16_t &y, int16_t &z);
    void setAccelReader(AccelReaderFunc reader);
    
private:
    WebServer* _server;
    Preferences _prefs;
    DeviceConfig _config;
    
    // WiFi 设置
    char _staSSID[32];
    char _staPassword[64];
    
    // KOReader AP 模式
    bool _koAPMode;
    
    // 加速度计读取函数
    AccelReaderFunc _accelReader;
    
    // HTML页面缓存（避免每次请求都重新生成大量字符串导致内存碎片）
    String _cachedPage;
    bool _pageCacheValid;
    
    // 生成配置页面 HTML
    String generateConfigPage();
    
    // 处理根路径
    void handleRoot();
    
    // 键值表页面
    void handleKeysPage();
    
    // 处理保存配置
    void handleSave();
    
    // 处理重启
    void handleReboot();
    
    // 加速度数值API
    void handleAccel();
    
    // 设置默认配置
    void setDefaultConfig();
};

#endif // WEBCONFIG_H
