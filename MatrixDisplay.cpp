/*
 * MatrixDisplay.cpp - 5x5 LED 点阵显示驱动实现
 */

#include "MatrixDisplay.h"

// 预定义图标（25 个点，按行优先，1=亮，0=灭）
const uint8_t ICON_PAGE[25] = {
    0,1,1,1,0,
    0,1,0,1,0,
    0,1,1,1,0,
    0,1,0,0,0,
    0,1,0,0,0
};

const uint8_t ICON_ARROW[25] = {
    0,0,0,0,0,
    0,1,0,1,0,
    1,1,1,1,1,
    0,1,0,1,0,
    0,0,0,0,0
};

const uint8_t ICON_MEDIA[25] = {
    0,0,1,0,0,
    0,0,1,0,0,
    1,1,1,1,1,
    0,0,1,0,0,
    0,0,1,0,0
};

const uint8_t ICON_MUSIC[25] = {
    0,0,1,1,0,
    0,0,1,1,0,
    0,0,1,0,0,
    0,1,1,0,0,
    0,1,1,0,0
};

const uint8_t ICON_PLAY[25] = {
    0,1,0,0,0,
    0,1,1,0,0,
    0,1,1,1,0,
    0,1,1,0,0,
    0,1,0,0,0
};

const uint8_t ICON_STOP[25] = {
    0,0,0,0,0,
    0,1,1,1,0,
    0,1,0,1,0,
    0,1,1,1,0,
    0,0,0,0,0
};

const uint8_t ICON_PLAY_PAUSE[25] = {
    0,0,0,0,0,
    0,1,1,1,0,
    0,0,0,0,0,
    0,1,1,1,0,
    0,0,0,0,0
};

const uint8_t ICON_CONNECTED[25] = {
    1,1,1,1,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,1
};

const uint8_t ICON_WAITING[25] = {
    0,0,1,0,0,
    0,1,1,1,0,
    1,1,1,1,1,
    0,1,1,1,0,
    0,0,1,0,0
};

const uint8_t ICON_ARROW_LEFT[25] = {
    0,0,1,0,0,
    0,1,0,0,0,
    1,1,1,1,1,
    0,1,0,0,0,
    0,0,1,0,0
};

const uint8_t ICON_ARROW_RIGHT[25] = {
    0,0,1,0,0,
    0,0,0,1,0,
    1,1,1,1,1,
    0,0,0,1,0,
    0,0,1,0,0
};

const uint8_t ICON_ARROW_ON[25] = {
    0,1,0,1,0,
    1,1,0,1,1,
    1,1,1,1,1,
    1,1,0,1,1,
    0,1,0,1,0
};

const uint8_t ICON_ARROW_OFF[25] = {
    1,0,0,0,1,
    0,1,0,1,0,
    0,0,1,0,0,
    0,1,0,1,0,
    1,0,0,0,1
};

const uint8_t ICON_KOREADER[25] = {
    1,0,0,0,1,
    1,0,0,1,0,
    1,1,1,0,0,
    1,0,0,1,0,
    1,0,0,0,1
};

// KO AP模式图标：小一点的K（横向4像素，纵向5像素）
const uint8_t ICON_KO_AP[25] = {
    0,1,0,0,1,
    0,1,0,1,0,
    0,1,1,0,0,
    0,1,0,1,0,
    0,1,0,0,1
};

// 自定义模式图标：字母U（User用户自定义）
const uint8_t ICON_CUSTOM[25] = {
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0
};

// 字母B（自定义键值模式下B键按下显示）
const uint8_t ICON_LETTER_B[25] = {
    1,1,1,0,0,
    1,0,0,1,0,
    1,1,1,0,0,
    1,0,0,1,0,
    1,1,1,0,0
};

// 字母C（自定义键值模式下C键按下显示）
const uint8_t ICON_LETTER_C[25] = {
    0,1,1,0,0,
    1,0,0,1,0,
    1,0,0,0,0,
    1,0,0,1,0,
    0,1,1,0,0
};

// 自动翻页模式图标：字母A（Auto）
const uint8_t ICON_AUTO[25] = {
    1,1,1,0,0,
    1,0,1,0,0,
    1,1,1,0,0,
    1,0,1,0,0,
    1,0,1,0,0
};

const uint8_t ICON_VOLUME_UP[25] = {
    0,0,0,0,0,
    0,0,1,0,0,
    0,1,1,1,0,
    0,0,1,0,0,
    0,0,0,0,0
};

