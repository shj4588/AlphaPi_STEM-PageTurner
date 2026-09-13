/*
 * FlappyGame.h - 像素鸟游戏
 * A上升，C下降，不自动下落
 * 小鸟上下移动，穿过管道间隙
 */

#ifndef FLAPPYGAME_H
#define FLAPPYGAME_H

#include <Arduino.h>
#include "MatrixDisplay.h"

class FlappyGame {
public:
    FlappyGame(MatrixDisplay& display);
    
    void begin();
    void update();
    void onButtonA();  // 上升
    void onButtonB();  // 暂停/继续
    void onButtonC();  // 下降
    void exit();
    
private:
    MatrixDisplay& _display;
    
    enum State {
        STATE_READY,
        STATE_PLAYING,
        STATE_PAUSED,
        STATE_GAMEOVER
    };
    
    State _state;
    int8_t _birdY;  // 小鸟Y位置（0-4）
    int8_t _pipeX;  // 管道X位置
    int8_t _gapY;  // 间隙中心Y位置
    int8_t _gapSize;  // 当前间隙大小（1-3）
    uint32_t _lastMoveTime;
    uint32_t _moveInterval;
    uint16_t _score;
    uint32_t _gameOverTime;
    bool _scoreCounted;
    
    static const uint8_t BIRD_X = 0;  // 鸟在最左侧
    static const uint8_t GAP_MIN = 1;  // 最小间隙1格
    static const uint8_t GAP_MAX = 3;  // 最大间隙3格
    static const uint32_t INITIAL_INTERVAL = 300;
    static const uint32_t MIN_INTERVAL = 150;
    
    void moveUp();
    void moveDown();
    void movePipe();
    void checkCollisions();
    void spawnPipe();
    void render();
    void resetGame();
};

#endif
