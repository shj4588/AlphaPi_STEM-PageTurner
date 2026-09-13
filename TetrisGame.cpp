/*
 * TetrisGame.cpp - 简化版俄罗斯方块实现（2x2格子）
 */

#include "TetrisGame.h"

// 5种基础形状（2x2网格）
// 0: 单点      1: 横向两个   2: 纵向两个   3: 2x2方块   4: L形(3点缺一角)
const uint8_t TetrisGame::SHAPES[5][2][2] = {
    {{1, 0}, {0, 0}},  // 单点
    {{1, 1}, {0, 0}},  // 横向
    {{1, 0}, {1, 0}},  // 纵向
    {{1, 1}, {1, 1}},  // 2x2方块
    {{1, 1}, {1, 0}}   // L形（去掉右下角）
};

TetrisGame::TetrisGame(MatrixDisplay& display) : _display(display) {
    _state = STATE_PLAYING;
    _score = 0;
    _fallInterval = 1200;  // 初始下落速度（慢一点，适合2x2）
    _lastFallTime = 0;
    _gameOverTimer = 0;
    _x = 0;
    _y = 0;
    _shapeType = 0;
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 5; x++)
            _board[y][x] = 0;
    for (int y = 0; y < 2; y++)
        for (int x = 0; x < 2; x++)
            _shape[y][x] = 0;
}

void TetrisGame::begin() {
    resetGame();
}

void TetrisGame::update() {
    if (_state == STATE_GAME_OVER) {
        if (millis() - _gameOverTimer > 2000) {
            resetGame();
        }
        return;
    }
    
    if (millis() - _lastFallTime >= _fallInterval) {
        _lastFallTime = millis();
        fall();
    }
    
    render();
}

void TetrisGame::onButtonA() {
    if (_state == STATE_GAME_OVER) {
        resetGame();
        return;
    }
    moveLeft();
}

void TetrisGame::onButtonB() {
    if (_state == STATE_GAME_OVER) {
        resetGame();
        return;
    }
    rotate();
}

void TetrisGame::onButtonC() {
    if (_state == STATE_GAME_OVER) {
        resetGame();
        return;
    }
    moveRight();
}

void TetrisGame::exit() {
    _display.clear();
}

void TetrisGame::resetGame() {
    _state = STATE_PLAYING;
    _score = 0;
    _fallInterval = 1200;
    _lastFallTime = millis();
    _gameOverTimer = 0;
    
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 5; x++)
            _board[y][x] = 0;
    
    spawnPiece();
}

void TetrisGame::spawnPiece() {
    _shapeType = random(0, 5);
    
    // 复制形状
    for (int y = 0; y < 2; y++)
        for (int x = 0; x < 2; x++)
            _shape[y][x] = SHAPES[_shapeType][y][x];
    
    // 初始位置：顶部中间
    _x = 2;
    _y = 0;
    
    // 如果生成位置就碰撞，游戏结束
    if (checkCollision(_x, _y, _shape)) {
        _state = STATE_GAME_OVER;
        _gameOverTimer = millis();
        showGameOver();
    }
}

void TetrisGame::fall() {
    if (!checkCollision(_x, _y + 1, _shape)) {
        _y++;
    } else {
        lockPiece();
        clearLines();
        spawnPiece();
    }
}

void TetrisGame::moveLeft() {
    // 穿墙模式：左移时如果超出左边界，从右边出来
    int8_t newX = _x - 1;
    if (newX < -1) newX = 4;  // 2x2方块最左可以到-1（只有右列在屏幕内）
    
    if (!checkCollision(newX, _y, _shape)) {
        _x = newX;
    }
}

void TetrisGame::moveRight() {
    // 穿墙模式：右移时如果超出右边界，从左边出来
    int8_t newX = _x + 1;
    if (newX > 4) newX = -1;  // 2x2方块最右可以到4（只有左列在屏幕内）
    
    if (!checkCollision(newX, _y, _shape)) {
        _x = newX;
    }
}

void TetrisGame::rotate() {
    // 2x2 网格内旋转：顺时针旋转90度
    uint8_t rotated[2][2];
    rotated[0][0] = _shape[1][0];
    rotated[0][1] = _shape[0][0];
    rotated[1][0] = _shape[1][1];
    rotated[1][1] = _shape[0][1];
    
    // 尝试旋转，如果碰撞则不旋转
    if (!checkCollision(_x, _y, rotated)) {
        for (int y = 0; y < 2; y++)
            for (int x = 0; x < 2; x++)
                _shape[y][x] = rotated[y][x];
    }
}

bool TetrisGame::checkCollision(int8_t newX, int8_t newY, const uint8_t shape[2][2]) {
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            if (shape[y][x]) {
                int boardX = newX + x;
                int boardY = newY + y;
                
                // 检查底部边界
                if (boardY >= 5) return true;
                
                // 检查左右边界（只有完全在屏幕外才不算碰撞）
                if (boardX < 0 || boardX >= 5) continue;  // 在屏幕外，不算碰撞（穿墙）
                
                // 检查与已固定方块的碰撞
                if (boardY >= 0 && _board[boardY][boardX]) return true;
            }
        }
    }
    return false;
}

void TetrisGame::lockPiece() {
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            if (_shape[y][x]) {
                int boardX = _x + x;
                int boardY = _y + y;
                
                // 只固定在屏幕内的部分
                if (boardX >= 0 && boardX < 5 && boardY >= 0 && boardY < 5) {
                    _board[boardY][boardX] = 1;
                }
            }
        }
    }
}

void TetrisGame::clearLines() {
    uint8_t linesCleared = 0;
    
    for (int y = 4; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < 5; x++) {
            if (!_board[y][x]) {
                full = false;
                break;
            }
        }
        
        if (full) {
            linesCleared++;
            // 消除这一行，上面的行下移
            for (int yy = y; yy > 0; yy--) {
                for (int x = 0; x < 5; x++) {
                    _board[yy][x] = _board[yy - 1][x];
                }
            }
            // 顶部清空
            for (int x = 0; x < 5; x++) {
                _board[0][x] = 0;
            }
            y++;  // 重新检查当前行（因为下移了）
        }
    }
    
    if (linesCleared > 0) {
        _score += linesCleared * 10;
        
        // 加速
        if (_fallInterval > 400) {
            _fallInterval -= 50;
        }
    }
}

void TetrisGame::render() {
    uint8_t pattern[25] = {0};
    
    // 绘制已固定的方块
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            if (_board[y][x]) {
                pattern[y * 5 + x] = 1;
            }
        }
    }
    
    // 绘制当前方块
    if (_state == STATE_PLAYING) {
        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < 2; x++) {
                if (_shape[y][x]) {
                    int boardX = _x + x;
                    int boardY = _y + y;
                    if (boardX >= 0 && boardX < 5 && boardY >= 0 && boardY < 5) {
                        pattern[boardY * 5 + boardX] = 1;
                    }
                }
            }
        }
    }
    
    _display.showPattern(pattern);
}

void TetrisGame::showGameOver() {
    // 显示 X 图案
    uint8_t pattern[25] = {
        1,0,0,0,1,
        0,1,0,1,0,
        0,0,1,0,0,
        0,1,0,1,0,
        1,0,0,0,1
    };
    _display.showPattern(pattern);
}
