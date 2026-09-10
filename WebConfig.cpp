/*
 * WebConfig.cpp - Web 配置功能实现
 * 
 * 参考 test.cpp 的实现
 */

#include "WebConfig.h"
#include "BleHid.h"

extern BleHid bleHid;

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
    
    // 摇晃检测参数
    _config.shakeEnable = true;
    _config.shakeAction = ACTION_PAGE_DOWN;
    _config.shakeSens = 2500;
    _config.shakeNeedCnt = 4;
    _config.shakeWinMs = 2000;
    _config.quietSens = 600;
    _config.quietHoldMs = 150;
    _config.shakeCooldownMs = 1000;
    
    // 其他设置
    _config.longPressMs = 800;
    _config.directionSwap = false;
    _config.pageDisplayEnable = true;  // 默认开启翻页屏幕显示
    _config.sleepTimeoutMs = 120000;  // 默认 2 分钟休眠
    
    // 模式启用状态（默认全部启用）
    _config.modePageEnable = true;
    _config.modeArrowEnable = true;
    _config.modeMediaEnable = true;
    _config.modeMusicEnable = true;
    _config.modePlayEnable = true;
    
    // KOReader 默认配置
    strcpy(_config.koreaderIP, "192.168.2.107");
    _config.koreaderPort = 8080;
    strcpy(_config.koreaderNextCmd, "GotoViewRel/1");
    strcpy(_config.koreaderPrevCmd, "GotoViewRel/-1");
    
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
    
    // 键位配置
    for (int m = 0; m < 5; m++) {
        for (int k = 0; k < 6; k++) {
            String key = "key_" + String(m) + "_" + String(k);
            _config.keyActions[m][k] = _prefs.getUChar(key.c_str(), _config.keyActions[m][k]);
        }
    }
    
    // 摇晃检测参数（按照 Python 版逻辑）
    _config.shakeEnable = _prefs.getBool("shake_en", true);
    _config.shakeAction = _prefs.getUChar("shake_act", ACTION_PAGE_DOWN);
    _config.shakeSens = _prefs.getInt("shake_sens", 3000);              // 摇晃阈值
    _config.shakeMinDurationMs = _prefs.getUInt("shake_min_dur", 100);   // 最小摇晃时长
    _config.quietHoldMs = _prefs.getUInt("quiet_hold", 300);             // 静止时长
    _config.shakeCooldownMs = _prefs.getUInt("shake_cd", 1000);          // 冷却时间
    // 以下参数已不再使用，保留兼容
    _config.shakeNeedCnt = _prefs.getUChar("shake_cnt", 4);
    _config.shakeWinMs = _prefs.getUInt("shake_win", 2000);
    _config.quietSens = _prefs.getInt("quiet_sens", 600);
    
    // 其他设置
    _config.longPressMs = _prefs.getUInt("long_press", 800);
    _config.directionSwap = _prefs.getBool("dir_swap", false);
    _config.pageDisplayEnable = _prefs.getBool("page_disp", true);
    _config.sleepTimeoutMs = _prefs.getUInt("sleep_timeout", 120000);
    
    // 模式启用状态
    _config.modePageEnable = _prefs.getBool("mode_page", true);
    _config.modeArrowEnable = _prefs.getBool("mode_arrow", true);
    _config.modeMediaEnable = _prefs.getBool("mode_media", true);
    _config.modeMusicEnable = _prefs.getBool("mode_music", true);
    _config.modePlayEnable = _prefs.getBool("mode_play", true);
    
    // KOReader 配置
    String koIP = _prefs.getString("ko_ip", "");
    if (koIP.length() > 0) {
        strncpy(_config.koreaderIP, koIP.c_str(), sizeof(_config.koreaderIP) - 1);
    }
    _config.koreaderPort = _prefs.getUShort("ko_port", 8080);
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
    
    _prefs.end();
}

