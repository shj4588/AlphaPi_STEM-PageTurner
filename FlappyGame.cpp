/*
 * FlappyGame.cpp - 像素鸟游戏实现
 * A上升，C下降，不自动下落
 */

#include "FlappyGame.h"

FlappyGame::FlappyGame(MatrixDisplay& display) : _display(display) {
    _state = STATE_READY;
    _birdY = 2;
    _pipeX = 5;
    _gapY = 2;
    _gapSize = 2;
    _lastMoveTime = 0;
    _moveInterval = INITIAL_INTERVAL;
    _score = 0;
    _gameOverTime = 0;
    _scoreCounted = false;
}

void FlappyGame::begin() {
    resetGame();
}

void FlappyGame::update() {
    if (_state == STATE_GAMEOVER) {
        if (millis() - _gameOverTime > 2000) {
            resetGame();
        }
        return;
    }
    
    if (_state != STATE_PLAYING) return;
    
    // 不自动下落，小鸟位置由按键控制
    
    if (millis() - _lastMoveTime >= _moveInterval) {
        _lastMoveTime = millis();
        movePipe();
        checkCollisions();
    }
    
    render();
}

void FlappyGame::onButtonA() {
    if (_state == STATE_GAMEOVER) {
        resetGame();
        return;
    }
    if (_state == STATE_READY) {
        _state = STATE_PLAYING;
        _lastMoveTime = millis();
    } else if (_state == STATE_PAUSED) {
        _state = STATE_PLAYING;
        return;
    }
    moveUp();
}

void FlappyGame::onButtonB() {
    if (_state == STATE_GAMEOVER) {
        resetGame();
        return;
    }
    if (_state == STATE_PLAYING) {
        _state = STATE_PAUSED;
    } else if (_state == STATE_PAUSED) {
        _state = STATE_PLAYING;
    }
}

void FlappyGame::onButtonC() {
    if (_state == STATE_GAMEOVER) {
        resetGame();
        return;
    }
    if (_state == STATE_READY) {
        _state = STATE_PLAYING;
        _lastMoveTime = millis();
    } else if (_state == STATE_PAUSED) {
        _state = STATE_PLAYING;
        return;
    }
    moveDown();
}

void FlappyGame::exit() {
    _display.clear();
}

void FlappyGame::resetGame() {
    _state = STATE_READY;
    _birdY = 2;
    _pipeX = 5;
    _gapY = 2;
    _gapSize = 2;
    _lastMoveTime = millis();
    _moveInterval = INITIAL_INTERVAL;
    _score = 0;
    _gameOverTime = 0;
    _scoreCounted = false;
    render();
}

void FlappyGame::moveUp() {
    if (_birdY > 0) {
        _birdY--;
        checkCollisions();
        render();
    }
}

void FlappyGame::moveDown() {
    if (_birdY < 4) {
        _birdY++;
        checkCollisions();
        render();
    }
}

void FlappyGame::movePipe() {
    _pipeX--;
    
    if (_pipeX < 0) {
        // 计分
        if (!_scoreCounted) {
            _score++;
            _scoreCounted = true;
            
            // 加速
            if (_moveInterval > MIN_INTERVAL) {
                _moveInterval -= 10;
            }
        }
        spawnPipe();
    }
}

void FlappyGame::spawnPipe() {
    _pipeX = 4;
    // 随机间隙大小1-3格
    _gapSize = random(GAP_MIN, GAP_MAX + 1);
    // 间隙中心位置，确保间隙在屏幕内（0-4）
    int halfGap = _gapSize / 2;
    int minY = halfGap;
    int maxY = 4 - halfGap;
    if (minY > maxY) minY = maxY;
    _gapY = random(minY, maxY + 1);
    _scoreCounted = false;
}

void FlappyGame::checkCollisions() {
    // 管道在小鸟的X位置时检查碰撞
    if (_pipeX == BIRD_X) {
        // 检查是否在间隙内
        int gapTop = _gapY - _gapSize / 2;
        int gapBottom = _gapY + _gapSize / 2;
        
        if (_birdY < gapTop || _birdY > gapBottom) {
            // 撞到管道
            _state = STATE_GAMEOVER;
            _gameOverTime = millis();
            render();
        }
    }
}

void FlappyGame::render() {
    uint8_t pattern[25] = {0};
    
    // 绘制管道
    if (_pipeX >= 0 && _pipeX < 5) {
        int gapTop = _gapY - _gapSize / 2;
        int gapBottom = _gapY + _gapSize / 2;
        
        for (int y = 0; y < 5; y++) {
            if (y < gapTop || y > gapBottom) {
                pattern[y * 5 + _pipeX] = 1;
            }
        }
    }
    
    // 绘制小鸟（单像素点）
    if (_birdY >= 0 && _birdY < 5) {
        pattern[_birdY * 5 + BIRD_X] = 1;
    }
    
    // 游戏结束显示
    if (_state == STATE_GAMEOVER) {
        // X
        uint8_t gameOverPattern[25] = {
            1,0,0,0,1,
            0,1,0,1,0,
            0,0,1,0,0,
            0,1,0,1,0,
            1,0,0,0,1
        };
        for (int i = 0; i < 25; i++) pattern[i] = gameOverPattern[i];
    }
    
    _display.showPattern(pattern);
}
