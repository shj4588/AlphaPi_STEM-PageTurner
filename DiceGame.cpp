/*
 * DiceGame.cpp - 摇色子小游戏实现
 */

#include "DiceGame.h"

// 色子图案（1-6，每个 25 个点，行优先，标准色子布局）
// 索引 0 未使用
const uint8_t DiceGame::DICE_PATTERNS[7][25] = {
    // 0（未使用，全灭）
    {0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0},
    // 1 点：中间
    {0,0,0,0,0, 0,0,0,0,0, 0,0,1,0,0, 0,0,0,0,0, 0,0,0,0,0},
    // 2 点：左上、右下
    {0,0,0,0,0, 0,1,0,0,0, 0,0,0,0,0, 0,0,0,1,0, 0,0,0,0,0},
    // 3 点：左上、中间、右下
    {0,0,0,0,0, 0,1,0,0,0, 0,0,1,0,0, 0,0,0,1,0, 0,0,0,0,0},
    // 4 点：左上、右上、左下、右下
    {0,0,0,0,0, 0,1,0,1,0, 0,0,0,0,0, 0,1,0,1,0, 0,0,0,0,0},
    // 5 点：左上、右上、中间、左下、右下
    {0,0,0,0,0, 0,1,0,1,0, 0,0,1,0,0, 0,1,0,1,0, 0,0,0,0,0},
    // 6 点：左上、右上、左中、右中、左下、右下
    {0,0,0,0,0, 0,1,0,1,0, 0,1,0,1,0, 0,1,0,1,0, 0,0,0,0,0}
};

DiceGame::DiceGame(MatrixDisplay& display, SC7A20& accel) 
    : _display(display), _accel(accel) {
    _state = STATE_READY;
    _currentNumber = 1;
    _lastX = 0;
    _lastY = 0;
    _lastZ = 0;
    _hasLastData = false;
    _lastReadTime = 0;
    _lastShakeTime = 0;
    _lastNumberChangeTime = 0;
}

void DiceGame::begin() {
    _state = STATE_READY;
    _hasLastData = false;
    _lastReadTime = 0;
    _lastShakeTime = 0;
    _lastNumberChangeTime = 0;
    
    // 初始化随机数种子
    randomSeed(analogRead(0));
    
    // 显示一个随机色子
    _currentNumber = random(1, 7);
    showDice(_currentNumber);
}

void DiceGame::update() {
    uint32_t now = millis();
    
    // 每 50ms 读取一次加速度计
    if (now - _lastReadTime >= 50) {
        _lastReadTime = now;
        
        if (detectShake()) {
            _lastShakeTime = now;
            
            if (_state != STATE_ROLLING) {
                _state = STATE_ROLLING;
            }
        }
    }
    
    if (_state == STATE_ROLLING) {
        // 摇动中：每 80ms 切换一次数字
        if (now - _lastNumberChangeTime >= 80) {
            _lastNumberChangeTime = now;
            _currentNumber = random(1, 7);
            showDice(_currentNumber);
        }
        
        // 如果 400ms 没有摇动，停止滚动，显示最终结果
        if (now - _lastShakeTime > 400) {
            _state = STATE_RESULT;
            _currentNumber = random(1, 7);
            showDice(_currentNumber);
        }
    } else if (_state == STATE_RESULT) {
        // 结果状态：等待下一次摇动
        if (now - _lastShakeTime > 500) {
            _state = STATE_READY;
        }
    }
}

void DiceGame::onButtonA() {
    rollDice();
}

void DiceGame::onButtonB() {
    rollDice();
}

void DiceGame::onButtonC() {
    rollDice();
}

bool DiceGame::isRunning() {
    return _state != STATE_READY;
}

void DiceGame::exit() {
    _state = STATE_READY;
    _display.clear();
}

void DiceGame::rollDice() {
    // 手动摇色子：快速切换几次后显示最终结果
    _state = STATE_ROLLING;
    _lastShakeTime = millis();
    _lastNumberChangeTime = 0;
}

void DiceGame::showDice(uint8_t num) {
    if (num < 1 || num > 6) num = 1;
    _display.showPattern(DICE_PATTERNS[num]);
}

bool DiceGame::detectShake() {
    int16_t x, y, z;
    _accel.readRaw(x, y, z);
    
    if (!_hasLastData) {
        _lastX = x;
        _lastY = y;
        _lastZ = z;
        _hasLastData = true;
        return false;
    }
    
    // 计算三轴加速度变化量
    int16_t dx = abs(x - _lastX);
    int16_t dy = abs(y - _lastY);
    int16_t dz = abs(z - _lastZ);
    
    _lastX = x;
    _lastY = y;
    _lastZ = z;
    
    // 总变化量超过阈值认为在摇动
    // 阈值设为 3000（和翻页器灵敏度一致，需要较大幅度摇动才触发）
    int32_t totalDelta = dx + dy + dz;
    return (totalDelta > 3000);
}