void WebConfig::saveConfig() {
    _prefs.begin("pageturner", false);
    
    // 设备设置
    _prefs.putString("dev_name", _config.deviceName);
    _prefs.putUChar("mode", _config.currentMode);
    
    // 键位配置
    for (int m = 0; m < 5; m++) {
        for (int k = 0; k < 6; k++) {
            String key = "key_" + String(m) + "_" + String(k);
            _prefs.putUChar(key.c_str(), _config.keyActions[m][k]);
        }
    }
    
    // 摇晃检测参数（按照 Python 版逻辑）
    _prefs.putBool("shake_en", _config.shakeEnable);
    _prefs.putUChar("shake_act", _config.shakeAction);
    _prefs.putInt("shake_sens", _config.shakeSens);
    _prefs.putUInt("shake_min_dur", _config.shakeMinDurationMs);
    _prefs.putUInt("quiet_hold", _config.quietHoldMs);
    _prefs.putUInt("shake_cd", _config.shakeCooldownMs);
    // 以下参数已不再使用，保留兼容
    _prefs.putUChar("shake_cnt", _config.shakeNeedCnt);
    _prefs.putUInt("shake_win", _config.shakeWinMs);
    _prefs.putInt("quiet_sens", _config.quietSens);
    
    // 其他设置
    _prefs.putUInt("long_press", _config.longPressMs);
    _prefs.putBool("dir_swap", _config.directionSwap);
    _prefs.putBool("page_disp", _config.pageDisplayEnable);
    _prefs.putUInt("sleep_timeout", _config.sleepTimeoutMs);
    
    // 模式启用状态
    _prefs.putBool("mode_page", _config.modePageEnable);
    _prefs.putBool("mode_arrow", _config.modeArrowEnable);
    _prefs.putBool("mode_media", _config.modeMediaEnable);
    _prefs.putBool("mode_music", _config.modeMusicEnable);
    _prefs.putBool("mode_play", _config.modePlayEnable);
    
    // KOReader 配置
    _prefs.putString("ko_ip", _config.koreaderIP);
    _prefs.putUShort("ko_port", _config.koreaderPort);
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
    WiFi.mode(WIFI_AP_STA);
    IPAddress apIP(192, 168, 4, 1);
    IPAddress netMsk(255, 255, 255, 0);
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP("AlphaPi-Config");
    
    // 尝试连接 STA WiFi（用于 KOReader 模式）
    if (strlen(_staSSID) > 0) {
        Serial.print("尝试连接 STA WiFi: ");
        Serial.println(_staSSID);
        WiFi.begin(_staSSID, _staPassword);
    }
    
    // 启动 Web 服务器
    if (_server == nullptr) {
        _server = new WebServer(80);
        _server->on("/", std::bind(&WebConfig::handleRoot, this));
        _server->on("/save", std::bind(&WebConfig::handleSave, this));
    }
    _server->begin();
    
    Serial.println("Web server started");
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
    html.reserve(6000);
    
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
    html += "<h2>AlphaPi 翻页器配置</h2>\n";
    
    // WiFi 信息
    html += "<div class=\"item\">\n";
    html += "<h3>WiFi 信息</h3>\n";
    html += "<p>热点AP: AlphaPi-Config</p>\n";
    html += "<p>AP IP: " + getAPIP() + "</p>\n";
    html += "</div>\n";
    
    html += "<form method=\"POST\" action=\"/save\">\n";
    
    // 摇晃检测设置（按照 Python 版逻辑）
    html += "<div class=\"item\">\n";
    html += "<h3>摇晃检测设置</h3>\n";
    html += "启用摇晃:<select name=\"shake_en\">\n";
    html += "<option value=\"1\"" + String(_config.shakeEnable ? " selected" : "") + ">开启</option>\n";
    html += "<option value=\"0\"" + String(!_config.shakeEnable ? " selected" : "") + ">关闭</option>\n";
    html += "</select><br>\n";
    
    html += "摇晃阈值(1000-10000，三轴差值之和超过此值算摇晃中):<input name=\"shake_sens\" value=\"" + String(_config.shakeSens) + "\"><br>\n";
    html += "最小摇晃时长ms(50-500，摇晃至少持续这么久才算有效摇晃):<input name=\"shake_min_dur\" value=\"" + String(_config.shakeMinDurationMs) + "\"><br>\n";
    html += "静止时长ms(100-1000，摇晃结束后静止这么久才触发翻页):<input name=\"quiet_hold\" value=\"" + String(_config.quietHoldMs) + "\"><br>\n";
    html += "摇晃冷却时间ms(>=500，触发翻页后经过冷却时间才能再次触发):<input name=\"shake_cd\" value=\"" + String(_config.shakeCooldownMs) + "\"><br>\n";
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
    html += "</div>\n";
    
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
    html += "KOReader 端口:<input name=\"ko_port\" value=\"" + String(_config.koreaderPort) + "\"><br>\n";
    html += "下一页 HTTP 指令:<input name=\"ko_next\" value=\"" + String(_config.koreaderNextCmd) + "\"><br>\n";
    html += "上一页 HTTP 指令:<input name=\"ko_prev\" value=\"" + String(_config.koreaderPrevCmd) + "\"><br>\n";
    html += "<p>默认: 下一页=GotoViewRel/1, 上一页=GotoViewRel/-1</p>\n";
    html += "<p>其他可用指令: IncreaseFlIntensity/5(加亮度), IncreaseFlIntensity/-5(减亮度), ToggleNightMode(切换夜间)</p>\n";
    html += "<div style=\"background:#f0f7ff;padding:10px;border-radius:4px;margin-top:10px;\">\n";
    html += "<p><b>📋 查看所有 KOReader 指令的方法：</b></p>\n";
    html += "<ol style=\"margin:5px 0;padding-left:20px;\">\n";
    html += "<li>确保手机和 KOReader 设备连接到<b>同一个 WiFi 网络</b></li>\n";
    html += "<li>在 KOReader 中启动 HTTP Inspector（工具 -> 更多工具 -> KOReader HTTP Inspector）</li>\n";
    html += "<li>在手机浏览器中访问：<code>http://" + String(_config.koreaderIP) + ":" + String(_config.koreaderPort) + "/koreader/event/</code></li>\n";
    html += "</ol>\n";
    html += "</div>\n";
    html += "<p style=\"color:#666;font-size:13px;\">💡 提示：只复制 <code>event/</code> 后面的内容填入上面的输入框</p>\n";
    html += "<p style=\"color:#666;font-size:13px;\">例如：完整地址 <code>http://192.168.2.107:8080/koreader/event/GotoViewRel/1</code>，只填 <code>GotoViewRel/1</code></p>\n";
    html += "</div>\n";
    
    html += "<br>\n";
    html += "<input type=\"submit\" value=\"保存配置并重启\">\n";
    html += "</form>\n";
    
    // 操作说明
    html += "<div class=\"note\">\n";
    html += "<h3>操作说明</h3>\n";
    html += "<table>\n";
    html += "<tr><th>按键</th><th>短按</th><th>长按</th></tr>\n";
    html += "<tr><td>A 键</td><td>切换翻页箭头显示开关</td><td>切换键位模式</td></tr>\n";
    html += "<tr><td>B 键</td><td>下一页/下一曲/音量+</td><td>开关摇晃翻页功能</td></tr>\n";
    html += "<tr><td>C 键</td><td>上一页/上一曲/音量-</td><td>对调翻页方向（play模式下不生效）</td></tr>\n";
    html += "</table>\n";
    html += "<br>\n";
    html += "<h4>6 种键位模式</h4>\n";
    html += "<ol>\n";
    html += "<li><b>page</b> - PageUp / PageDown（PowerPoint、乐谱 App 通用）</li>\n";
    html += "<li><b>arrow</b> - Left / Right（ForScore 等方向键翻页 App）</li>\n";
    html += "<li><b>media</b> - 音量+ / 音量-（阅读类 App 无翻页键时用音量键触发）</li>\n";
    html += "<li><b>music</b> - 上一曲 / 下一曲（音乐播放器切歌）</li>\n";
    html += "<li><b>play</b> - 播放暂停 / 停止（音乐播放器播放控制）</li>\n";
    html += "<li><b>koreader</b> - 通过 HTTP 请求控制 KOReader（需要连接 WiFi，可自定义指令）</li>\n";
    html += "</ol>\n";
    html += "<br>\n";
    html += "<h4>摇晃翻页功能</h4>\n";
    html += "<ul>\n";
    html += "<li>默认开启，长按 B 键可开关摇晃翻页功能</li>\n";
    html += "<li>摇晃设备即可翻页，和当前按键模式同步切换</li>\n";
    html += "<li><b>play 模式下摇晃只触发播放暂停</b>，不触发停止</li>\n";
    html += "<li>摇晃检测：检测到晃动幅度连续多帧超限 → 等待静止，静止达标才触发</li>\n";
    html += "</ul>\n";
    html += "<br>\n";
    html += "<p>连接热点 AlphaPi-Config，访问 192.168.4.1 配置</p>\n";
    html += "<p>保存配置后设备会自动重启</p>\n";
    html += "</div>\n";
    
    html += "</body></html>\n";
    return html;
}

