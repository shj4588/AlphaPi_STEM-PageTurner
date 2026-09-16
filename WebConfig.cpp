/*
 * WebConfig.cpp - Web 配置功能实现
 */

#include "WebConfig.h"
#include "BleHid.h"

extern BleHid bleHid;

// KOReader AP 模式常量
const char* WebConfig::KO_AP_SSID = "AlphaPi-Config";
const char* WebConfig::KO_AP_IP = "192.168.4.2";

// 动作名称列表
const char* actionNames[] = {
    "无动作",
    "Page Up（上翻页）",
    "Page Down（下翻页）",
    "方向键上",
    "方向键下",
    "方向键左",
    "方向键右",
    "音量加",
    "音量减",
    "上一曲",
    "下一曲",
    "播放暂停",
    "停止",
    "回车",
    "空格",
    "ESC",
    "Tab"
};

WebConfig::WebConfig() {
    _server = nullptr;
    _koAPMode = false;
    _pageCacheValid = false;
    _accelReader = nullptr;
}

void WebConfig::setDefaultConfig() {
    // 设备设置
    strcpy(_config.deviceName, "AlphaPi Turner");
    _config.currentMode = MODE_PAGE;
    
    // 默认键位配置（和 Python 版一致）
    // PAGE 模式：B=PageDown（下一页），C=PageUp（上一页）
    _config.keyActions[MODE_PAGE][0] = ACTION_NONE;         // A 短按（切换显示开关，不执行按键）
    _config.keyActions[MODE_PAGE][1] = ACTION_NONE;         // A 长按（切换模式，不执行按键）
    _config.keyActions[MODE_PAGE][2] = ACTION_PAGE_DOWN;   // B 短按（下一页）
    _config.keyActions[MODE_PAGE][3] = ACTION_NONE;         // B 长按（开关摇晃，不执行按键）
    _config.keyActions[MODE_PAGE][4] = ACTION_PAGE_UP;     // C 短按（上一页）
    _config.keyActions[MODE_PAGE][5] = ACTION_NONE;         // C 长按（对调方向，不执行按键）
    
    // ARROW 模式：B=Right（右），C=Left（左）
    _config.keyActions[MODE_ARROW][0] = ACTION_NONE;
    _config.keyActions[MODE_ARROW][1] = ACTION_NONE;
    _config.keyActions[MODE_ARROW][2] = ACTION_ARROW_RIGHT;
    _config.keyActions[MODE_ARROW][3] = ACTION_NONE;
    _config.keyActions[MODE_ARROW][4] = ACTION_ARROW_LEFT;
    _config.keyActions[MODE_ARROW][5] = ACTION_NONE;
    
    // MEDIA 模式：B=音量+，C=音量-
    _config.keyActions[MODE_MEDIA][0] = ACTION_NONE;
    _config.keyActions[MODE_MEDIA][1] = ACTION_NONE;
    _config.keyActions[MODE_MEDIA][2] = ACTION_VOLUME_UP;
    _config.keyActions[MODE_MEDIA][3] = ACTION_NONE;
    _config.keyActions[MODE_MEDIA][4] = ACTION_VOLUME_DOWN;
    _config.keyActions[MODE_MEDIA][5] = ACTION_NONE;
    
    // MUSIC 模式：B=下一曲，C=上一曲
    _config.keyActions[MODE_MUSIC][0] = ACTION_NONE;
    _config.keyActions[MODE_MUSIC][1] = ACTION_NONE;
    _config.keyActions[MODE_MUSIC][2] = ACTION_MEDIA_NEXT;
    _config.keyActions[MODE_MUSIC][3] = ACTION_NONE;
    _config.keyActions[MODE_MUSIC][4] = ACTION_MEDIA_PREV;
    _config.keyActions[MODE_MUSIC][5] = ACTION_NONE;
    
    // PLAY 模式：B=停止，C=播放暂停
    _config.keyActions[MODE_PLAY][0] = ACTION_NONE;
    _config.keyActions[MODE_PLAY][1] = ACTION_NONE;
    _config.keyActions[MODE_PLAY][2] = ACTION_MEDIA_STOP;
    _config.keyActions[MODE_PLAY][3] = ACTION_NONE;
    _config.keyActions[MODE_PLAY][4] = ACTION_MEDIA_PLAYPAUSE;
    _config.keyActions[MODE_PLAY][5] = ACTION_NONE;
    
    // CUSTOM 自定义模式：B=发送customKeyB，C=发送customKeyC
    _config.keyActions[MODE_CUSTOM][0] = ACTION_NONE;
    _config.keyActions[MODE_CUSTOM][1] = ACTION_NONE;
    _config.keyActions[MODE_CUSTOM][2] = ACTION_CUSTOM_B;
    _config.keyActions[MODE_CUSTOM][3] = ACTION_NONE;
    _config.keyActions[MODE_CUSTOM][4] = ACTION_CUSTOM_C;
    _config.keyActions[MODE_CUSTOM][5] = ACTION_NONE;
    
    // 自定义模式默认组合键（HID Usage ID，默认PageDown/PageUp，0表示不使用）
    _config.customKeyB[0] = 0x4E;  // PageDown
    _config.customKeyB[1] = 0;
    _config.customKeyB[2] = 0;
    _config.customKeyBType[0] = KEY_TYPE_KEYBOARD;
    _config.customKeyBType[1] = KEY_TYPE_KEYBOARD;
    _config.customKeyBType[2] = KEY_TYPE_KEYBOARD;
    _config.customKeyC[0] = 0x4B;  // PageUp
    _config.customKeyC[1] = 0;
    _config.customKeyC[2] = 0;
    _config.customKeyCType[0] = KEY_TYPE_KEYBOARD;
    _config.customKeyCType[1] = KEY_TYPE_KEYBOARD;
    _config.customKeyCType[2] = KEY_TYPE_KEYBOARD;
    
    // 摇晃检测参数
    _config.shakeEnable = true;
    _config.shakeAction = ACTION_PAGE_DOWN;
    _config.shakeSens = 2500;
    _config.shakeNeedCnt = 4;
    _config.shakeWinMs = 2000;
    _config.quietSens = 600;
    _config.quietHoldMs = 150;
    _config.shakeCooldownMs = 1000;
    // 各轴独立阈值模式
    _config.shakeMode = 0;  // 默认使用原模式（三轴差值之和）
    _config.shakeThresholdX = 1000;
    _config.shakeThresholdY = 1000;
    _config.shakeThresholdZ = 1000;
    
    // 其他设置
    _config.longPressMs = 800;
    _config.directionSwap = false;
    _config.pageDisplayEnable = true;  // 默认开启翻页屏幕显示
    _config.sleepTimeoutMs = 120000;  // 默认 2 分钟休眠
    _config.accelDisplayEnable = false;  // 默认关闭实时加速度数值显示
    
    // 模式启用状态（默认全部启用）
    _config.modePageEnable = true;
    _config.modeArrowEnable = true;
    _config.modeMediaEnable = true;
    _config.modeMusicEnable = true;
    _config.modePlayEnable = true;
    _config.modeCustomEnable = true;
    
    // 游戏启用状态（默认全部启用）
    _config.gameSnakeEnable = true;
    _config.gameCatchEnable = true;
    _config.gameDiceEnable = true;
    _config.gameStopwatchEnable = true;
    _config.gameGomokuEnable = true;
    _config.gameFlappyEnable = true;
    _config.gameRacingEnable = true;
    _config.gameTetrisEnable = true;
    
    // KOReader 默认配置
    strcpy(_config.koreaderIP, "192.168.2.107");
    _config.koreaderPort = 8080;
    strcpy(_config.koreaderNextCmd, "GotoViewRel/1");
    strcpy(_config.koreaderPrevCmd, "GotoViewRel/-1");
    _config.koApPort = 8080;  // AP模式下默认端口8080
    
    // WiFi STA 默认配置
    strcpy(_staSSID, "");
    strcpy(_staPassword, "");
}

