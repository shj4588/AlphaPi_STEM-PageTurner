/*
 * RacingGame.h - 赛车避障游戏
 * 3条车道，左右切换躲避前方障碍
 */

#ifndef RACINGGAME_H
#define RACINGGAME_H

#include <Arduino.h>
#include "MatrixDisplay.h"

class RacingGame {
public:
    RacingGame(MatrixDisplay& display);
    
    void begin();
    void update();
    void onButtonA();  // 左切车道
    void onButtonB();  // 暂停/继续
    void onButtonC();  // 右切车道
    void exit();
    
private:
    MatrixDisplay& _display;
    
    int8_t _playerLane;  // 玩家车道：0=左, 1=中, 2=右
    int8_t _obstacles[5];  // 每行的障碍车道（-1=无障碍）
    bool _paused;
    bool _gameOver;
    uint32_t _lastMoveTime;
    uint32_t _moveInterval;
    uint16_t _score;
    uint32_t _gameOverTime;
    
    static const uint32_t INITIAL_INTERVAL = 500;
    static const uint32_t MIN_INTERVAL = 150;
    
    void moveDown();
    void spawnObstacle();
    void checkCollision();
    void render();
    void resetGame();
};

#endif