void WebConfig::handleRoot() {
    _server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    _server->sendHeader("Pragma", "no-cache");
    _server->sendHeader("Expires", "-1");
    _server->send(200, "text/html; charset=utf-8", generateConfigPage());
}

void WebConfig::handleSave() {
    // 摇晃检测参数（按照 Python 版逻辑）
    _config.shakeEnable = _server->arg("shake_en").toInt() == 1;
    _config.shakeSens = _server->arg("shake_sens").toInt();
    _config.shakeMinDurationMs = _server->arg("shake_min_dur").toInt();
    _config.quietHoldMs = _server->arg("quiet_hold").toInt();
    _config.shakeCooldownMs = _server->arg("shake_cd").toInt();
    
    // 参数校验
    if (_config.shakeSens < 1000) _config.shakeSens = 1000;
    if (_config.shakeSens > 10000) _config.shakeSens = 10000;
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
    
    // 模式启用状态
    _config.modePageEnable = _server->arg("mode_page").toInt() == 1;
    _config.modeArrowEnable = _server->arg("mode_arrow").toInt() == 1;
    _config.modeMediaEnable = _server->arg("mode_media").toInt() == 1;
    _config.modeMusicEnable = _server->arg("mode_music").toInt() == 1;
    _config.modePlayEnable = _server->arg("mode_play").toInt() == 1;
    
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
        default:
            break;
    }
}

bool WebConfig::isSTAConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool WebConfig::sendKoreaderRequest(const char* path) {
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