void WebConfig::loadConfig() {
    _prefs.begin("pageturner", false);
    
    // 设备设置
    String name = _prefs.getString("dev_name", "");
    if (name.length() > 0) {
        strncpy(_config.deviceName, name.c_str(), sizeof(_config.deviceName) - 1);
    }
    _config.currentMode = _prefs.getUChar("mode", MODE_PAGE);
    
    // 键位配置（7个模式：0-6）
    for (int m = 0; m < 7; m++) {
        for (int k = 0; k < 6; k++) {
            String key = "key_" + String(m) + "_" + String(k);
            _config.keyActions[m][k] = _prefs.getUChar(key.c_str(), _config.keyActions[m][k]);
        }
    }
    
    // 自定义模式组合键（每个按键3个键值，0表示不使用）
    _config.customKeyB[0] = _prefs.getUChar("custom_key_b_0", 0x4E);
    _config.customKeyB[1] = _prefs.getUChar("custom_key_b_1", 0);
    _config.customKeyB[2] = _prefs.getUChar("custom_key_b_2", 0);
    _config.customKeyBType[0] = _prefs.getUChar("ckb_type0", KEY_TYPE_KEYBOARD);
    _config.customKeyBType[1] = _prefs.getUChar("ckb_type1", KEY_TYPE_KEYBOARD);
    _config.customKeyBType[2] = _prefs.getUChar("ckb_type2", KEY_TYPE_KEYBOARD);
    _config.customKeyC[0] = _prefs.getUChar("custom_key_c_0", 0x4B);
    _config.customKeyC[1] = _prefs.getUChar("custom_key_c_1", 0);
    _config.customKeyC[2] = _prefs.getUChar("custom_key_c_2", 0);
    _config.customKeyCType[0] = _prefs.getUChar("ckc_type0", KEY_TYPE_KEYBOARD);
    _config.customKeyCType[1] = _prefs.getUChar("ckc_type1", KEY_TYPE_KEYBOARD);
    _config.customKeyCType[2] = _prefs.getUChar("ckc_type2", KEY_TYPE_KEYBOARD);
    
    // 摇晃检测参数（按照 Python 版逻辑）
    _config.shakeEnable = _prefs.getBool("shake_en", true);
    _config.shakeAction = _prefs.getUChar("shake_act", ACTION_PAGE_DOWN);
    _config.shakeSens = _prefs.getInt("shake_sens", 3000);              // 摇晃阈值
    _config.shakeMinDurationMs = _prefs.getUInt("shake_min_dur", 100);   // 最小摇晃时长
    _config.quietHoldMs = _prefs.getUInt("quiet_hold", 300);             // 静止时长
    _config.shakeCooldownMs = _prefs.getUInt("shake_cd", 1000);          // 冷却时间
    // 各轴独立阈值模式
    _config.shakeMode = _prefs.getUChar("shake_mode", 0);                 // 0=三轴差值之和，1=各轴独立阈值
    _config.shakeThresholdX = _prefs.getInt("shake_th_x", 1000);         // X轴阈值
    _config.shakeThresholdY = _prefs.getInt("shake_th_y", 1000);         // Y轴阈值
    _config.shakeThresholdZ = _prefs.getInt("shake_th_z", 1000);         // Z轴阈值
    // 以下参数已不再使用，保留兼容
    _config.shakeNeedCnt = _prefs.getUChar("shake_cnt", 4);
    _config.shakeWinMs = _prefs.getUInt("shake_win", 2000);
    _config.quietSens = _prefs.getInt("quiet_sens", 600);
    
    // 其他设置
    _config.longPressMs = _prefs.getUInt("long_press", 800);
    _config.directionSwap = _prefs.getBool("dir_swap", false);
    _config.pageDisplayEnable = _prefs.getBool("page_disp", true);
    _config.sleepTimeoutMs = _prefs.getUInt("sleep_timeout", 120000);
    _config.accelDisplayEnable = _prefs.getBool("accel_disp", false);
    
    // 模式启用状态
    _config.modePageEnable = _prefs.getBool("mode_page", true);
    _config.modeArrowEnable = _prefs.getBool("mode_arrow", true);
    _config.modeMediaEnable = _prefs.getBool("mode_media", true);
    _config.modeMusicEnable = _prefs.getBool("mode_music", true);
    _config.modePlayEnable = _prefs.getBool("mode_play", true);
    _config.modeCustomEnable = _prefs.getBool("mode_custom", true);
    
    // 游戏启用状态
    _config.gameSnakeEnable = _prefs.getBool("game_snake", true);
    _config.gameCatchEnable = _prefs.getBool("game_catch", true);
    _config.gameDiceEnable = _prefs.getBool("game_dice", true);
    _config.gameStopwatchEnable = _prefs.getBool("game_stopwatch", true);
    _config.gameGomokuEnable = _prefs.getBool("game_gomoku", true);
    _config.gameFlappyEnable = _prefs.getBool("game_flappy", true);
    _config.gameRacingEnable = _prefs.getBool("game_racing", true);
    _config.gameTetrisEnable = _prefs.getBool("game_tetris", true);
    
    // KOReader 配置
    String koIP = _prefs.getString("ko_ip", "");
    if (koIP.length() > 0) {
        strncpy(_config.koreaderIP, koIP.c_str(), sizeof(_config.koreaderIP) - 1);
    }
    _config.koreaderPort = _prefs.getUShort("ko_port", 8080);
    _config.koApPort = _prefs.getUShort("ko_ap_port", 8080);
    String koNext = _prefs.getString("ko_next", "");
    if (koNext.length() > 0) {
        strncpy(_config.koreaderNextCmd, koNext.c_str(), sizeof(_config.koreaderNextCmd) - 1);
    }
    String koPrev = _prefs.getString("ko_prev", "");
    if (koPrev.length() > 0) {
        strncpy(_config.koreaderPrevCmd, koPrev.c_str(), sizeof(_config.koreaderPrevCmd) - 1);
    }
    
    // WiFi STA 配置
    String staSSID = _prefs.getString("sta_ssid", "");
    if (staSSID.length() > 0) {
        strncpy(_staSSID, staSSID.c_str(), sizeof(_staSSID) - 1);
    }
    String staPass = _prefs.getString("sta_pass", "");
    if (staPass.length() > 0) {
        strncpy(_staPassword, staPass.c_str(), sizeof(_staPassword) - 1);
    }
    
    // 强制重置自定义模式的键位配置（防止旧固件数组越界时保存的垃圾值导致B键无效）
    _config.keyActions[MODE_CUSTOM][0] = ACTION_NONE;
    _config.keyActions[MODE_CUSTOM][1] = ACTION_NONE;
    _config.keyActions[MODE_CUSTOM][2] = ACTION_CUSTOM_B;
    _config.keyActions[MODE_CUSTOM][3] = ACTION_NONE;
    _config.keyActions[MODE_CUSTOM][4] = ACTION_CUSTOM_C;
    _config.keyActions[MODE_CUSTOM][5] = ACTION_NONE;
    
    // 兼容旧固件：如果旧的单键值字段存在，迁移到新的数组格式
    if (_prefs.isKey("custom_key_b")) {
        uint8_t oldVal = _prefs.getUChar("custom_key_b", 0);
        if (oldVal != 0 && _config.customKeyB[0] == 0x4E) {
            _config.customKeyB[0] = oldVal;
        }
    }
    if (_prefs.isKey("custom_key_c")) {
        uint8_t oldVal = _prefs.getUChar("custom_key_c", 0);
        if (oldVal != 0 && _config.customKeyC[0] == 0x4B) {
            _config.customKeyC[0] = oldVal;
        }
    }
    
    _prefs.end();
}

void WebConfig::saveConfig() {
    _pageCacheValid = false;  // 配置改变后使页面缓存失效
    _prefs.begin("pageturner", false);
    
    // 设备设置
    _prefs.putString("dev_name", _config.deviceName);
    _prefs.putUChar("mode", _config.currentMode);
    
    // 键位配置（7个模式：0-6）
    for (int m = 0; m < 7; m++) {
        for (int k = 0; k < 6; k++) {
            String key = "key_" + String(m) + "_" + String(k);
            _prefs.putUChar(key.c_str(), _config.keyActions[m][k]);
        }
    }
    
    // 自定义模式组合键（每个按键3个键值）
    _prefs.putUChar("custom_key_b_0", _config.customKeyB[0]);
    _prefs.putUChar("custom_key_b_1", _config.customKeyB[1]);
    _prefs.putUChar("custom_key_b_2", _config.customKeyB[2]);
    _prefs.putUChar("ckb_type0", _config.customKeyBType[0]);
    _prefs.putUChar("ckb_type1", _config.customKeyBType[1]);
    _prefs.putUChar("ckb_type2", _config.customKeyBType[2]);
    _prefs.putUChar("custom_key_c_0", _config.customKeyC[0]);
    _prefs.putUChar("custom_key_c_1", _config.customKeyC[1]);
    _prefs.putUChar("custom_key_c_2", _config.customKeyC[2]);
    _prefs.putUChar("ckc_type0", _config.customKeyCType[0]);
    _prefs.putUChar("ckc_type1", _config.customKeyCType[1]);
    _prefs.putUChar("ckc_type2", _config.customKeyCType[2]);
    
    // 摇晃检测参数（按照 Python 版逻辑）
    _prefs.putBool("shake_en", _config.shakeEnable);
    _prefs.putUChar("shake_act", _config.shakeAction);
    _prefs.putInt("shake_sens", _config.shakeSens);
    _prefs.putUInt("shake_min_dur", _config.shakeMinDurationMs);
    _prefs.putUInt("quiet_hold", _config.quietHoldMs);
    _prefs.putUInt("shake_cd", _config.shakeCooldownMs);
    // 各轴独立阈值模式
    _prefs.putUChar("shake_mode", _config.shakeMode);
    _prefs.putInt("shake_th_x", _config.shakeThresholdX);
    _prefs.putInt("shake_th_y", _config.shakeThresholdY);
    _prefs.putInt("shake_th_z", _config.shakeThresholdZ);
    // 以下参数已不再使用，保留兼容
    _prefs.putUChar("shake_cnt", _config.shakeNeedCnt);
    _prefs.putUInt("shake_win", _config.shakeWinMs);
    _prefs.putInt("quiet_sens", _config.quietSens);
    
    // 其他设置
    _prefs.putUInt("long_press", _config.longPressMs);
    _prefs.putBool("dir_swap", _config.directionSwap);
    _prefs.putBool("page_disp", _config.pageDisplayEnable);
    _prefs.putUInt("sleep_timeout", _config.sleepTimeoutMs);
    _prefs.putBool("accel_disp", _config.accelDisplayEnable);
    
    // 模式启用状态
    _prefs.putBool("mode_page", _config.modePageEnable);
    _prefs.putBool("mode_arrow", _config.modeArrowEnable);
    _prefs.putBool("mode_media", _config.modeMediaEnable);
    _prefs.putBool("mode_music", _config.modeMusicEnable);
    _prefs.putBool("mode_play", _config.modePlayEnable);
    _prefs.putBool("mode_custom", _config.modeCustomEnable);
    
    // 游戏启用状态
    _prefs.putBool("game_snake", _config.gameSnakeEnable);
    _prefs.putBool("game_catch", _config.gameCatchEnable);
    _prefs.putBool("game_dice", _config.gameDiceEnable);
    _prefs.putBool("game_stopwatch", _config.gameStopwatchEnable);
    _prefs.putBool("game_gomoku", _config.gameGomokuEnable);
    _prefs.putBool("game_flappy", _config.gameFlappyEnable);
    _prefs.putBool("game_racing", _config.gameRacingEnable);
    _prefs.putBool("game_tetris", _config.gameTetrisEnable);
    
    // KOReader 配置
    _prefs.putString("ko_ip", _config.koreaderIP);
    _prefs.putUShort("ko_port", _config.koreaderPort);
    _prefs.putUShort("ko_ap_port", _config.koApPort);
    _prefs.putString("ko_next", _config.koreaderNextCmd);
    _prefs.putString("ko_prev", _config.koreaderPrevCmd);
    
    // WiFi STA 配置
    _prefs.putString("sta_ssid", _staSSID);
    _prefs.putString("sta_pass", _staPassword);
    
    _prefs.end();
}

