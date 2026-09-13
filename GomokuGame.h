/*
 * GomokuGame.h - 井字棋游戏
 * 5x5点阵上只有9个位置可以落子（行1,3,5 × 列1,3,5）
 * A横向移动，C纵向移动，B落子
 * 3子连珠获胜（横、竖、斜）
 */

#ifndef GOMOKUGAME_H
#define GOMOKUGAME_H

#include <Arduino.h>
#include "MatrixDisplay.h"

class GomokuGame {
public:
    GomokuGame(MatrixDisplay& display);
    
    void begin();
    void update();
    void onButtonA();  // 横向移动（右移）
    void onButtonB();  // 落子
    void onButtonC();  // 纵向移动（下移）
    void exit();
    
private:
    MatrixDisplay& _display;
    
    enum CellState {
        EMPTY = 0,
        PLAYER = 1,
        AI = 2
    };
    
    // 3x3逻辑棋盘（对应5x5点阵上的9个位置）
    CellState _board[3][3];
    
    // 当前选中位置（0-2，逻辑坐标）
    int8_t _selectedRow;
    int8_t _selectedCol;
    
    bool _playerTurn;
    bool _gameOver;
    int8_t _winner;  // 0=无, 1=玩家赢, 2=AI赢, 3=平局
    
    uint32_t _lastBlinkTime;
    bool _blinkState;
    uint32_t _lastPlayerBlinkTime;
    bool _playerBlinkState;
    
    uint32_t _aiMoveTime;
    bool _aiThinking;
    
    // 5x5点阵上的9个落子位置坐标
    static const int8_t POS_X[3];
    static const int8_t POS_Y[3];
    
    void resetGame();
    void playerMove();
    void aiMove();
    bool checkWin(int8_t row, int8_t col, CellState player);
    bool isBoardFull();
    int evaluatePosition(int8_t row, int8_t col, CellState player);
    void moveSelection(int8_t dRow, int8_t dCol);
    void render();
};

#endif
