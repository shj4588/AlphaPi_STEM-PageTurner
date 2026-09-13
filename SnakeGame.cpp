/*
 * SnakeGame.cpp - 贪吃蛇小游戏实现
 */

#include "SnakeGame.h"

// 数字 0-9 的 5x5 点阵图案（行优先，1=亮）
static const uint8_t NUMBER_PATTERNS[10][25] = {
    // 0
    {0,1,1,1,0,
     0,1,0,1,0,
     0,1,0,1,0,
     0,1,0,1,0,
     0,1,1,1,0},
    // 1
    {0,0,1,0,0,
     0,1,1,0,0,
     0,0,1,0,0,
     0,0,1,0,0,
     0,1,1,1,0},
    // 2
    {0,1,1,1,0,
     0,0,0,1,0,
     0,1,1,1,0,
     0,1,0,0,0,
     0,1,1,1,0},
    // 3
    {0,1,1,1,0,
     0,0,0,1,0,
     0,1,1,1,0,
     0,0,0,1,0,
     0,1,1,1,0},
    // 4
    {0,1,0,1,0,
     0,1,0,1,0,
     0,1,1,1,0,
     0,0,0,1,0,
     0,0,0,1,0},
    // 5
    {0,1,1,1,0,
     0,1,0,0,0,
     0,1,1,1,0,
     0,0,0,1,0,
     0,1,1,1,0},
    // 6
    {0,1,1,1,0,
     0,1,0,0,0,
     0,1,1,1,0,
     0,1,0,1,0,
     0,1,1,1,0},
    // 7
    {0,1,1,1,0,
     0,0,0,1,0,
     0,0,1,0,0,
     0,0,1,0,0,
     0,0,1,0,0},
    // 8
    {0,1,1,1,0,
     0,1,0,1,0,
     0,1,1,1,0,
     0,1,0,1,0,
     0,1,1,1,0},
    // 9
    {0,1,1,1,0,
     0,1,0,1,0,
     0,1,1,1,0,
     0,0,0,1,0,
     0,1,1,1,0}
};

SnakeGame::SnakeGame(MatrixDisplay& display) : _display(display) {
    _state = STATE_READY;
    _snakeLength = 0;
    _score = 0;
    _moveInterval = 500;
    _lastMoveTime = 0;
    _foodVisible = true;
    _blinkTimer = 0;
    _gameOverTimer = 0;
    _showScore = false;
}

void SnakeGame::begin() {
    resetGame();
    _state = STATE_READY;
    showReady();
}

void SnakeGame::resetGame() {
    // 初始化蛇：在中间位置，长度为 3，向右移动
    _snakeX[0] = 2;  // 头
    _snakeY[0] = 2;
    _snakeX[1] = 1;
    _snakeY[1] = 2;
    _snakeX[2] = 0;
    _snakeY[2] = 2;
    _snakeLength = 3;
    
    _direction = DIR_RIGHT;
    _nextDirection = DIR_RIGHT;
    
    _score = 0;
    _moveInterval = 1000;  // 初始速度 1000ms（减半）
    _lastMoveTime = millis();
    _foodVisible = true;
    _blinkTimer = millis();
    _gameOverTimer = 0;
    _showScore = false;
    
    spawnFood();
}

void SnakeGame::update() {
    uint32_t now = millis();
    
    if (_state == STATE_PLAYING) {
        // 食物闪烁
        if (now - _blinkTimer > 200) {
            _foodVisible = !_foodVisible;
            _blinkTimer = now;
        }
        
        // 移动蛇
        if (now - _lastMoveTime >= _moveInterval) {
            _lastMoveTime = now;
            moveSnake();
        }
        
        render();
    } else if (_state == STATE_GAME_OVER) {
        // 游戏结束：闪烁显示得分和游戏结束图案
        if (now - _gameOverTimer > 800) {
            _gameOverTimer = now;
            _showScore = !_showScore;
            
            if (_showScore) {
                showNumber(_score % 10);  // 显示个位数（5x5 只能显示一个数字）
            } else {
                showGameOver();
            }
        }
    } else if (_state == STATE_PAUSED) {
        // 暂停状态：显示暂停图案（中间一个点闪烁）
        if (now - _blinkTimer > 300) {
            _foodVisible = !_foodVisible;
            _blinkTimer = now;
            
            uint8_t pattern[25] = {0};
            if (_foodVisible) {
                pattern[12] = 1;  // 中间点
            }
            _display.showPattern(pattern);
        }
    }
}

void SnakeGame::onButtonA() {
    if (_state == STATE_READY) {
        // 准备状态：按 A 开始游戏
        _state = STATE_PLAYING;
        _lastMoveTime = millis();
        return;
    }
    
    if (_state == STATE_PLAYING) {
        // 顺时针旋转方向
        _nextDirection = (Direction)((_nextDirection + 1) % 4);
    }
}

