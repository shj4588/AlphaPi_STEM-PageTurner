/*
 * CatchGame.h - 接球小游戏
 * 
 * 5x5 LED 点阵接球游戏
 * 
 * 控制方式：
 * - 短按 A：接球横杆左移
 * - 短按 B：球快速下落（硬降）
 * - 短按 C：接球横杆右移
 * - 长按 B：重新开始
 * 
 * 游戏规则：
 * - 底部一行有 1 个像素作为接球横杆
 * - 球从顶部随机位置下落
 * - 球落到底部时，接球横杆必须在球的正下方才能接住
 * - 接住球得分 +1，球下落速度加快
 * - 没接住球游戏结束
 * - 左右移动穿墙（碰到边界从另一边出来）
 */

#ifndef CATCH_GAME_H
#define CATCH_GAME_H

#include <Arduino.h>
#include "MatrixDisplay.h"

class CatchGame {
public:
    CatchGame(MatrixDisplay& display);
    
    // 初始化游戏
    void begin();
    
    // 更新游戏状态（需要在主循环中持续调用）
    void update();
    
    // 处理按键 A（左移）
    void onButtonA();
    
    // 处理按键 B（快速下落）
    void onButtonB();
    
    // 处理按键 C（右移）
    void onButtonC();
    
    // 游戏是否正在运行
    bool isRunning();
    
    // 退出游戏
    void exit();
    
private:
    MatrixDisplay& _display;
    
    // 游戏状态
    enum GameState {
        STATE_READY,      // 准备开始
        STATE_PLAYING,    // 游戏中
        STATE_PAUSED,     // 暂停
        STATE_GAME_OVER   // 游戏结束
    };
    
    GameState _state;
    
    // 接球横杆位置（底部一行，2个像素，记录左像素的列位置 0-3）
    uint8_t _paddleX;
    
    // 球的位置
    int8_t _ballX;   // 球的列位置 0-4
    int8_t _ballY;   // 球的行位置 0-4
    
    // 球是否存在
    bool _ballActive;
    
    // 得分
    uint16_t _score;
    
    // 球下落速度（毫秒）
    uint32_t _fallInterval;
    uint32_t _lastFallTime;
    
    // 闪烁计时
    uint32_t _blinkTimer;
    bool _blinkVisible;
    
    // 游戏结束后显示得分的计时
    uint32_t _gameOverTimer;
    bool _showScore;
    
    // 重置游戏
    void resetGame();
    
    // 生成新球
    void spawnBall();
    
    // 球下落一格
    void fall();
    
    // 左移接球横杆
    void moveLeft();
    
    // 右移接球横杆
    void moveRight();
    
    // 快速下落（硬降）
    void hardDrop();
    
    // 检查是否接住球
    bool checkCatch();
    
    // 显示游戏画面
    void render();
    
    // 显示数字（0-9）
    void showNumber(uint8_t num);
    
    // 显示游戏结束图案
    void showGameOver();
    
    // 显示准备开始图案
    void showReady();
};

#endif // CATCH_GAME_H