const uint8_t ICON_VOLUME_DOWN[25] = {
    0,0,0,0,0,
    0,0,0,0,0,
    0,1,1,1,0,
    0,0,0,0,0,
    0,0,0,0,0
};

const uint8_t ICON_SHAKE_ON[25] = {
    0,1,0,1,0,
    1,0,1,0,1,
    0,1,0,1,0,
    1,0,1,0,1,
    0,1,0,1,0
};

const uint8_t ICON_SHAKE_OFF[25] = {
    1,0,0,0,1,
    0,1,0,1,0,
    0,0,1,0,0,
    0,1,0,1,0,
    1,0,0,0,1
};

const uint8_t ICON_DIRECTION_SWAP[25] = {
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0
};

const uint8_t ICON_CLEAR[25] = {
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0
};

MatrixDisplay::MatrixDisplay() {
    _uart = nullptr;
    _failStreak = 0;
    _offline = false;
    _offlineSince = 0;
}

void MatrixDisplay::begin() {
    // 复用已有对象：休眠唤醒会重复调用 begin()，每次 new 会泄漏一个 HardwareSerial
    if (!_uart) {
        _uart = new HardwareSerial(1);
    }
    _uart->begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);
    delay(100);
    
    // 复位熔断状态
    _failStreak = 0;
    _offline = false;
    _offlineSince = 0;
}

// 关闭 UART：TX 保持输出 UART 空闲电平（高），RX 上拉保持确定电平
// 注意：TX 不能设为悬空高阻——协控 RX 浮空会收到噪声，可能被解析成乱码帧
// 导致 LED 处于半亮的杂散状态（休眠时屏幕微微发光）；保持空闲高电平与
// UART 正常工作时的静态一致，唤醒后必须重新调用 begin()
void MatrixDisplay::end() {
    if (!_uart) return;
    _uart->end();
    delay(10);                          // 等最后一帧发完
    pinMode(TX_PIN, OUTPUT);
    digitalWrite(TX_PIN, HIGH);         // UART 空闲电平，电平确定且无噪声
    pinMode(RX_PIN, INPUT_PULLUP);      // 保持确定电平，避免悬空噪声
}

uint8_t MatrixDisplay::calcChecksum(const uint8_t* buf, uint8_t len) {
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < len - 1; i++) {
        checksum += buf[i];
    }
    return checksum & 0xFF;
}

bool MatrixDisplay::uartWrite(uint8_t addr, const uint8_t* data, uint8_t dataLen) {
    if (dataLen == 0 || !_uart) return false;
    
    // 熔断：协控芯片连续无响应时暂停发送
    // 否则每次显示都要走满 10 次重试，单次最坏阻塞约 1.6 秒，会把按键整段吞掉
    if (_offline) {
        if (millis() - _offlineSince < OFFLINE_RETRY_MS) {
            return false;               // 熔断期内零开销直接返回
        }
        _failStreak = 0;                // 到探测时间，放行一次
    }
    
    // 构建帧：[0x90, addr, len(data), data..., checksum]
    uint8_t buf[32];
    uint8_t idx = 0;
    buf[idx++] = FRAME_HEADER;
    buf[idx++] = addr;
    buf[idx++] = dataLen;
    for (uint8_t i = 0; i < dataLen; i++) {
        buf[idx++] = data[i];
    }
    buf[idx++] = 0; // 校验和占位
    
    // 计算校验和
    buf[idx - 1] = calcChecksum(buf, idx);
    
    // 发送帧
    _uart->write(buf, idx);
    
    // 读取响应（3 字节）
    delay(10);
    if (_uart->available() >= 3) {
        uint8_t resp[3];
        _uart->readBytes(resp, 3);
        // 有响应说明协控芯片存活，无论状态码如何都清零失败计数
        _failStreak = 0;
        _offline = false;
        return resp[2] == 0x05; // 第 3 字节 = 0x05 表示成功
    }
    
    // 没有响应，重发一个 0 字节帮助对端重新同步
    _uart->write((uint8_t)0);
    _failStreak++;
    if (_failStreak >= MAX_FAIL_STREAK) {
        _offline = true;
        _offlineSince = millis();
        Serial.println("MatrixDisplay: controller not responding, display suspended");
        return false;   // 熔断后不再阻塞等待
    }
    delay(150);
    return false;
}

