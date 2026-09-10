/*
 * WebConfig.h - Web 配置功能
 * 
 * 功能：
 * - WiFi AP 模式（热点）
 * - Web 服务器（配置页面）
 * - 参数配置和保存
 * 
 * 参考 test.cpp 的实现
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

struct DeviceConfig {
    // 设备设置
    char deviceName[32];
    uint8_t currentMode;
    
    // 各模式键位配置（A短按、A长按、B短按、B长按、C短按、C长按）
    uint8_t keyActions[5][6];
    
    // 摇晃检测参数
    bool shakeEnable;
    uint8_t shakeAction;  // 摇晃触发的动作
    int shakeSens;              // 摇晃阈值（三轴差值之和超过此值算摇晃中）
    uint32_t shakeMinDurationMs; // 最小摇晃时长（摇晃至少持续这么久才算有效摇晃）
    uint32_t quietHoldMs;       // 静止时长（摇晃结束后静止这么久才触发翻页）
    uint32_t shakeCooldownMs;   // 冷却时间
    // 以下参数已不再使用，保留兼容
    uint8_t shakeNeedCnt;
    uint32_t shakeWinMs;
    int quietSens;
    
    // 其他设置
    uint32_t longPressMs;
    bool directionSwap;  // 翻页方向对调
    bool pageDisplayEnable;  // 翻页屏幕显示开关
    uint32_t sleepTimeoutMs;  // 休眠超时时间（毫秒），0=不休眠
    
    // 模式启用状态（true=启用，false=关闭，切换时跳过）
    bool modePageEnable;    // page 模式
    bool modeArrowEnable;   // arrow 模式
    bool modeMediaEnable;   // media 模式
    bool modeMusicEnable;   // music 模式
    bool modePlayEnable;    // play 模式
    
    // KOReader 配置
    char koreaderIP[16];
    uint16_t koreaderPort;
    char koreaderNextCmd[64];  // 下一页 HTTP 指令
    char koreaderPrevCmd[64];  // 上一页 HTTP 指令
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
    
private:
    WebServer* _server;
    Preferences _prefs;
    DeviceConfig _config;
    
    // WiFi 设置
    char _staSSID[32];
    char _staPassword[64];
    
    // 生成配置页面 HTML
    String generateConfigPage();
    
    // 处理根路径
    void handleRoot();
    
    // 处理保存配置
    void handleSave();
    
    // 设置默认配置
    void setDefaultConfig();
};

#endif // WEBCONFIG_H
