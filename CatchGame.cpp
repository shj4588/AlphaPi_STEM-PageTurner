/*
 * CatchGame.cpp - 接球小游戏实现
 */

#include "CatchGame.h"

// 数字 0-9 的 5x5 点阵图案（行优先，1=亮）
static const uint8_t NUMBER_PATTERNS[10][25] = {
    // 0
    {0,1,1,1,0, 0,1,0,1,0, 0,1,0,1,0, 0,1,0,1,0, 0,1,1,1,0},
    // 1
    {0,0,1,0,0, 0,1,1,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,1,1,1,0},
    // 2
    {0,1,1,1,0, 0,0,0,1,0, 0,1,1,1,0, 0,1,0,0,0, 0,1,1,1,0},
    // 3
    {0,1,1,1,0, 0,0,0,1,0, 0,1,1,1,0, 0,0,0,1,0, 0,1,1,1,0},
    // 4
    {0,1,0,1,0, 0,1,0,1,0, 0,1,1,1,0, 0,0,0,1,0, 0,0,0,1,0},
    // 5
    {0,1,1,1,0, 0,1,0,0,0, 0,1,1,1,0, 0,0,0,1,0, 0,1,1,1,0},
    // 6
    {0,1,1,1,0, 0,1,0,0,0, 0,1,1,1,0, 0,1,0,1,0, 0,1,1,1,0},
    // 7
    {0,1,1,1,0, 0,0,0,1,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0},
    // 8
    {0,1,1,1,0, 0,1,0,1,0, 0,1,1,1,0, 0,1,0,1,0, 0,1,1,1,0},
    // 9
    {0,1,1,1,0, 0,1,0,1,0, 0,1,1,1,0, 0,0,0,1,0, 0,1,1,1,0}
};

CatchGame::CatchGame(MatrixDisplay& display) : _display(display) {
    _state = STATE_READY;
    _paddleX = 2;  // 接球横杆初始在中间（1个像素，列2）
    _ballX = 0;
    _ballY = 0;
    _ballActive = false;
    _score = 0;
    _fallInterval = 600;  // 初始下落速度 600ms
    _lastFallTime = 0;
    _blinkTimer = 0;
    _blinkVisible = true;
    _gameOverTimer = 0;
    _showScore = false;
}

void CatchGame::begin() {
    resetGame();
    _state = STATE_READY;
    showReady();
}

void CatchGame::resetGame() {
    _paddleX = 2;  // 接球横杆初始在中间（1个像素，列2）
    _ballActive = false;
    _score = 0;
    _fallInterval = 600;  // 初始下落速度 600ms
    _lastFallTime = millis();
    _blinkTimer = millis();
    _blinkVisible = true;
    _gameOverTimer = 0;
    _showScore = false;
    
    // 初始化随机数种子
    randomSeed(analogRead(0));
    
    // 生成第一个球
    spawnBall();
}

void CatchGame::spawnBall() {
    _ballX = random(5);  // 随机列位置 0-4
    _ballY = 0;  // 从顶部开始
    _ballActive = true;
}

void CatchGame::update() {
    uint32_t now = millis();
    
    if (_state == STATE_PLAYING) {
        // 球闪烁
        if (now - _blinkTimer > 200) {
            _blinkVisible = !_blinkVisible;
            _blinkTimer = now;
        }
        
        // 球自动下落
        if (_ballActive && now - _lastFallTime >= _fallInterval) {
            _lastFallTime = now;
            fall();
        }
        
        render();
    } else if (_state == STATE_GAME_OVER) {
        // 游戏结束：闪烁显示得分和游戏结束图案
        if (now - _gameOverTimer > 800) {
            _gameOverTimer = now;
            _showScore = !_showScore;
            
            if (_showScore) {
                showNumber(_score % 10);  // 显示个位数
            } else {
                showGameOver();
            }
        }
    } else if (_state == STATE_PAUSED) {
        // 暂停状态：显示暂停图案（中间一个点闪烁）
        if (now - _blinkTimer > 300) {
            _blinkVisible = !_blinkVisible;
            _blinkTimer = now;
            
            uint8_t pattern[25] = {0};
            if (_blinkVisible) {
                pattern[12] = 1;  // 中间点
            }
            _display.showPattern(pattern);
        }
    }
}