void MatrixDisplay::patternToBytes(const uint8_t pattern[25], uint8_t bytes[5]) {
    // 5 列数据，每列 0-31（5 位，对应 5 行）
    uint8_t cols[5] = {0, 0, 0, 0, 0};
    
    // 按列优先编码
    // 对于每一列 x（0-4）：
    //   第 1 行（y=0）-> bit 4 (16)
    //   第 2 行（y=1）-> bit 3 (8)
    //   第 3 行（y=2）-> bit 2 (4)
    //   第 4 行（y=3）-> bit 1 (2)
    //   第 5 行（y=4）-> bit 0 (1)
    for (uint8_t y = 0; y < 5; y++) {
        for (uint8_t x = 0; x < 5; x++) {
            uint8_t idx = y * 5 + x;
            if (pattern[idx] == 1) {
                cols[x] |= (1 << (4 - y));
            }
        }
    }
    
    // 左移 3 位后返回
    for (uint8_t i = 0; i < 5; i++) {
        bytes[i] = cols[i] << 3;
    }
}

void MatrixDisplay::patternToBytes(const char* rows[5], uint8_t bytes[5]) {
    uint8_t pattern[25];
    for (uint8_t y = 0; y < 5; y++) {
        for (uint8_t x = 0; x < 5; x++) {
            char c = rows[y][x];
            pattern[y * 5 + x] = (c == '1' || c == 'X' || c == 'x' || c == '#') ? 1 : 0;
        }
    }
    patternToBytes(pattern, bytes);
}

void MatrixDisplay::showPattern(const uint8_t pattern[25]) {
    uint8_t bytes[5];
    patternToBytes(pattern, bytes);
    
    // 循环发送直到成功
    for (uint8_t retry = 0; retry < 10; retry++) {
        if (uartWrite(DEFAULT_ADDR, bytes, 5)) {
            return;
        }
        if (_offline) {
            return;  // 已熔断，立即放弃重试，避免长时间阻塞
        }
        delay(10);
    }
}

void MatrixDisplay::showPattern(const char* rows[5]) {
    uint8_t bytes[5];
    patternToBytes(rows, bytes);
    
    for (uint8_t retry = 0; retry < 10; retry++) {
        if (uartWrite(DEFAULT_ADDR, bytes, 5)) {
            return;
        }
        if (_offline) {
            return;  // 已熔断，立即放弃重试，避免长时间阻塞
        }
        delay(10);
    }
}

void MatrixDisplay::clear() {
    showPattern(ICON_CLEAR);
}

void MatrixDisplay::showIcon(const char* iconName) {
    if (strcmp(iconName, "page") == 0) showPattern(ICON_PAGE);
    else if (strcmp(iconName, "arrow") == 0) showPattern(ICON_ARROW);
    else if (strcmp(iconName, "media") == 0) showPattern(ICON_MEDIA);
    else if (strcmp(iconName, "music") == 0) showPattern(ICON_MUSIC);
    else if (strcmp(iconName, "play") == 0) showPattern(ICON_PLAY);
    else if (strcmp(iconName, "stop") == 0) showPattern(ICON_STOP);
    else if (strcmp(iconName, "play_pause") == 0) showPattern(ICON_PLAY_PAUSE);
    else if (strcmp(iconName, "connected") == 0) showPattern(ICON_CONNECTED);
    else if (strcmp(iconName, "waiting") == 0) showPattern(ICON_WAITING);
    else if (strcmp(iconName, "arrow_left") == 0) showPattern(ICON_ARROW_LEFT);
    else if (strcmp(iconName, "arrow_right") == 0) showPattern(ICON_ARROW_RIGHT);
    else if (strcmp(iconName, "arrow_on") == 0) showPattern(ICON_ARROW_ON);
    else if (strcmp(iconName, "arrow_off") == 0) showPattern(ICON_ARROW_OFF);
    else if (strcmp(iconName, "koreader") == 0) showPattern(ICON_KOREADER);
    else if (strcmp(iconName, "ko_ap") == 0) showPattern(ICON_KO_AP);
    else if (strcmp(iconName, "custom") == 0) showPattern(ICON_CUSTOM);
    else if (strcmp(iconName, "auto") == 0) showPattern(ICON_AUTO);
    else if (strcmp(iconName, "volume_up") == 0) showPattern(ICON_VOLUME_UP);
    else if (strcmp(iconName, "volume_down") == 0) showPattern(ICON_VOLUME_DOWN);
    else if (strcmp(iconName, "shake_on") == 0) showPattern(ICON_SHAKE_ON);
    else if (strcmp(iconName, "shake_off") == 0) showPattern(ICON_SHAKE_OFF);
    else if (strcmp(iconName, "direction_swap") == 0) showPattern(ICON_DIRECTION_SWAP);
    else if (strcmp(iconName, "clear") == 0) showPattern(ICON_CLEAR);
}
