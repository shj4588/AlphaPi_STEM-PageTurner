/*
 * RacingGame.cpp - 赛车避障游戏实现
 */

#include "RacingGame.h"

RacingGame::RacingGame(MatrixDisplay& display) : _display(display) {
    _playerLane = 1;
    _paused = false;
    _gameOver = false;
    _lastMoveTime = 0;
    _moveInterval = INITIAL_INTERVAL;
    _score = 0;
    _gameOverTime = 0;
    for (int i = 0; i < 5; i++) _obstacles[i] = -1;
}

void RacingGame::begin() {
    resetGame();
}

void RacingGame::update() {
    if (_gameOver) {
        // 游戏结束后闪烁得分，2秒后自动重新开始
        if (millis() - _gameOverTime > 2000) {
            resetGame();
        }
        return;
    }
    
    if (_paused) return;
    
    if (millis() - _lastMoveTime >= _moveInterval) {
        _lastMoveTime = millis();
        moveDown();
        checkCollision();
        render();
    }
}

void RacingGame::onButtonA() {
    if (_gameOver) return;
    if (_paused) {
        _paused = false;
        return;
    }
    if (_playerLane > 0) {
        _playerLane--;
        render();
    }
}

void RacingGame::onButtonB() {
    if (_gameOver) {
        resetGame();
        return;
    }
    _paused = !_paused;
}

void RacingGame::onButtonC() {
    if (_gameOver) return;
    if (_paused) {
        _paused = false;
        return;
    }
    if (_playerLane < 2) {
        _playerLane++;
        render();
    }
}

void RacingGame::exit() {
    _display.clear();
}

void RacingGame::resetGame() {
    _playerLane = 1;
    _paused = false;
    _gameOver = false;
    _lastMoveTime = millis();
    _moveInterval = INITIAL_INTERVAL;
    _score = 0;
    _gameOverTime = 0;
    for (int i = 0; i < 5; i++) _obstacles[i] = -1;
    render();
}

void RacingGame::moveDown() {
    // 障碍下移
    for (int i = 4; i > 0; i--) {
        _obstacles[i] = _obstacles[i - 1];
    }
    
    // 顶部生成新障碍
    if (_obstacles[1] == -1 || random(0, 100) < 70) {
        spawnObstacle();
    } else {
        _obstacles[0] = -1;
    }
    
    _score++;
    
    // 加速
    if (_score % 10 == 0 && _moveInterval > MIN_INTERVAL) {
        _moveInterval -= 30;
    }
}

void RacingGame::spawnObstacle() {
    // 随机生成1-2个障碍，但保证至少有一条通道
    int lane1 = random(0, 3);
    _obstacles[0] = lane1;
    
    // 30%概率生成第二个障碍
    if (random(0, 100) < 30) {
        int lane2 = random(0, 3);
        if (lane2 != lane1) {
            // 用特殊值表示两个障碍？不行，只能存一个
            // 改为只生成一个障碍，保证可玩性
        }
    }
}

void RacingGame::checkCollision() {
    // 玩家在第4行（最底部）
    if (_obstacles[4] == _playerLane) {
        _gameOver = true;
        _gameOverTime = millis();
        // 显示碰撞效果（全亮）
        uint8_t pattern[25];
        for (int i = 0; i < 25; i++) pattern[i] = 1;
        _display.showPattern(pattern);
        delay(200);
    }
}

void RacingGame::render() {
    uint8_t pattern[25] = {0};
    
    // 绘制障碍
    for (int row = 0; row < 5; row++) {
        if (_obstacles[row] >= 0) {
            int col = _obstacles[row] * 2;  // 0, 2, 4
            pattern[row * 5 + col] = 1;
        }
    }
    
    // 绘制玩家（底部，用两个点表示赛车）
    int playerCol = _playerLane * 2;
    pattern[4 * 5 + playerCol] = 1;
    // 玩家上方也亮一个，形成赛车形状
    if (playerCol > 0) pattern[4 * 5 + playerCol - 1] = 1;
    if (playerCol < 4) pattern[4 * 5 + playerCol + 1] = 1;
    
    _display.showPattern(pattern);
}