DeviceConfig* WebConfig::getConfig() {
    return &_config;
}

void WebConfig::begin() {
    // 先设置默认配置
    setDefaultConfig();
    
    // 加载保存的配置
    loadConfig();
    
    // 启动 WiFi 和 Web 服务器
    startWiFi();
}

void WebConfig::loadConfigOnly() {
    // 先设置默认配置
    setDefaultConfig();
    
    // 加载保存的配置
    loadConfig();
}

void WebConfig::startWiFi() {
    // 初始化 WiFi
    // 优化：设置WiFi睡眠模式为NONE，减少延迟，提高Web响应速度
    // 这对STA模式尤其重要，避免WiFi睡眠导致TCP连接挂起
    WiFi.setSleep(false);
    
    // 同时开启AP和STA模式
    // AP模式：手机直连192.168.4.1，稳定不卡死
    // STA模式：连接路由器，通过路由器IP访问，需要额外优化
    WiFi.mode(WIFI_AP_STA);
    
    IPAddress apIP(192, 168, 4, 1);
    IPAddress netMsk(255, 255, 255, 0);
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP("AlphaPi-Config");
    
    // 尝试连接 STA WiFi（用于 KOReader 模式和Web配置）
    if (strlen(_staSSID) > 0) {
        Serial.print("尝试连接 STA WiFi: ");
        Serial.println(_staSSID);
        WiFi.setAutoReconnect(true);  // 启用自动重连，避免连接断开后TCP挂起
        WiFi.begin(_staSSID, _staPassword);
    }
    
    // 启动 Web 服务器
    if (_server == nullptr) {
        _server = new WebServer(80);
        _server->on("/", std::bind(&WebConfig::handleRoot, this));
        _server->on("/save", std::bind(&WebConfig::handleSave, this));
        _server->on("/keys", std::bind(&WebConfig::handleKeysPage, this));
        _server->on("/accel", std::bind(&WebConfig::handleAccel, this));
    }
    _server->begin();
    
    Serial.println("Web server started (AP+STA mode)");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
}

void WebConfig::stopWiFi() {
    if (_server) {
        _server->stop();
        delete _server;
        _server = nullptr;
    }
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi stopped");
}

void WebConfig::handleClient() {
    if (_server) {
        _server->handleClient();
    }
}

bool WebConfig::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WebConfig::getAPIP() {
    return WiFi.softAPIP().toString();
}

String WebConfig::getSTAIP() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return "未连接";
}

