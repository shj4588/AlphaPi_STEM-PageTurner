/*
 * DiceGame.h - 摇色子小游戏
 * 
 * 5x5 LED 点阵摇色子游戏
 * 
 * 控制方式：
 * - 摇动设备：摇色子（随机显示 1-6 点）
 * - 短按任意键：也可以摇色子
 * - 长按 B：重新开始（摇一次）
 * 
 * 游戏规则：
 * - 进入游戏后显示一个色子
 * - 摇动设备，色子会快速滚动
 * - 摇动停止后显示最终的随机数字（1-6）
 * - 色子点数用标准色子布局显示
 */

#ifndef DICE_GAME_H
#define DICE_GAME_H

#include <Arduino.h>
#include "MatrixDisplay.h"
#include "SC7A20.h"

class DiceGame {
public:
    DiceGame(MatrixDisplay& display, SC7A20& accel);
    
    // 初始化游戏
    void begin();
    
    // 更新游戏状态（需要在主循环中持续调用）
    void update();
    
    // 处理按键 A（摇色子）
    void onButtonA();
    
    // 处理按键 B（摇色子）
    void onButtonB();
    
    // 处理按键 C（摇色子）
    void onButtonC();
    
    // 游戏是否正在运行
    bool isRunning();
    
    // 退出游戏
    void exit();
    
private:
    MatrixDisplay& _display;
    SC7A20& _accel;
    
    // 游戏状态
    enum GameState {
        STATE_READY,      // 准备状态
        STATE_ROLLING,    // 摇动中
        STATE_RESULT      // 显示结果
    };
    
    GameState _state;
    
    // 当前显示的数字
    uint8_t _currentNumber;
    
    // 摇动检测
    int16_t _lastX, _lastY, _lastZ;
    bool _hasLastData;
    uint32_t _lastReadTime;
    uint32_t _lastShakeTime;
    uint32_t _lastNumberChangeTime;
    
    // 色子图案（1-6，每个 25 个点，行优先）
    static const uint8_t DICE_PATTERNS[7][25];
    
    // 摇色子（生成新的随机数字）
    void rollDice();
    
    // 显示指定数字的色子
    void showDice(uint8_t num);
    
    // 检测摇动
    bool detectShake();
};

#endif // DICE_GAME_H
