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
}

void MatrixDisplay::begin() {
    _uart = new HardwareSerial(1);
    _uart->begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);
    delay(100);
}

uint8_t MatrixDisplay::calcChecksum(const uint8_t* buf, uint8_t len) {
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < len - 1; i++) {
        checksum += buf[i];
    }
    return checksum & 0xFF;
}

bool MatrixDisplay::uartWrite(uint8_t addr, const uint8_t* data, uint8_t dataLen) {
    if (dataLen == 0) return false;
    
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
        return resp[2] == 0x05; // 第 3 字节 = 0x05 表示成功
    }
    
    // 没有响应，重发一个 0 字节
    _uart->write((uint8_t)0);
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
    else if (strcmp(iconName, "volume_up") == 0) showPattern(ICON_VOLUME_UP);
    else if (strcmp(iconName, "volume_down") == 0) showPattern(ICON_VOLUME_DOWN);
    else if (strcmp(iconName, "shake_on") == 0) showPattern(ICON_SHAKE_ON);
    else if (strcmp(iconName, "shake_off") == 0) showPattern(ICON_SHAKE_OFF);
    else if (strcmp(iconName, "direction_swap") == 0) showPattern(ICON_DIRECTION_SWAP);
    else if (strcmp(iconName, "clear") == 0) showPattern(ICON_CLEAR);
}