void SnakeGame::onButtonB() {
    if (_state == STATE_READY) {
        // 准备状态：按 B 开始游戏
        _state = STATE_PLAYING;
        _lastMoveTime = millis();
        return;
    }
    
    if (_state == STATE_PLAYING) {
        // 暂停
        _state = STATE_PAUSED;
        _blinkTimer = millis();
        _foodVisible = true;
    } else if (_state == STATE_PAUSED) {
        // 继续
        _state = STATE_PLAYING;
        _lastMoveTime = millis();
    } else if (_state == STATE_GAME_OVER) {
        // 重新开始
        resetGame();
        _state = STATE_PLAYING;
        _lastMoveTime = millis();
    }
}

void SnakeGame::onButtonC() {
    if (_state == STATE_READY) {
        // 准备状态：按 C 开始游戏
        _state = STATE_PLAYING;
        _lastMoveTime = millis();
        return;
    }
    
    if (_state == STATE_PLAYING) {
        // 逆时针旋转方向
        _nextDirection = (Direction)((_nextDirection + 3) % 4);
    }
}

bool SnakeGame::isRunning() {
    return _state != STATE_READY;
}

void SnakeGame::exit() {
    _state = STATE_READY;
    _display.clear();
}

void SnakeGame::moveSnake() {
    // 应用方向变化（防止 180 度掉头）
    if ((_direction == DIR_UP && _nextDirection != DIR_DOWN) ||
        (_direction == DIR_DOWN && _nextDirection != DIR_UP) ||
        (_direction == DIR_LEFT && _nextDirection != DIR_RIGHT) ||
        (_direction == DIR_RIGHT && _nextDirection != DIR_LEFT)) {
        _direction = _nextDirection;
    }
    
    // 计算新的头部位置（穿墙模式：碰到边界从另一边出来）
    uint8_t newX = _snakeX[0];
    uint8_t newY = _snakeY[0];
    
    switch (_direction) {
        case DIR_UP:
            newY = (newY == 0) ? 4 : newY - 1;  // 穿墙：从上边出去从下边出来
            break;
        case DIR_DOWN:
            newY = (newY == 4) ? 0 : newY + 1;  // 穿墙：从下边出去从上边出来
            break;
        case DIR_LEFT:
            newX = (newX == 0) ? 4 : newX - 1;  // 穿墙：从左边出去从右边出来
            break;
        case DIR_RIGHT:
            newX = (newX == 4) ? 0 : newX + 1;  // 穿墙：从右边出去从左边出来
            break;
    }
    
    // 检查是否撞到自己
    if (checkSelfCollision(newX, newY)) {
        _state = STATE_GAME_OVER;
        _gameOverTimer = millis();
        _showScore = false;
        showGameOver();
        return;
    }
    
    // 检查是否吃到食物
    bool ateFood = (newX == _foodX && newY == _foodY);
    
    // 移动蛇身（从尾到头）
    if (ateFood) {
        // 吃到食物：蛇身变长，不移动尾巴
        if (_snakeLength < MAX_SNAKE_LENGTH) {
            _snakeLength++;
        }
        _score++;
        
        // 加快速度（每 5 分加快 50ms，最快 400ms，减半）
        if (_score % 5 == 0 && _moveInterval > 400) {
            _moveInterval -= 50;
        }
        
        // 生成新食物
        spawnFood();
    }
    
    // 移动蛇身
    for (int i = _snakeLength - 1; i > 0; i--) {
        _snakeX[i] = _snakeX[i - 1];
        _snakeY[i] = _snakeY[i - 1];
    }
    _snakeX[0] = newX;
    _snakeY[0] = newY;
}

void SnakeGame::spawnFood() {
    // 随机生成食物位置，不能在蛇身上
    uint8_t attempts = 0;
    do {
        _foodX = random(5);
        _foodY = random(5);
        attempts++;
    } while (checkSelfCollision(_foodX, _foodY) && attempts < 100);
    
    _foodVisible = true;
}

bool SnakeGame::checkSelfCollision(uint8_t x, uint8_t y) {
    for (uint8_t i = 0; i < _snakeLength; i++) {
        if (_snakeX[i] == x && _snakeY[i] == y) {
            return true;
        }
    }
    return false;
}

void SnakeGame::render() {
    uint8_t pattern[25] = {0};
    
    // 绘制蛇身
    for (uint8_t i = 0; i < _snakeLength; i++) {
        uint8_t idx = _snakeY[i] * 5 + _snakeX[i];
        if (idx < 25) {
            pattern[idx] = 1;
        }
    }
    
    // 绘制食物（闪烁）
    if (_foodVisible) {
        uint8_t idx = _foodY * 5 + _foodX;
        if (idx < 25) {
            pattern[idx] = 1;
        }
    }
    
    _display.showPattern(pattern);
}

void SnakeGame::showNumber(uint8_t num) {
    if (num > 9) num = 9;
    _display.showPattern(NUMBER_PATTERNS[num]);
}

void SnakeGame::showGameOver() {
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

void SnakeGame::showReady() {
    // 闪烁的蛇头图案（准备开始）
    uint8_t pattern[25] = {0};
    pattern[12] = 1;  // 中间点
    _display.showPattern(pattern);
}
