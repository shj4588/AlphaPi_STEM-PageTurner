/*
 * SnakeGame.h - 贪吃蛇小游戏
 * 
 * 5x5 LED 点阵贪吃蛇游戏
 * 
 * 控制方式：
 * - 短按 A：顺时针旋转方向（上→右→下→左→上）
 * - 短按 C：逆时针旋转方向（上→左→下→右→上）
 * - 短按 B：暂停/继续
 * - 长按 B：重新开始
 * 
 * 游戏规则：
 * - 蛇在 5x5 网格中移动（穿墙模式：碰到边界从另一边出来）
 * - 吃到食物后蛇身变长，得分增加
 * - 撞到自己的身体则游戏结束
 * - 游戏结束后显示得分，按 B 重新开始
 */

#ifndef SNAKE_GAME_H
#define SNAKE_GAME_H

#include <Arduino.h>
#include "MatrixDisplay.h"

class SnakeGame {
public:
    SnakeGame(MatrixDisplay& display);
    
    // 初始化游戏
    void begin();
    
    // 更新游戏状态（需要在主循环中持续调用）
    void update();
    
    // 处理按键 A（顺时针旋转方向）
    void onButtonA();
    
    // 处理按键 B（逆时针旋转方向）
    void onButtonB();
    
    // 处理按键 C（暂停/继续/重新开始）
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
    
    // 方向
    enum Direction {
        DIR_UP = 0,
        DIR_RIGHT = 1,
        DIR_DOWN = 2,
        DIR_LEFT = 3
    };
    
    Direction _direction;
    Direction _nextDirection;
    
    // 蛇身（最多 25 节，因为 5x5=25）
    static const uint8_t MAX_SNAKE_LENGTH = 25;
    uint8_t _snakeX[MAX_SNAKE_LENGTH];
    uint8_t _snakeY[MAX_SNAKE_LENGTH];
    uint8_t _snakeLength;
    
    // 食物位置
    uint8_t _foodX;
    uint8_t _foodY;
    bool _foodVisible;  // 食物闪烁用
    
    // 得分
    uint16_t _score;
    
    // 游戏速度（毫秒）
    uint32_t _moveInterval;
    uint32_t _lastMoveTime;
    
    // 闪烁计时
    uint32_t _blinkTimer;
    
    // 游戏结束后显示得分的计时
    uint32_t _gameOverTimer;
    bool _showScore;  // true=显示得分，false=显示游戏结束图案
    
    // 重置游戏
    void resetGame();
    
    // 移动蛇
    void moveSnake();
    
    // 生成新食物
    void spawnFood();
    
    // 检查是否撞到自己
    bool checkSelfCollision(uint8_t x, uint8_t y);
    
    // 显示游戏画面
    void render();
    
    // 显示数字（0-9）在 5x5 点阵上
    void showNumber(uint8_t num);
    
    // 显示游戏结束图案
    void showGameOver();
    
    // 显示准备开始图案
    void showReady();
};

#endif // SNAKE_GAME_H
