/*
 * StopwatchGame.h - 秒表小游戏
 * 
 * 5x5 LED 点阵秒表游戏
 * 
 * 控制方式：
 * - 短按 A：开始/继续计时
 * - 短按 B：暂停
 * - 短按 C：重置/清零
 * - 长按 B：重新开始
 * 
 * 游戏规则：
 * 1. 进入游戏显示 0（准备状态）
 * 2. 按 A 开始计时，显示当前秒数
 * 3. 按 B 暂停，显示 'P'
 * 4. 按 A 继续计时
 * 5. 按 C 重置，回到 0
 * 6. 秒数显示个位数（0-9 循环）
 */

#ifndef STOPWATCH_GAME_H
#define STOPWATCH_GAME_H

#include <Arduino.h>
#include "MatrixDisplay.h"

class StopwatchGame {
public:
    StopwatchGame(MatrixDisplay& display);
    
    // 初始化游戏
    void begin();
    
    // 更新游戏状态（需要在主循环中持续调用）
    void update();
    
    // 处理按键 A（开始/继续）
    void onButtonA();
    
    // 处理按键 B（暂停）
    void onButtonB();
    
    // 处理按键 C（重置）
    void onButtonC();
    
    // 游戏是否正在运行
    bool isRunning();
    
    // 退出游戏
    void exit();
    
private:
    MatrixDisplay& _display;
    
    // 游戏状态
    enum GameState {
        STATE_READY,       // 准备状态（显示 0）
        STATE_RUNNING,     // 计时中
        STATE_PAUSED       // 暂停（显示 P）
    };
    
    GameState _state;
    
    // 计时
    uint32_t _startMs;       // 开始时间
    uint32_t _elapsedMs;     // 已用时间（暂停时保存）
    uint32_t _currentSec;    // 当前秒数
    uint32_t _lastDisplayedSec;  // 上次显示的秒数
    
    // 数字和字母图案（0-9, P）
    static const uint8_t CHAR_PATTERNS[12][25];
    
    // 开始计时
    void start();
    
    // 暂停
    void pause();
    
    // 继续
    void resume();
    
    // 重置
    void reset();
    
    // 显示字符（0-9 或 P）
    void showChar(char ch);
    
    // 获取当前秒数
    uint32_t getCurrentSeconds();
};

#endif // STOPWATCH_GAME_H
