/*
 * StopwatchGame.cpp - 秒表小游戏实现
 */

#include "StopwatchGame.h"

// 字符图案（0-9, P，行优先，25个点，1=亮）
// 索引：0-9 对应数字 0-9，10 对应 P，11 未使用
const uint8_t StopwatchGame::CHAR_PATTERNS[12][25] = {
    // 0
    {0,1,1,1,0, 1,0,0,0,1, 1,0,0,0,1, 1,0,0,0,1, 0,1,1,1,0},
    // 1
    {0,0,1,0,0, 0,1,1,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,1,1,1,0},
    // 2
    {0,1,1,1,0, 1,0,0,0,1, 0,0,1,0,0, 0,1,0,0,0, 1,1,1,1,1},
    // 3
    {1,1,1,1,1, 0,0,1,0,0, 0,1,1,1,0, 0,0,1,0,0, 1,1,1,1,1},
    // 4
    {1,0,0,0,1, 1,0,0,0,1, 1,1,1,1,1, 0,0,0,0,1, 0,0,0,0,1},
    // 5
    {1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,0, 0,0,0,0,1, 1,1,1,1,0},
    // 6
    {0,1,1,1,0, 1,0,0,0,0, 1,1,1,1,0, 1,0,0,0,1, 0,1,1,1,0},
    // 7
    {1,1,1,1,1, 0,0,0,0,1, 0,0,0,1,0, 0,0,1,0,0, 0,1,0,0,0},
    // 8
    {0,1,1,1,0, 1,0,0,0,1, 0,1,1,1,0, 1,0,0,0,1, 0,1,1,1,0},
    // 9
    {0,1,1,1,0, 1,0,0,0,1, 0,1,1,1,1, 0,0,0,0,1, 0,1,1,1,0},
    // P
    {0,1,1,1,0, 1,0,0,1,0, 1,1,1,1,0, 1,0,0,0,0, 1,0,0,0,0},
    // 11（未使用，全灭）
    {0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0}
};

StopwatchGame::StopwatchGame(MatrixDisplay& display) : _display(display) {
    _state = STATE_READY;
    _startMs = 0;
    _elapsedMs = 0;
    _currentSec = 0;
    _lastDisplayedSec = 0xFFFFFFFF;  // 初始化为一个不可能的值，确保第一次会更新显示
}

void StopwatchGame::begin() {
    _state = STATE_READY;
    _startMs = 0;
    _elapsedMs = 0;
    _currentSec = 0;
    _lastDisplayedSec = 0xFFFFFFFF;
    
    // 显示 0
    showChar('0');
}

void StopwatchGame::update() {
    if (_state == STATE_RUNNING) {
        // 计算当前秒数
        _currentSec = getCurrentSeconds();
        
        // 秒数变化时更新显示
        if (_currentSec != _lastDisplayedSec) {
            _lastDisplayedSec = _currentSec;
            // 显示个位数（0-9 循环）
            uint8_t displayNum = _currentSec % 10;
            showChar('0' + displayNum);
        }
    }
}

void StopwatchGame::onButtonA() {
    if (_state == STATE_READY) {
        // 准备状态：开始计时
        start();
    } else if (_state == STATE_PAUSED) {
        // 暂停状态：继续计时
        resume();
    }
    // 计时中按 A 不做任何事
}

void StopwatchGame::onButtonB() {
    if (_state == STATE_RUNNING) {
        // 计时中：暂停
        pause();
    }
    // 准备状态或暂停状态按 B 不做任何事
}

void StopwatchGame::onButtonC() {
    if (_state == STATE_RUNNING || _state == STATE_PAUSED) {
        // 计时中或暂停：重置
        reset();
    }
    // 准备状态按 C 不做任何事
}

bool StopwatchGame::isRunning() {
    return _state != STATE_READY;
}

void StopwatchGame::exit() {
    _state = STATE_READY;
    _display.clear();
}

void StopwatchGame::start() {
    _state = STATE_RUNNING;
    _startMs = millis();
    _elapsedMs = 0;
    _currentSec = 0;
    _lastDisplayedSec = 0xFFFFFFFF;
    
    // 显示 0
    showChar('0');
}

void StopwatchGame::pause() {
    _state = STATE_PAUSED;
    _elapsedMs = millis() - _startMs;
    
    // 显示 P（暂停）
    showChar('P');
}

void StopwatchGame::resume() {
    _state = STATE_RUNNING;
    _startMs = millis() - _elapsedMs;
    _lastDisplayedSec = 0xFFFFFFFF;  // 强制更新显示
}

void StopwatchGame::reset() {
    _state = STATE_READY;
    _startMs = 0;
    _elapsedMs = 0;
    _currentSec = 0;
    _lastDisplayedSec = 0xFFFFFFFF;
    
    // 显示 0
    showChar('0');
}

void StopwatchGame::showChar(char ch) {
    uint8_t index;
    
    if (ch >= '0' && ch <= '9') {
        index = ch - '0';
    } else if (ch == 'P' || ch == 'p') {
        index = 10;
    } else {
        index = 11;  // 未知字符，全灭
    }
    
    _display.showPattern(CHAR_PATTERNS[index]);
}

uint32_t StopwatchGame::getCurrentSeconds() {
    if (_state == STATE_RUNNING) {
        return (millis() - _startMs) / 1000;
    } else if (_state == STATE_PAUSED) {
        return _elapsedMs / 1000;
    } else {
        return 0;
    }
}