void CatchGame::onButtonA() {
    if (_state == STATE_READY) {
        _state = STATE_PLAYING;
        _lastFallTime = millis();
        return;
    }
    
    if (_state == STATE_PLAYING) {
        moveLeft();
    }
}

void CatchGame::onButtonB() {
    if (_state == STATE_READY) {
        _state = STATE_PLAYING;
        _lastFallTime = millis();
        return;
    }
    
    if (_state == STATE_PLAYING) {
        hardDrop();  // 快速下落
    } else if (_state == STATE_PAUSED) {
        _state = STATE_PLAYING;
        _lastFallTime = millis();
    } else if (_state == STATE_GAME_OVER) {
        resetGame();
        _state = STATE_PLAYING;
        _lastFallTime = millis();
    }
}

void CatchGame::onButtonC() {
    if (_state == STATE_READY) {
        _state = STATE_PLAYING;
        _lastFallTime = millis();
        return;
    }
    
    if (_state == STATE_PLAYING) {
        moveRight();
    }
}

bool CatchGame::isRunning() {
    return _state != STATE_READY;
}

void CatchGame::exit() {
    _state = STATE_READY;
    _display.clear();
}

void CatchGame::fall() {
    if (!_ballActive) return;
    
    _ballY++;
    
    // 球落到底部
    if (_ballY >= 4) {
        if (checkCatch()) {
            // 接住了！
            _score++;
            _ballActive = false;
            
            // 加快速度（每接 3 个球加快 50ms，最快 200ms）
            if (_score % 3 == 0 && _fallInterval > 200) {
                _fallInterval -= 50;
            }
            
            // 生成新球
            spawnBall();
        } else {
            // 没接住，游戏结束
            _state = STATE_GAME_OVER;
            _gameOverTimer = millis();
            _showScore = false;
            showGameOver();
        }
    }
}

void CatchGame::moveLeft() {
    // 穿墙模式：左移时如果超出左边界，从右边界出来
    if (_paddleX == 0) {
        _paddleX = 4;  // 接球横杆1个像素，最右位置是4
    } else {
        _paddleX--;
    }
}

void CatchGame::moveRight() {
    // 穿墙模式：右移时如果超出右边界，从左边界出来
    if (_paddleX >= 4) {
        _paddleX = 0;
    } else {
        _paddleX++;
    }
}

void CatchGame::hardDrop() {
    if (!_ballActive) return;
    
    // 直接落到底部
    _ballY = 4;
    
    if (checkCatch()) {
        // 接住了！
        _score++;
        _ballActive = false;
        
        // 加快速度
        if (_score % 3 == 0 && _fallInterval > 200) {
            _fallInterval -= 50;
        }
        
        // 生成新球
        spawnBall();
    } else {
        // 没接住，游戏结束
        _state = STATE_GAME_OVER;
        _gameOverTimer = millis();
        _showScore = false;
        showGameOver();
    }
    
    _lastFallTime = millis();
}

bool CatchGame::checkCatch() {
    // 接球横杆占 1 列：_paddleX
    // 球在列 _ballX
    // 接住条件：球的列位置等于接球横杆的列位置
    return (_ballX == _paddleX);
}

void CatchGame::render() {
    uint8_t pattern[25] = {0};
    
    // 绘制接球横杆（底部一行，1个像素）
    pattern[4 * 5 + _paddleX] = 1;
    
    // 绘制球（闪烁）
    if (_ballActive && _blinkVisible) {
        if (_ballY >= 0 && _ballY < 5 && _ballX >= 0 && _ballX < 5) {
            pattern[_ballY * 5 + _ballX] = 1;
        }
    }
    
    _display.showPattern(pattern);
}

void CatchGame::showNumber(uint8_t num) {
    if (num > 9) num = 9;
    _display.showPattern(NUMBER_PATTERNS[num]);
}

void CatchGame::showGameOver() {
    // X 图案（游戏结束）
    uint8_t pattern[25] = {
        1,0,0,0,1,
        0,1,0,1,0,
        0,0,1,0,0,
        0,1,0,1,0,
        1,0,0,0,1
    };
    _display.showPattern(pattern);
}

void CatchGame::showReady() {
    // 准备图案：底部横杆（1个像素）+ 顶部一个点
    uint8_t pattern[25] = {0};
    pattern[2 * 5 + 2] = 1;  // 中间点（球）
    pattern[4 * 5 + 2] = 1;  // 接球横杆（1个像素）
    _display.showPattern(pattern);
}
