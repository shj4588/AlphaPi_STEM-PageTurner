/*
 * TetrisGame.h - 简化版俄罗斯方块（2x2格子）
 * 
 * 5x5 LED 点阵俄罗斯方块游戏
 * 方块大小不超过 2x2
 * 
 * 控制方式：
 * - 短按 A：左移
 * - 短按 B：旋转方块
 * - 短按 C：右移
 * - 长按 B：重新开始
 */

#ifndef TETRIS_GAME_H
#define TETRIS_GAME_H

#include <Arduino.h>
#include "MatrixDisplay.h"

class TetrisGame {
public:
    TetrisGame(MatrixDisplay& display);
    
    void begin();
    void update();
    void onButtonA();  // 左移
    void onButtonB();  // 旋转
    void onButtonC();  // 右移
    void exit();
    
private:
    MatrixDisplay& _display;
    
    enum GameState {
        STATE_PLAYING,
        STATE_GAME_OVER
    };
    
    GameState _state;
    
    // 游戏区域：5列 x 5行
    uint8_t _board[5][5];
    
    // 当前方块（2x2 网格内的形状）
    uint8_t _shape[2][2];  // 当前形状（旋转后）
    uint8_t _shapeType;     // 形状类型索引
    int8_t _x;              // 方块左上角 X 位置
    int8_t _y;              // 方块左上角 Y 位置
    
    uint16_t _score;
    uint32_t _fallInterval;
    uint32_t _lastFallTime;
    uint32_t _gameOverTimer;
    
    // 5种基础形状（2x2）
    // 0: 单点  1: 横向  2: 纵向  3: 2x2方块  4: L形(3点缺一角)
    static const uint8_t NUM_SHAPES = 5;
    static const uint8_t SHAPES[5][2][2];
    
    void resetGame();
    void spawnPiece();
    void fall();
    void moveLeft();
    void moveRight();
    void rotate();
    bool checkCollision(int8_t newX, int8_t newY, const uint8_t shape[2][2]);
    void lockPiece();
    void clearLines();
    void render();
    void showGameOver();
};

#endif