String WebConfig::generateConfigPage() {
    String html;
    html.reserve(25000);  // 预分配足够大的内存，避免多次重新分配导致内存碎片
    
    html += "<!DOCTYPE html>\n";
    html += "<html lang=\"zh-CN\">\n";
    html += "<head>\n";
    html += "<meta charset=\"UTF-8\">\n";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html += "<title>AlphaPi 翻页器配置</title>\n";
    html += "<style>\n";
    html += "body{font-family:Arial;margin:16px;max-width:800px;}\n";
    html += ".item{margin:10px 0;padding:10px;border:1px solid #ccc;border-radius:4px;}\n";
    html += "select,input{min-width:200px;padding:4px;margin:4px 0;}\n";
    html += ".note{background:#f7f7f7;padding:10px;border-radius:4px;margin-top:8px;font-size:14px;}\n";
    html += "h2{color:#333;}\n";
    html += "h3{color:#555;margin-top:0;}\n";
    html += "table{border-collapse:collapse;width:100%;}\n";
    html += "td,th{border:1px solid #ddd;padding:8px;text-align:left;}\n";
    html += "th{background-color:#f2f2f2;}\n";
    html += "</style>\n";
    html += "</head>\n<body>\n";
    html += "<h2>AlphaPi 翻页器配置 <span style='font-size:14px;color:#666;font-weight:normal;'>固件版本 V1.0.3</span></h2>\n";
    
    // WiFi 信息
    html += "<div class=\"item\">\n";
    html += "<h3>WiFi 信息</h3>\n";
    html += "<p>热点AP: AlphaPi-Config</p>\n";
    html += "<p>AP IP: " + getAPIP() + "</p>\n";
    html += "</div>\n";
    
    html += "<form method=\"POST\" action=\"/save\">\n";
    
    // WiFi STA 设置（用于 KOReader 模式）
    html += "<div class=\"item\">\n";
    html += "<h3>WiFi STA 设置（用于 KOReader 模式）</h3>\n";
    html += "WiFi SSID:<input name=\"sta_ssid\" value=\"" + String(_staSSID) + "\"><br>\n";
    html += "WiFi 密码:<input name=\"sta_pass\" type=\"password\" value=\"" + String(_staPassword) + "\"><br>\n";
    html += "<p>当前 STA 状态: " + String(isSTAConnected() ? "已连接，IP: " + getSTAIP() : "未连接") + "</p>\n";
    html += "</div>\n";
    
    // KOReader 设置
    html += "<div class=\"item\">\n";
    html += "<h3>KOReader 设置</h3>\n";
    html += "KOReader IP:<input name=\"ko_ip\" value=\"" + String(_config.koreaderIP) + "\"><br>\n";
    html += "KOReader 端口(STA模式):<input name=\"ko_port\" value=\"" + String(_config.koreaderPort) + "\"><br>\n";
    html += "KOReader 端口(AP模式):<input name=\"ko_ap_port\" value=\"" + String(_config.koApPort) + "\"><br>\n";
    html += "<p style=\"color:#666;font-size:12px;\">AP模式下IP固定为192.168.4.2，端口可单独设置</p>\n";
    html += "下一页 HTTP 指令:<input name=\"ko_next\" value=\"" + String(_config.koreaderNextCmd) + "\"><br>\n";
    html += "上一页 HTTP 指令:<input name=\"ko_prev\" value=\"" + String(_config.koreaderPrevCmd) + "\"><br>\n";
    html += "<p>默认: 下一页=GotoViewRel/1, 上一页=GotoViewRel/-1</p>\n";
    html += "<p>其他可用指令: IncreaseFlIntensity/5(加亮度), IncreaseFlIntensity/-5(减亮度), ToggleNightMode(切换夜间)</p>\n";
    html += "<div style=\"background:#f0f7ff;padding:10px;border-radius:4px;margin-top:10px;\">\n";
    html += "<p><b>📋 查看所有 KOReader 指令的方法：</b></p>\n";
    html += "<ol style=\"margin:5px 0;padding-left:20px;\">\n";
    html += "<li><b>STA模式</b>：确保手机和 KOReader 设备连接到同一个 WiFi 网络</li>\n";
    html += "<li><b>AP模式</b>：手机连接翻页器热点\"AlphaPi-Config\"，自动获取IP 192.168.4.2</li>\n";
    html += "<li>在 KOReader 中启动 HTTP Inspector（工具 -> 更多工具 -> KOReader HTTP Inspector）</li>\n";
    html += "<li>在手机浏览器中访问：<code>http://" + String(_config.koreaderIP) + ":" + String(_config.koreaderPort) + "/koreader/event/</code></li>\n";
    html += "</ol>\n";
    html += "</div>\n";
    html += "<p style=\"color:#666;font-size:13px;\">💡 提示：只复制 <code>event/</code> 后面的内容填入上面的输入框</p>\n";
    html += "<p style=\"color:#666;font-size:13px;\">例如：完整地址 <code>http://192.168.2.107:8080/koreader/event/GotoViewRel/1</code>，只填 <code>GotoViewRel/1</code></p>\n";
    html += "</div>\n";
    
    // 摇晃检测设置（按照 Python 版逻辑）
    html += "<div class=\"item\">\n";
    html += "<h3>摇晃检测设置</h3>\n";
    html += "启用摇晃:<select name=\"shake_en\">\n";
    html += "<option value=\"1\"" + String(_config.shakeEnable ? " selected" : "") + ">开启</option>\n";
    html += "<option value=\"0\"" + String(!_config.shakeEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    
    html += "摇晃检测模式:<select name=\"shake_mode\">\n";
    html += "<option value=\"0\"" + String(_config.shakeMode == 0 ? " selected" : "") + ">三轴差值之和（原模式）</option>\n";
    html += "<option value=\"1\"" + String(_config.shakeMode == 1 ? " selected" : "") + ">各轴独立阈值</option>\n";
    html += "</select><br>\n";
    html += "<p style='color:#666;font-size:13px;margin:4px 0;'>模式说明：三轴差值之和=三个轴的变化量加起来超过阈值算摇晃；各轴独立阈值=任一轴变化量超过对应阈值算摇晃，阈值设为0时忽略该轴</p>\n";
    html += "<p style='color:#666;font-size:13px;margin:4px 0;'>轴方向定义（设备水平放置、屏幕朝上）：长边为X轴，短边为Y轴，高度方向为Z轴</p>\n";
    html += "摇晃阈值(500-50000，三轴差值之和超过此值算摇晃中，仅模式0有效):<input name=\"shake_sens\" value=\"" + String(_config.shakeSens) + "\"><br>\n";
    html += "X轴阈值(0=忽略此轴，200-30000，X轴差值超过此值算摇晃中，仅模式1有效):<input name=\"shake_th_x\" value=\"" + String(_config.shakeThresholdX) + "\"><br>\n";
    html += "Y轴阈值(0=忽略此轴，200-30000，Y轴差值超过此值算摇晃中，仅模式1有效):<input name=\"shake_th_y\" value=\"" + String(_config.shakeThresholdY) + "\"><br>\n";
    html += "Z轴阈值(0=忽略此轴，200-30000，Z轴差值超过此值算摇晃中，仅模式1有效):<input name=\"shake_th_z\" value=\"" + String(_config.shakeThresholdZ) + "\"><br>\n";
    html += "最小摇晃时长ms(50-500，摇晃至少持续这么久才算有效摇晃):<input name=\"shake_min_dur\" value=\"" + String(_config.shakeMinDurationMs) + "\"><br>\n";
    html += "静止时长ms(100-1000，摇晃结束后静止这么久才触发翻页):<input name=\"quiet_hold\" value=\"" + String(_config.quietHoldMs) + "\"><br>\n";
    html += "摇晃冷却时间ms(>=500，触发翻页后经过冷却时间才能再次触发):<input name=\"shake_cd\" value=\"" + String(_config.shakeCooldownMs) + "\"><br>\n";
    
    // 加速度实时数值显示（放在摇晃检测设置内）
    html += "实时显示加速度计X/Y/Z轴数值（采样率500ms）:<select name=\"accel_disp\">\n";
    html += "<option value=\"1\"" + String(_config.accelDisplayEnable ? " selected" : "") + ">开启</option>\n";
    html += "<option value=\"0\"" + String(!_config.accelDisplayEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    html += "<div id=\"accel-display\" style=\"margin-top:10px;padding:10px;background:#f5f5f5;border-radius:5px;font-family:monospace;font-size:14px;display:none;\">\n";
    html += "X轴: <span id=\"accel-x\">0</span><br>\n";
    html += "Y轴: <span id=\"accel-y\">0</span><br>\n";
    html += "Z轴: <span id=\"accel-z\">0</span>\n";
    html += "</div>\n";
    html += "</div>\n";
    
    // 休眠设置
    html += "<div class=\"item\">\n";
    html += "<h3>休眠设置</h3>\n";
    html += "休眠超时时间秒(>=30，无操作多久后进入休眠，0=不休眠):<input name=\"sleep_timeout\" value=\"" + String(_config.sleepTimeoutMs / 1000) + "\"><br>\n";
    html += "<p style=\"color:#666;font-size:13px;\">进入休眠前会闪烁2次X图标提示，单击A/B/C任意按键即可唤醒。AP有设备连接时不进入休眠。</p>\n";
    html += "</div>\n";
    
    // 模式启用设置
    html += "<div class=\"item\">\n";
    html += "<h3>模式启用设置（关闭后长按A切换时跳过）</h3>\n";
    html += "Page模式(PageUp/PageDown):<select name=\"mode_page\">\n";
    html += "<option value=\"1\"" + String(_config.modePageEnable ? " selected" : "") + ">启用</option>\n";
    html += "<option value=\"0\"" + String(!_config.modePageEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    html += "Arrow模式(方向键):<select name=\"mode_arrow\">\n";
    html += "<option value=\"1\"" + String(_config.modeArrowEnable ? " selected" : "") + ">启用</option>\n";
    html += "<option value=\"0\"" + String(!_config.modeArrowEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    html += "Media模式(音量加减):<select name=\"mode_media\">\n";
    html += "<option value=\"1\"" + String(_config.modeMediaEnable ? " selected" : "") + ">启用</option>\n";
    html += "<option value=\"0\"" + String(!_config.modeMediaEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    html += "Music模式(上下曲):<select name=\"mode_music\">\n";
    html += "<option value=\"1\"" + String(_config.modeMusicEnable ? " selected" : "") + ">启用</option>\n";
    html += "<option value=\"0\"" + String(!_config.modeMusicEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    html += "Play模式(播放暂停/停止):<select name=\"mode_play\">\n";
    html += "<option value=\"1\"" + String(_config.modePlayEnable ? " selected" : "") + ">启用</option>\n";
    html += "<option value=\"0\"" + String(!_config.modePlayEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    html += "Custom自定义模式(自定义键值):<select name=\"mode_custom\">\n";
    html += "<option value=\"1\"" + String(_config.modeCustomEnable ? " selected" : "") + ">启用</option>\n";
    html += "<option value=\"0\"" + String(!_config.modeCustomEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    html += "</div>\n";
    
    // 自定义模式键值设置
    html += "<div class=\"item\">\n";
    html += "<h3>自定义模式键值设置（HID Usage ID，十进制）</h3>\n";
    html += "<p>常用键值：PageUp=75, PageDown=78, Left=80, Right=79, Up=82, Down=81, Space=44, Enter=40, Esc=41, Tab=43</p>\n";
    html += "<p>修饰键：Ctrl=224, Shift=225, Alt=226, Win=227（组合键示例：Ctrl+C = 键1:224, 键2:6）</p>\n";
    html += "<p>媒体键：播放/暂停=205, 音量+=233, 音量-=234, 下一曲=181, 上一曲=182, 静音=226（类型需选「媒体键」）</p>\n";
    html += "<p><a href='/keys' style='color:#4CAF50;font-weight:bold;'>📖 查看完整HID键值表和常用组合键 →</a></p>\n";
    html += "B键短按组合键（最多3个，0=不用，类型选键盘键或媒体键）:<br>\n";
    html += "  键1:<input type='number' name='custom_key_b_0' value='" + String(_config.customKeyB[0]) + "' min='0' max='255' style='width:60px'>\n";
    html += "  类型:<select name='custom_key_b_type_0' style='width:80px'><option value='0' " + String(_config.customKeyBType[0]==0?"selected":"") + ">键盘键</option><option value='1' " + String(_config.customKeyBType[0]==1?"selected":"") + ">媒体键</option></select>\n";
    html += "  键2:<input type='number' name='custom_key_b_1' value='" + String(_config.customKeyB[1]) + "' min='0' max='255' style='width:60px'>\n";
    html += "  类型:<select name='custom_key_b_type_1' style='width:80px'><option value='0' " + String(_config.customKeyBType[1]==0?"selected":"") + ">键盘键</option><option value='1' " + String(_config.customKeyBType[1]==1?"selected":"") + ">媒体键</option></select>\n";
    html += "  键3:<input type='number' name='custom_key_b_2' value='" + String(_config.customKeyB[2]) + "' min='0' max='255' style='width:60px'>\n";
    html += "  类型:<select name='custom_key_b_type_2' style='width:80px'><option value='0' " + String(_config.customKeyBType[2]==0?"selected":"") + ">键盘键</option><option value='1' " + String(_config.customKeyBType[2]==1?"selected":"") + ">媒体键</option></select><br>\n";
    html += "C键短按组合键（最多3个，0=不用，类型选键盘键或媒体键）:<br>\n";
    html += "  键1:<input type='number' name='custom_key_c_0' value='" + String(_config.customKeyC[0]) + "' min='0' max='255' style='width:60px'>\n";
    html += "  类型:<select name='custom_key_c_type_0' style='width:80px'><option value='0' " + String(_config.customKeyCType[0]==0?"selected":"") + ">键盘键</option><option value='1' " + String(_config.customKeyCType[0]==1?"selected":"") + ">媒体键</option></select>\n";
    html += "  键2:<input type='number' name='custom_key_c_1' value='" + String(_config.customKeyC[1]) + "' min='0' max='255' style='width:60px'>\n";
    html += "  类型:<select name='custom_key_c_type_1' style='width:80px'><option value='0' " + String(_config.customKeyCType[1]==0?"selected":"") + ">键盘键</option><option value='1' " + String(_config.customKeyCType[1]==1?"selected":"") + ">媒体键</option></select>\n";
    html += "  键3:<input type='number' name='custom_key_c_2' value='" + String(_config.customKeyC[2]) + "' min='0' max='255' style='width:60px'>\n";
    html += "  类型:<select name='custom_key_c_type_2' style='width:80px'><option value='0' " + String(_config.customKeyCType[2]==0?"selected":"") + ">键盘键</option><option value='1' " + String(_config.customKeyCType[2]==1?"selected":"") + ">媒体键</option></select><br>\n";
    html += "</div>\n";
    
    // 游戏启用设置
    html += "<div class=\"item\">\n";
    html += "<h3>游戏启用设置（关闭后长按A切换游戏时跳过）</h3>\n";
    html += "贪吃蛇:<select name=\"game_snake\">\n";
    html += "<option value=\"1\"" + String(_config.gameSnakeEnable ? " selected" : "") + ">启用</option>\n";
    html += "<option value=\"0\"" + String(!_config.gameSnakeEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    html += "接球:<select name=\"game_catch\">\n";
    html += "<option value=\"1\"" + String(_config.gameCatchEnable ? " selected" : "") + ">启用</option>\n";
    html += "<option value=\"0\"" + String(!_config.gameCatchEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    html += "摇色子:<select name=\"game_dice\">\n";
    html += "<option value=\"1\"" + String(_config.gameDiceEnable ? " selected" : "") + ">启用</option>\n";
    html += "<option value=\"0\"" + String(!_config.gameDiceEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    html += "秒表:<select name=\"game_stopwatch\">\n";
    html += "<option value=\"1\"" + String(_config.gameStopwatchEnable ? " selected" : "") + ">启用</option>\n";
    html += "<option value=\"0\"" + String(!_config.gameStopwatchEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    html += "井字棋:<select name=\"game_gomoku\">\n";
    html += "<option value=\"1\"" + String(_config.gameGomokuEnable ? " selected" : "") + ">启用</option>\n";
    html += "<option value=\"0\"" + String(!_config.gameGomokuEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    html += "像素鸟:<select name=\"game_flappy\">\n";
    html += "<option value=\"1\"" + String(_config.gameFlappyEnable ? " selected" : "") + ">启用</option>\n";
    html += "<option value=\"0\"" + String(!_config.gameFlappyEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    html += "赛车避障:<select name=\"game_racing\">\n";
    html += "<option value=\"1\"" + String(_config.gameRacingEnable ? " selected" : "") + ">启用</option>\n";
    html += "<option value=\"0\"" + String(!_config.gameRacingEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    html += "俄罗斯方块:<select name=\"game_tetris\">\n";
    html += "<option value=\"1\"" + String(_config.gameTetrisEnable ? " selected" : "") + ">启用</option>\n";
    html += "<option value=\"0\"" + String(!_config.gameTetrisEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    html += "</div>\n";
    
    html += "<br>\n";
    html += "<input type=\"submit\" value=\"保存配置并重启\">\n";
    html += "</form>\n";
    
    // 功能介绍（移到底部）
    html += "<div class=\"item\">\n";
    html += "<h3>功能介绍</h3>\n";
    html += "<p><b>5 种蓝牙键位模式：</b>page（PageUp/Down）、arrow（方向键）、media（音量加减）、music（上下曲）、play（播放暂停/停止）</p>\n";
    html += "<p><b>KOReader 模式：</b>通过 WiFi HTTP 请求控制 KOReader 电子书阅读器，同时长按 A+B 切换。支持STA/AP双模式，长按A一键切换：</p>\n";
    html += "<ul>\n";
    html += "<li><b>STA模式</b>：连接路由器，手机和翻页器在同一局域网</li>\n";
    html += "<li><b>AP模式</b>：手机直连翻页器热点\"AlphaPi-Config\"，IP固定192.168.4.2，不需要路由器</li>\n";
    html += "</ul>\n";
    html += "<p><b>8 种内置小游戏：</b>贪吃蛇、接球、摇色子、秒表、井字棋、像素鸟、赛车避障、俄罗斯方块。同时长按 B+C 进入/退出游戏模式，长按A切换游戏，长按B重新开始。</p>\n";
    html += "<p><b>按键功能：</b></p>\n";
    html += "<table>\n";
    html += "<tr><th>按键</th><th>短按</th><th>长按</th></tr>\n";
    html += "<tr><td>A</td><td>切换翻页箭头显示</td><td>切换模式 / KOReader模式切换STA/AP</td></tr>\n";
    html += "<tr><td>B</td><td>下一页/下一曲/音量+</td><td>开关摇晃翻页</td></tr>\n";
    html += "<tr><td>C</td><td>上一页/上一曲/音量-</td><td>对调翻页方向（play模式不生效）</td></tr>\n";
    html += "<tr><td>A+B</td><td>-</td><td>切换到/离开 KOReader 模式</td></tr>\n";
    html += "<tr><td>B+C</td><td>-</td><td>进入/退出游戏模式</td></tr>\n";
    html += "</table>\n";
    html += "<p><b>休眠功能：</b>可配置无操作超时进入休眠（最低30秒），单击任意按键唤醒，AP有设备连接时不进入休眠，游戏模式也遵守休眠逻辑</p>\n";
    html += "<p><b>屏幕显示：</b>5x5 LED 点阵显示各模式图标和操作状态，翻页箭头显示500ms，其他图标显示2秒后自动熄灭。KOReader模式STA显示大K，AP显示小K</p>\n";
    html += "</div>\n";
    
    // 加速度实时显示JavaScript（500ms采样率）
    html += "<script>\n";
    html += "(function() {\n";
    html += "  var accelDisp = document.querySelector('select[name=\"accel_disp\"]');\n";
    html += "  var accelDisplay = document.getElementById('accel-display');\n";
    html += "  var accelX = document.getElementById('accel-x');\n";
    html += "  var accelY = document.getElementById('accel-y');\n";
    html += "  var accelZ = document.getElementById('accel-z');\n";
    html += "  var accelTimer = null;\n";
    html += "  \n";
    html += "  function updateAccelDisplay() {\n";
    html += "    if (accelDisp.value === '1') {\n";
    html += "      accelDisplay.style.display = 'block';\n";
    html += "      if (accelTimer === null) {\n";
    html += "        accelTimer = setInterval(function() {\n";
    html += "          fetch('/accel', {cache: 'no-store'}).then(function(r) { return r.json(); }).then(function(d) {\n";
    html += "            accelX.textContent = d.x;\n";
    html += "            accelY.textContent = d.y;\n";
    html += "            accelZ.textContent = d.z;\n";
    html += "          }).catch(function() {});\n";
    html += "        }, 500);\n";
    html += "      }\n";
    html += "    } else {\n";
    html += "      accelDisplay.style.display = 'none';\n";
    html += "      if (accelTimer !== null) {\n";
    html += "        clearInterval(accelTimer);\n";
    html += "        accelTimer = null;\n";
    html += "      }\n";
    html += "    }\n";
    html += "  }\n";
    html += "  \n";
    html += "  if (accelDisp) {\n";
    html += "    accelDisp.addEventListener('change', updateAccelDisplay);\n";
    html += "    updateAccelDisplay();\n";
    html += "  }\n";
    html += "})();\n";
    html += "</script>\n";
    
    // 底部项目地址
    html += "<div style='text-align:center;padding:20px 0;color:#999;font-size:13px;'>\n";
    html += "项目地址: <a href='https://github.com/shj4588/AlphaPi_STEM-PageTurner' target='_blank' style='color:#666;'>https://github.com/shj4588/AlphaPi_STEM-PageTurner</a>\n";
    html += "</div>\n";
    
    html += "</body></html>\n";
    return html;
}

void WebConfig::handleRoot() {
    _server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    _server->sendHeader("Pragma", "no-cache");
    _server->sendHeader("Expires", "-1");
    _server->sendHeader("Connection", "close");
    // 使用页面缓存，避免每次请求都重新生成大量字符串导致内存碎片
    if (!_pageCacheValid) {
        _cachedPage = generateConfigPage();
        _pageCacheValid = true;
        Serial.print("Config page generated, size: ");
        Serial.print(_cachedPage.length());
        Serial.print(" bytes, free heap: ");
        Serial.println(ESP.getFreeHeap());
    }
    Serial.print("Serving config page, free heap: ");
    Serial.println(ESP.getFreeHeap());
    _server->send(200, "text/html; charset=utf-8", _cachedPage);
}

void WebConfig::handleKeysPage() {
    String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>HID键值参考表</title>";
    html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f5f5f5;}";
    html += "h1{color:#333;font-size:22px;}h2{color:#555;font-size:18px;margin-top:25px;border-bottom:2px solid #4CAF50;padding-bottom:5px;}";
    html += "table{border-collapse:collapse;width:100%;margin:10px 0;background:#fff;box-shadow:0 1px 3px rgba(0,0,0,0.1);}";
    html += "th,td{border:1px solid #ddd;padding:8px;text-align:left;font-size:14px;}";
    html += "th{background:#4CAF50;color:white;}tr:nth-child(even){background:#f9f9f9;}";
    html += ".key-id{font-weight:bold;color:#d32f2f;font-family:monospace;}";
    html += ".combo{background:#fff3e0;padding:10px;border-radius:5px;margin:5px 0;}";
    html += ".back-link{display:inline-block;margin:15px 0;padding:10px 20px;background:#4CAF50;color:white;text-decoration:none;border-radius:5px;}";
    html += ".back-link:hover{background:#45a049;}";
    html += ".note{background:#e3f2fd;padding:10px;border-radius:5px;margin:10px 0;font-size:14px;}";
    html += "</style></head><body>";
    
    html += "<h1>HID 键盘键值参考表（十进制）</h1>";
    html += "<a href='/' class='back-link'>← 返回配置页面</a>";
    html += "<div class='note'><b>使用说明：</b>在自定义模式中，每个按键最多可设置3个键值（十进制），0表示不使用。修饰键（Ctrl/Shift/Alt/Win）放在前面，普通键放在后面。例如 Ctrl+C = 键1:224, 键2:6, 键3:0。</div>";
    
    // 常用组合键
    html += "<h2>常用组合键速查</h2>";
    html += "<table><tr><th>组合键</th><th>功能</th><th>键1</th><th>键2</th><th>键3</th></tr>";
    html += "<tr><td>Ctrl+C</td><td>复制</td><td class='key-id'>224</td><td class='key-id'>6</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Ctrl+V</td><td>粘贴</td><td class='key-id'>224</td><td class='key-id'>25</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Ctrl+X</td><td>剪切</td><td class='key-id'>224</td><td class='key-id'>27</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Ctrl+Z</td><td>撤销</td><td class='key-id'>224</td><td class='key-id'>29</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Ctrl+Y</td><td>重做</td><td class='key-id'>224</td><td class='key-id'>28</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Ctrl+A</td><td>全选</td><td class='key-id'>224</td><td class='key-id'>4</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Ctrl+S</td><td>保存</td><td class='key-id'>224</td><td class='key-id'>22</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Ctrl+F</td><td>查找</td><td class='key-id'>224</td><td class='key-id'>9</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Ctrl+T</td><td>新建标签页</td><td class='key-id'>224</td><td class='key-id'>23</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Ctrl+W</td><td>关闭标签页</td><td class='key-id'>224</td><td class='key-id'>26</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Ctrl+Shift+T</td><td>恢复关闭的标签页</td><td class='key-id'>224</td><td class='key-id'>225</td><td class='key-id'>23</td></tr>";
    html += "<tr><td>Alt+Tab</td><td>切换窗口</td><td class='key-id'>226</td><td class='key-id'>43</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Alt+F4</td><td>关闭窗口</td><td class='key-id'>226</td><td class='key-id'>61</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Win+D</td><td>显示桌面</td><td class='key-id'>227</td><td class='key-id'>7</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Win+E</td><td>文件管理器</td><td class='key-id'>227</td><td class='key-id'>8</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Win+L</td><td>锁定屏幕</td><td class='key-id'>227</td><td class='key-id'>15</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Win+R</td><td>运行</td><td class='key-id'>227</td><td class='key-id'>21</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Shift+Delete</td><td>永久删除</td><td class='key-id'>225</td><td class='key-id'>76</td><td class='key-id'>0</td></tr>";
    html += "<tr><td>Ctrl+Shift+Esc</td><td>任务管理器</td><td class='key-id'>224</td><td class='key-id'>225</td><td class='key-id'>41</td></tr>";
    html += "</table>";
    
    // 修饰键
    html += "<h2>修饰键（Modifier Keys）</h2>";
    html += "<table><tr><th>键名</th><th>十进制值</th><th>说明</th></tr>";
    html += "<tr><td>Left Ctrl</td><td class='key-id'>224</td><td>左Control</td></tr>";
    html += "<tr><td>Left Shift</td><td class='key-id'>225</td><td>左Shift</td></tr>";
    html += "<tr><td>Left Alt</td><td class='key-id'>226</td><td>左Alt</td></tr>";
    html += "<tr><td>Left Win/Cmd</td><td class='key-id'>227</td><td>左Windows/Command</td></tr>";
    html += "<tr><td>Right Ctrl</td><td class='key-id'>228</td><td>右Control</td></tr>";
    html += "<tr><td>Right Shift</td><td class='key-id'>229</td><td>右Shift</td></tr>";
    html += "<tr><td>Right Alt</td><td class='key-id'>230</td><td>右Alt(AltGr)</td></tr>";
    html += "<tr><td>Right Win/Cmd</td><td class='key-id'>231</td><td>右Windows/Command</td></tr>";
    html += "</table>";
    
    // 字母
    html += "<h2>字母 A-Z</h2>";
    html += "<table><tr><th>键</th><th>值</th><th>键</th><th>值</th><th>键</th><th>值</th></tr>";
    const char* letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (int i = 0; i < 26; i += 3) {
        html += "<tr>";
        for (int j = 0; j < 3 && i + j < 26; j++) {
            html += "<td>" + String(letters[i+j]) + "</td><td class='key-id'>" + String(4 + i + j) + "</td>";
        }
        html += "</tr>";
    }
    html += "</table>";
    
    // 数字
    html += "<h2>数字键（主键盘区）</h2>";
    html += "<table><tr><th>键</th><th>值</th><th>键</th><th>值</th><th>键</th><th>值</th><th>键</th><th>值</th><th>键</th><th>值</th></tr>";
    html += "<tr>";
    for (int i = 0; i < 5; i++) {
        html += "<td>" + String((i + 1) % 10) + "</td><td class='key-id'>" + String(30 + i) + "</td>";
    }
    html += "</tr><tr>";
    for (int i = 5; i < 10; i++) {
        html += "<td>" + String((i + 1) % 10) + "</td><td class='key-id'>" + String(30 + i) + "</td>";
    }
    html += "</tr></table>";
    
    // 控制/编辑键
    html += "<h2>控制与编辑键</h2>";
    html += "<table><tr><th>键名</th><th>值</th><th>说明</th></tr>";
    html += "<tr><td>Enter</td><td class='key-id'>40</td><td>回车键</td></tr>";
    html += "<tr><td>Esc</td><td class='key-id'>41</td><td>退出键</td></tr>";
    html += "<tr><td>Backspace</td><td class='key-id'>42</td><td>退格键</td></tr>";
    html += "<tr><td>Tab</td><td class='key-id'>43</td><td>制表键</td></tr>";
    html += "<tr><td>Space</td><td class='key-id'>44</td><td>空格键</td></tr>";
    html += "<tr><td>Caps Lock</td><td class='key-id'>57</td><td>大写锁定</td></tr>";
    html += "<tr><td>Print Screen</td><td class='key-id'>70</td><td>截屏键</td></tr>";
    html += "<tr><td>Scroll Lock</td><td class='key-id'>71</td><td>滚动锁定</td></tr>";
    html += "<tr><td>Pause</td><td class='key-id'>72</td><td>暂停键</td></tr>";
    html += "<tr><td>Insert</td><td class='key-id'>73</td><td>插入键</td></tr>";
    html += "<tr><td>Home</td><td class='key-id'>74</td><td>行首</td></tr>";
    html += "<tr><td>Page Up</td><td class='key-id'>75</td><td>上一页</td></tr>";
    html += "<tr><td>Delete</td><td class='key-id'>76</td><td>删除键</td></tr>";
    html += "<tr><td>End</td><td class='key-id'>77</td><td>行尾</td></tr>";
    html += "<tr><td>Page Down</td><td class='key-id'>78</td><td>下一页</td></tr>";
    html += "<tr><td>Num Lock</td><td class='key-id'>83</td><td>数字锁定</td></tr>";
    html += "</table>";
    
    // 方向键
    html += "<h2>方向键</h2>";
    html += "<table><tr><th>键名</th><th>值</th></tr>";
    html += "<tr><td>Right →</td><td class='key-id'>79</td></tr>";
    html += "<tr><td>Left ←</td><td class='key-id'>80</td></tr>";
    html += "<tr><td>Down ↓</td><td class='key-id'>81</td></tr>";
    html += "<tr><td>Up ↑</td><td class='key-id'>82</td></tr>";
    html += "</table>";
    
    // 功能键
    html += "<h2>功能键 F1-F24</h2>";
    html += "<table><tr><th>键</th><th>值</th><th>键</th><th>值</th><th>键</th><th>值</th></tr>";
    for (int i = 0; i < 12; i += 3) {
        html += "<tr>";
        for (int j = 0; j < 3; j++) {
            html += "<td>F" + String(i + j + 1) + "</td><td class='key-id'>" + String(58 + i + j) + "</td>";
        }
        html += "</tr>";
    }
    for (int i = 0; i < 12; i += 3) {
        html += "<tr>";
        for (int j = 0; j < 3; j++) {
            html += "<td>F" + String(i + j + 13) + "</td><td class='key-id'>" + String(104 + i + j) + "</td>";
        }
        html += "</tr>";
    }
    html += "</table>";
    
    // 标点符号
    html += "<h2>标点符号</h2>";
    html += "<table><tr><th>键名</th><th>值</th><th>Shift后字符</th></tr>";
    html += "<tr><td>- (减号)</td><td class='key-id'>45</td><td>_ (下划线)</td></tr>";
    html += "<tr><td>= (等号)</td><td class='key-id'>46</td><td>+ (加号)</td></tr>";
    html += "<tr><td>[ (左方括号)</td><td class='key-id'>47</td><td>{ (左大括号)</td></tr>";
    html += "<tr><td>] (右方括号)</td><td class='key-id'>48</td><td>} (右大括号)</td></tr>";
    html += "<tr><td>\\ (反斜杠)</td><td class='key-id'>49</td><td>| (竖线)</td></tr>";
    html += "<tr><td>; (分号)</td><td class='key-id'>51</td><td>: (冒号)</td></tr>";
    html += "<tr><td>' (单引号)</td><td class='key-id'>52</td><td>\" (双引号)</td></tr>";
    html += "<tr><td>` (反引号)</td><td class='key-id'>53</td><td>~ (波浪号)</td></tr>";
    html += "<tr><td>, (逗号)</td><td class='key-id'>54</td><td>< (小于号)</td></tr>";
    html += "<tr><td>. (句号)</td><td class='key-id'>55</td><td>> (大于号)</td></tr>";
    html += "<tr><td>/ (斜杠)</td><td class='key-id'>56</td><td>? (问号)</td></tr>";
    html += "</table>";
    
    // 数字小键盘
    html += "<h2>数字小键盘</h2>";
    html += "<table><tr><th>键名</th><th>值</th><th>键名</th><th>值</th></tr>";
    html += "<tr><td>KP / (除)</td><td class='key-id'>84</td><td>KP * (乘)</td><td class='key-id'>85</td></tr>";
    html += "<tr><td>KP - (减)</td><td class='key-id'>86</td><td>KP + (加)</td><td class='key-id'>87</td></tr>";
    html += "<tr><td>KP Enter</td><td class='key-id'>88</td><td>KP 1 / End</td><td class='key-id'>89</td></tr>";
    html += "<tr><td>KP 2 / Down</td><td class='key-id'>90</td><td>KP 3 / PageDn</td><td class='key-id'>91</td></tr>";
    html += "<tr><td>KP 4 / Left</td><td class='key-id'>92</td><td>KP 5</td><td class='key-id'>93</td></tr>";
    html += "<tr><td>KP 6 / Right</td><td class='key-id'>94</td><td>KP 7 / Home</td><td class='key-id'>95</td></tr>";
    html += "<tr><td>KP 8 / Up</td><td class='key-id'>96</td><td>KP 9 / PageUp</td><td class='key-id'>97</td></tr>";
    html += "<tr><td>KP 0 / Insert</td><td class='key-id'>98</td><td>KP . / Delete</td><td class='key-id'>99</td></tr>";
    html += "</table>";
    
    // 媒体键（Consumer Page）
    html += "<h2>媒体键（Consumer Page）</h2>";
    html += "<div class='note'><b>使用说明：</b>媒体键是 16 位值（常用值小于255），需要使用 Consumer Report 发送。在自定义模式中，将对应键的「类型」下拉框选择为 <b>「媒体键」</b> 即可自动走媒体键通道。媒体键通常单独使用，不建议与键盘键组合。</div>";
    
    html += "<h3>播放控制</h3>";
    html += "<table><tr><th>功能</th><th>十进制</th><th>十六进制</th></tr>";
    html += "<tr><td>播放/暂停（最常用）</td><td class='key-id'>205</td><td>0x00CD</td></tr>";
    html += "<tr><td>播放</td><td class='key-id'>176</td><td>0x00B0</td></tr>";
    html += "<tr><td>暂停</td><td class='key-id'>177</td><td>0x00B1</td></tr>";
    html += "<tr><td>停止</td><td class='key-id'>183</td><td>0x00B7</td></tr>";
    html += "<tr><td>下一曲</td><td class='key-id'>181</td><td>0x00B5</td></tr>";
    html += "<tr><td>上一曲</td><td class='key-id'>182</td><td>0x00B6</td></tr>";
    html += "<tr><td>快进</td><td class='key-id'>179</td><td>0x00B3</td></tr>";
    html += "<tr><td>快退</td><td class='key-id'>180</td><td>0x00B4</td></tr>";
    html += "<tr><td>录制</td><td class='key-id'>178</td><td>0x00B2</td></tr>";
    html += "<tr><td>弹出</td><td class='key-id'>184</td><td>0x00B8</td></tr>";
    html += "<tr><td>随机播放</td><td class='key-id'>185</td><td>0x00B9</td></tr>";
    html += "</table>";
    
    html += "<h3>音量控制</h3>";
    html += "<table><tr><th>功能</th><th>十进制</th><th>十六进制</th></tr>";
    html += "<tr><td>音量+</td><td class='key-id'>233</td><td>0x00E9</td></tr>";
    html += "<tr><td>音量-</td><td class='key-id'>234</td><td>0x00EA</td></tr>";
    html += "<tr><td>静音</td><td class='key-id'>226</td><td>0x00E2</td></tr>";
    html += "<tr><td>低音增强</td><td class='key-id'>229</td><td>0x00E5</td></tr>";
    html += "</table>";
    
    html += "<h3>应用启动</h3>";
    html += "<table><tr><th>功能</th><th>十进制</th><th>十六进制</th></tr>";
    html += "<tr><td>媒体播放器</td><td class='key-id'>387</td><td>0x0183</td></tr>";
    html += "<tr><td>邮件</td><td class='key-id'>394</td><td>0x018A</td></tr>";
    html += "<tr><td>计算器</td><td class='key-id'>402</td><td>0x0192</td></tr>";
    html += "<tr><td>文件管理器</td><td class='key-id'>404</td><td>0x0194</td></tr>";
    html += "<tr><td>任务管理器</td><td class='key-id'>422</td><td>0x01A6</td></tr>";
    html += "</table>";
    
    html += "<h3>浏览器导航</h3>";
    html += "<table><tr><th>功能</th><th>十进制</th><th>十六进制</th></tr>";
    html += "<tr><td>搜索</td><td class='key-id'>545</td><td>0x0221</td></tr>";
    html += "<tr><td>主页</td><td class='key-id'>547</td><td>0x0223</td></tr>";
    html += "<tr><td>后退</td><td class='key-id'>548</td><td>0x0224</td></tr>";
    html += "<tr><td>前进</td><td class='key-id'>549</td><td>0x0225</td></tr>";
    html += "<tr><td>停止</td><td class='key-id'>550</td><td>0x0226</td></tr>";
    html += "<tr><td>刷新</td><td class='key-id'>551</td><td>0x0227</td></tr>";
    html += "<tr><td>书签</td><td class='key-id'>554</td><td>0x022A</td></tr>";
    html += "</table>";
    
    html += "<h3>显示与系统</h3>";
    html += "<table><tr><th>功能</th><th>十进制</th><th>十六进制</th></tr>";
    html += "<tr><td>亮度+</td><td class='key-id'>111</td><td>0x006F</td></tr>";
    html += "<tr><td>亮度-</td><td class='key-id'>112</td><td>0x0070</td></tr>";
    html += "<tr><td>睡眠</td><td class='key-id'>50</td><td>0x0032</td></tr>";
    html += "<tr><td>锁屏/屏保</td><td class='key-id'>414</td><td>0x019E</td></tr>";
    html += "</table>";
    
    html += "<a href='/' class='back-link'>← 返回配置页面</a>";
    html += "</body></html>";
    
    _server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    _server->sendHeader("Pragma", "no-cache");
    _server->sendHeader("Expires", "-1");
    _server->send(200, "text/html; charset=utf-8", html);
}

void WebConfig::setAccelReader(AccelReaderFunc reader) {
    _accelReader = reader;
}

void WebConfig::handleAccel() {
    // 返回加速度数值JSON
    int16_t x = 0, y = 0, z = 0;
    if (_accelReader != nullptr) {
        _accelReader(x, y, z);
    }
    String json = "{\"x\":" + String(x) + ",\"y\":" + String(y) + ",\"z\":" + String(z) + "}";
    _server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    _server->sendHeader("Pragma", "no-cache");
    _server->sendHeader("Expires", "-1");
    _server->send(200, "application/json", json);
}

void WebConfig::handleSave() {
    // 摇晃检测参数（按照 Python 版逻辑）
    _config.shakeEnable = _server->arg("shake_en").toInt() == 1;
    _config.shakeMode = _server->arg("shake_mode").toInt();
    _config.shakeSens = _server->arg("shake_sens").toInt();
    _config.shakeThresholdX = _server->arg("shake_th_x").toInt();
    _config.shakeThresholdY = _server->arg("shake_th_y").toInt();
    _config.shakeThresholdZ = _server->arg("shake_th_z").toInt();
    _config.shakeMinDurationMs = _server->arg("shake_min_dur").toInt();
    _config.quietHoldMs = _server->arg("quiet_hold").toInt();
    _config.shakeCooldownMs = _server->arg("shake_cd").toInt();
    
    // 参数校验
    if (_config.shakeMode > 1) _config.shakeMode = 0;
    if (_config.shakeSens < 500) _config.shakeSens = 500;
    if (_config.shakeSens > 50000) _config.shakeSens = 50000;
    if (_config.shakeThresholdX > 0 && _config.shakeThresholdX < 200) _config.shakeThresholdX = 200;
    if (_config.shakeThresholdX > 30000) _config.shakeThresholdX = 30000;
    if (_config.shakeThresholdY > 0 && _config.shakeThresholdY < 200) _config.shakeThresholdY = 200;
    if (_config.shakeThresholdY > 30000) _config.shakeThresholdY = 30000;
    if (_config.shakeThresholdZ > 0 && _config.shakeThresholdZ < 200) _config.shakeThresholdZ = 200;
    if (_config.shakeThresholdZ > 30000) _config.shakeThresholdZ = 30000;
    if (_config.shakeMinDurationMs < 50) _config.shakeMinDurationMs = 50;
    if (_config.shakeMinDurationMs > 500) _config.shakeMinDurationMs = 500;
    if (_config.quietHoldMs < 100) _config.quietHoldMs = 100;
    if (_config.quietHoldMs > 1000) _config.quietHoldMs = 1000;
    if (_config.shakeCooldownMs < 500) _config.shakeCooldownMs = 500;
    
    // 休眠超时时间（秒转毫秒，最低30秒，0=不休眠）
    uint32_t sleepSec = _server->arg("sleep_timeout").toInt();
    if (sleepSec == 0) {
        _config.sleepTimeoutMs = 0;  // 不休眠
    } else if (sleepSec < 30) {
        _config.sleepTimeoutMs = 30000;  // 最低30秒
    } else {
        _config.sleepTimeoutMs = sleepSec * 1000;
    }
    
    // 加速度实时数值显示开关
    _config.accelDisplayEnable = _server->arg("accel_disp").toInt() == 1;
    
    // 模式启用状态
    _config.modePageEnable = _server->arg("mode_page").toInt() == 1;
    _config.modeArrowEnable = _server->arg("mode_arrow").toInt() == 1;
    _config.modeMediaEnable = _server->arg("mode_media").toInt() == 1;
    _config.modeMusicEnable = _server->arg("mode_music").toInt() == 1;
    _config.modePlayEnable = _server->arg("mode_play").toInt() == 1;
    _config.modeCustomEnable = _server->arg("mode_custom").toInt() == 1;
    
    // 自定义模式组合键（每个按键3个键值+类型）
    for (int i = 0; i < 3; i++) {
        String keyName = "custom_key_b_" + String(i);
        if (_server->hasArg(keyName)) {
            int val = _server->arg(keyName).toInt();
            if (val >= 0 && val <= 255) _config.customKeyB[i] = (uint8_t)val;
        }
        String typeName = "custom_key_b_type_" + String(i);
        if (_server->hasArg(typeName)) {
            int val = _server->arg(typeName).toInt();
            if (val == 0 || val == 1) _config.customKeyBType[i] = (uint8_t)val;
        }
    }
    for (int i = 0; i < 3; i++) {
        String keyName = "custom_key_c_" + String(i);
        if (_server->hasArg(keyName)) {
            int val = _server->arg(keyName).toInt();
            if (val >= 0 && val <= 255) _config.customKeyC[i] = (uint8_t)val;
        }
        String typeName = "custom_key_c_type_" + String(i);
        if (_server->hasArg(typeName)) {
            int val = _server->arg(typeName).toInt();
            if (val == 0 || val == 1) _config.customKeyCType[i] = (uint8_t)val;
        }
    }
    
    // 游戏启用状态
    _config.gameSnakeEnable = _server->arg("game_snake").toInt() == 1;
    _config.gameCatchEnable = _server->arg("game_catch").toInt() == 1;
    _config.gameDiceEnable = _server->arg("game_dice").toInt() == 1;
    _config.gameStopwatchEnable = _server->arg("game_stopwatch").toInt() == 1;
    _config.gameGomokuEnable = _server->arg("game_gomoku").toInt() == 1;
    _config.gameFlappyEnable = _server->arg("game_flappy").toInt() == 1;
    _config.gameRacingEnable = _server->arg("game_racing").toInt() == 1;
    _config.gameTetrisEnable = _server->arg("game_tetris").toInt() == 1;
    
    // WiFi STA 配置
    String newSSID = _server->arg("sta_ssid");
    if (newSSID.length() > 0) {
        strncpy(_staSSID, newSSID.c_str(), sizeof(_staSSID) - 1);
    }
    String newPass = _server->arg("sta_pass");
    if (newPass.length() > 0) {
        strncpy(_staPassword, newPass.c_str(), sizeof(_staPassword) - 1);
    }
    
    // KOReader 配置
    String newKOIP = _server->arg("ko_ip");
    if (newKOIP.length() > 0) {
        strncpy(_config.koreaderIP, newKOIP.c_str(), sizeof(_config.koreaderIP) - 1);
    }
    _config.koreaderPort = _server->arg("ko_port").toInt();
    if (_config.koreaderPort == 0) _config.koreaderPort = 8080;
    _config.koApPort = _server->arg("ko_ap_port").toInt();
    if (_config.koApPort == 0) _config.koApPort = 8080;
    String newKONext = _server->arg("ko_next");
    if (newKONext.length() > 0) {
        strncpy(_config.koreaderNextCmd, newKONext.c_str(), sizeof(_config.koreaderNextCmd) - 1);
    }
    String newKOPrev = _server->arg("ko_prev");
    if (newKOPrev.length() > 0) {
        strncpy(_config.koreaderPrevCmd, newKOPrev.c_str(), sizeof(_config.koreaderPrevCmd) - 1);
    }
    
    // 保存配置
    saveConfig();
    
    _server->send(200, "text/plain; charset=utf-8", "配置已保存，设备即将重启");
    delay(800);
    ESP.restart();
}

void WebConfig::executeAction(uint8_t action) {
    if (!bleHid.isConnected()) return;
    
    switch (action) {
        case ACTION_PAGE_UP:
            bleHid.sendKey(KEY_PAGE_UP);
            break;
        case ACTION_PAGE_DOWN:
            bleHid.sendKey(KEY_PAGE_DOWN);
            break;
        case ACTION_ARROW_UP:
            bleHid.sendKey(KEY_UP);
            break;
        case ACTION_ARROW_DOWN:
            bleHid.sendKey(KEY_DOWN);
            break;
        case ACTION_ARROW_LEFT:
            bleHid.sendKey(KEY_LEFT);
            break;
        case ACTION_ARROW_RIGHT:
            bleHid.sendKey(KEY_RIGHT);
            break;
        case ACTION_VOLUME_UP:
            bleHid.sendMedia(MEDIA_VOLUME_UP);
            break;
        case ACTION_VOLUME_DOWN:
            bleHid.sendMedia(MEDIA_VOLUME_DOWN);
            break;
        case ACTION_MEDIA_PREV:
            bleHid.sendMedia(MEDIA_PREV_TRACK);
            break;
        case ACTION_MEDIA_NEXT:
            bleHid.sendMedia(MEDIA_NEXT_TRACK);
            break;
        case ACTION_MEDIA_PLAYPAUSE:
            bleHid.sendMedia(MEDIA_PLAY_PAUSE);
            break;
        case ACTION_MEDIA_STOP:
            bleHid.sendMedia(MEDIA_STOP);
            break;
        case ACTION_ENTER:
            bleHid.sendKey(KEY_KP_ENTER);
            break;
        case ACTION_SPACE:
            bleHid.sendKey(KEY_SPACE);
            break;
        case ACTION_ESC:
            bleHid.sendKey(KEY_ESCAPE);
            break;
        case ACTION_TAB:
            bleHid.sendKey(KEY_TAB);
            break;
        case ACTION_CUSTOM_B:
            bleHid.sendCombination(_config.customKeyB, _config.customKeyBType);
            break;
        case ACTION_CUSTOM_C:
            bleHid.sendCombination(_config.customKeyC, _config.customKeyCType);
            break;
        default:
            break;
    }
}

bool WebConfig::isSTAConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WebConfig::setKOModeAP(bool apMode) {
    if (_koAPMode == apMode) return;
    _koAPMode = apMode;
    
    // AP模式只是改变HTTP命令的目标IP，不需要重新配置WiFi
    // WiFi一直保持AP+STA模式，热点"AlphaPi-Config"始终存在
    if (apMode) {
        Serial.println("KO mode: AP mode (target IP = 192.168.4.2)");
    } else {
        Serial.println("KO mode: STA mode (target IP = configured IP)");
    }
}

bool WebConfig::isKOModeAP() {
    return _koAPMode;
}

bool WebConfig::sendKoreaderRequest(const char* path) {
    // AP模式下：直接向固定IP发送，不需要检查STA连接
    if (_koAPMode) {
        WiFiClient client;
        String urlPath = "/koreader/event/" + String(path);
        IPAddress targetIP;
        targetIP.fromString(KO_AP_IP);
        
        Serial.print("KO AP mode HTTP GET: ");
        Serial.print(KO_AP_IP);
        Serial.print(":");
        Serial.print(_config.koApPort);
        Serial.println(urlPath);
        
        if (!client.connect(targetIP, _config.koApPort)) {
            Serial.println("KO AP mode: connect FAIL");
            client.stop();
            return false;
        }
        
        String httpReq = "GET " + urlPath + " HTTP/1.1\r\n";
        httpReq += "Host: " + String(KO_AP_IP) + "\r\n";
        httpReq += "Connection: close\r\n";
        httpReq += "\r\n";
        client.print(httpReq);
        
        // 等待响应（最多 400ms）
        unsigned long t = millis();
        while (client.available() == 0 && millis() - t < 400) {
            delay(1);
        }
        
        if (client.available()) {
            String statusLine = client.readStringUntil('\r');
            Serial.print("KO AP response: ");
            Serial.println(statusLine);
        }
        
        client.stop();
        return true;
    }
    
    // STA模式下：需要检查WiFi连接
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("KOReader: WiFi not connected");
        return false;
    }
    
    WiFiClient client;
    String urlPath = "/koreader/event/" + String(path);
    
    Serial.print("KOReader HTTP GET: ");
    Serial.print(_config.koreaderIP);
    Serial.print(":");
    Serial.print(_config.koreaderPort);
    Serial.println(urlPath);
    
    if (!client.connect(_config.koreaderIP, _config.koreaderPort)) {
        Serial.println("KOReader: connect FAIL");
        client.stop();
        return false;
    }
    
    String httpReq = "GET " + urlPath + " HTTP/1.1\r\n";
    httpReq += "Host: " + String(_config.koreaderIP) + "\r\n";
    httpReq += "Connection: close\r\n";
    httpReq += "\r\n";
    client.print(httpReq);
    
    // 等待响应（最多 400ms）
    unsigned long t = millis();
    while (client.available() == 0 && millis() - t < 400) {
        delay(1);
    }
    
    // 读取响应状态行
    if (client.available()) {
        String statusLine = client.readStringUntil('\r');
        Serial.print("KOReader response: ");
        Serial.println(statusLine);
    }
    
    client.stop();
    return true;
}

bool WebConfig::sendKoreaderNext() {
    return sendKoreaderRequest(_config.koreaderNextCmd);
}

bool WebConfig::sendKoreaderPrev() {
    return sendKoreaderRequest(_config.koreaderPrevCmd);
}
