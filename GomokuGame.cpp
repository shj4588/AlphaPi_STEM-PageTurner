/*
 * GomokuGame.cpp - 井字棋游戏实现
 * 5x5点阵上只有9个位置可以落子（行1,3,5 × 列1,3,5）
 * A横向移动，C纵向移动，B落子
 */

#include "GomokuGame.h"

// 5x5点阵上的9个落子位置坐标（索引0,2,4对应行/列1,3,5）
const int8_t GomokuGame::POS_X[3] = {0, 2, 4};
const int8_t GomokuGame::POS_Y[3] = {0, 2, 4};

GomokuGame::GomokuGame(MatrixDisplay& display) : _display(display) {
    _selectedRow = 1;
    _selectedCol = 1;
    _playerTurn = true;
    _gameOver = false;
    _winner = 0;
    _lastBlinkTime = 0;
    _blinkState = true;
    _lastPlayerBlinkTime = 0;
    _playerBlinkState = true;
    _aiMoveTime = 0;
    _aiThinking = false;
    for (int y = 0; y < 3; y++)
        for (int x = 0; x < 3; x++)
            _board[y][x] = EMPTY;
}

void GomokuGame::begin() {
    resetGame();
}

void GomokuGame::update() {
    // AI棋子闪烁（300ms）
    if (millis() - _lastBlinkTime >= 300) {
        _lastBlinkTime = millis();
        _blinkState = !_blinkState;
        if (!_gameOver) render();
    }
    
    // 玩家选中位置闪烁（150ms，比AI快1倍）
    if (millis() - _lastPlayerBlinkTime >= 150) {
        _lastPlayerBlinkTime = millis();
        _playerBlinkState = !_playerBlinkState;
        if (!_gameOver && _playerTurn && !_aiThinking) render();
    }
    
    // AI思考
    if (_aiThinking && millis() - _aiMoveTime >= 500) {
        _aiThinking = false;
        aiMove();
    }
}

void GomokuGame::onButtonA() {
    if (_gameOver || !_playerTurn || _aiThinking) return;
    // 横向移动（右移），跳过已落子的位置
    moveSelection(0, 1);
    render();
}

void GomokuGame::onButtonB() {
    if (_gameOver) {
        resetGame();
        return;
    }
    if (!_playerTurn || _aiThinking) return;
    playerMove();
}

void GomokuGame::onButtonC() {
    if (_gameOver || !_playerTurn || _aiThinking) return;
    // 纵向移动（下移），跳过已落子的位置
    moveSelection(1, 0);
    render();
}

void GomokuGame::exit() {
    _display.clear();
}

void GomokuGame::resetGame() {
    for (int y = 0; y < 3; y++)
        for (int x = 0; x < 3; x++)
            _board[y][x] = EMPTY;
    
    _selectedRow = 1;
    _selectedCol = 1;
    _playerTurn = true;
    _gameOver = false;
    _winner = 0;
    _aiThinking = false;
    _playerBlinkState = true;
    render();
}

void GomokuGame::moveSelection(int8_t dRow, int8_t dCol) {
    // 先统计空位数量和位置
    int8_t emptyCount = 0;
    int8_t emptyRow = -1;
    int8_t emptyCol = -1;
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            if (_board[y][x] == EMPTY) {
                emptyCount++;
                emptyRow = y;
                emptyCol = x;
            }
        }
    }
    
    // 如果只有一个空位，直接移动到那个点
    if (emptyCount == 1) {
        _selectedRow = emptyRow;
        _selectedCol = emptyCol;
        return;
    }
    
    // 如果没有空位，不移动
    if (emptyCount == 0) return;
    
    // 多个空位：先按指定方向找
    int8_t newRow = _selectedRow;
    int8_t newCol = _selectedCol;
    bool found = false;
    
    for (int i = 0; i < 9; i++) {
        newRow = (newRow + dRow + 3) % 3;
        newCol = (newCol + dCol + 3) % 3;
        if (_board[newRow][newCol] == EMPTY) {
            found = true;
            break;
        }
    }
    
    // 如果指定方向找不到空位（横竖都被填上），尝试斜向移动
    if (!found) {
        newRow = _selectedRow;
        newCol = _selectedCol;
        for (int i = 0; i < 9; i++) {
            newRow = (newRow + 1 + 3) % 3;  // 斜向：行+1
            newCol = (newCol + 1 + 3) % 3;  // 斜向：列+1
            if (_board[newRow][newCol] == EMPTY) {
                found = true;
                break;
            }
        }
    }
    
    if (found) {
        _selectedRow = newRow;
        _selectedCol = newCol;
    }
}

void GomokuGame::playerMove() {
    if (_board[_selectedRow][_selectedCol] != EMPTY) return;
    
    _board[_selectedRow][_selectedCol] = PLAYER;
    
    if (checkWin(_selectedRow, _selectedCol, PLAYER)) {
        _gameOver = true;
        _winner = 1;
        render();
        return;
    }
    
    if (isBoardFull()) {
        _gameOver = true;
        _winner = 3;  // 平局
        render();
        return;
    }
    
    _playerTurn = false;
    _aiThinking = true;
    _aiMoveTime = millis();
    render();
}

void GomokuGame::aiMove() {
    // 贪心策略：选择评分最高的位置
    int bestScore = -1;
    int bestRow = -1;
    int bestCol = -1;
    
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            if (_board[y][x] == EMPTY) {
                int score = evaluatePosition(y, x, AI);
                int defenseScore = evaluatePosition(y, x, PLAYER);
                score = max(score, defenseScore * 9 / 10);  // 进攻略优先
                
                if (score > bestScore) {
                    bestScore = score;
                    bestRow = y;
                    bestCol = x;
                }
            }
        }
    }
    
    if (bestRow >= 0) {
        _board[bestRow][bestCol] = AI;
        
        if (checkWin(bestRow, bestCol, AI)) {
            _gameOver = true;
            _winner = 2;
            render();
            return;
        }
        
        if (isBoardFull()) {
            _gameOver = true;
            _winner = 3;
            render();
            return;
        }
    }
    
    _playerTurn = true;
    // AI落子后，把选择位置移到下一个空位
    moveSelection(0, 1);
    render();
}

int GomokuGame::evaluatePosition(int8_t row, int8_t col, CellState player) {
    // 模拟落子后评估
    _board[row][col] = player;
    int score = 0;
    
    // 检查四个方向的连子数
    int directions[4][2] = {{0,1}, {1,0}, {1,1}, {1,-1}};
    
    for (int d = 0; d < 4; d++) {
        int count = 1;
        // 正方向
        for (int i = 1; i < 3; i++) {
            int r = row + directions[d][0] * i;
            int c = col + directions[d][1] * i;
            if (r < 0 || r >= 3 || c < 0 || c >= 3) break;
            if (_board[r][c] != player) break;
            count++;
        }
        // 反方向
        for (int i = 1; i < 3; i++) {
            int r = row - directions[d][0] * i;
            int c = col - directions[d][1] * i;
            if (r < 0 || r >= 3 || c < 0 || c >= 3) break;
            if (_board[r][c] != player) break;
            count++;
        }
        
        // 评分：3子=10000, 2子=1000, 1子=100
        if (count >= 3) score += 10000;
        else if (count == 2) score += 1000;
        else if (count == 1) score += 100;
    }
    
    _board[row][col] = EMPTY;
    return score;
}

bool GomokuGame::checkWin(int8_t row, int8_t col, CellState player) {
    int directions[4][2] = {{0,1}, {1,0}, {1,1}, {1,-1}};
    
    for (int d = 0; d < 4; d++) {
        int count = 1;
        // 正方向
        for (int i = 1; i < 3; i++) {
            int r = row + directions[d][0] * i;
            int c = col + directions[d][1] * i;
            if (r < 0 || r >= 3 || c < 0 || c >= 3) break;
            if (_board[r][c] != player) break;
            count++;
        }
        // 反方向
        for (int i = 1; i < 3; i++) {
            int r = row - directions[d][0] * i;
            int c = col - directions[d][1] * i;
            if (r < 0 || r >= 3 || c < 0 || c >= 3) break;
            if (_board[r][c] != player) break;
            count++;
        }
        
        if (count >= 3) return true;
    }
    return false;
}

bool GomokuGame::isBoardFull() {
    for (int y = 0; y < 3; y++)
        for (int x = 0; x < 3; x++)
            if (_board[y][x] == EMPTY) return false;
    return true;
}

void GomokuGame::render() {
    uint8_t pattern[25] = {0};
    
    // 绘制棋子
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            int px = POS_X[x];
            int py = POS_Y[y];
            if (_board[y][x] == PLAYER) {
                pattern[py * 5 + px] = 1;  // 玩家：常亮
            } else if (_board[y][x] == AI) {
                // AI：闪烁
                if (_blinkState) pattern[py * 5 + px] = 1;
            }
        }
    }
    
    // 当前选中位置：快速闪烁（150ms，比AI快1倍）
    if (!_gameOver && _playerTurn && !_aiThinking) {
        int px = POS_X[_selectedCol];
        int py = POS_Y[_selectedRow];
        if (_board[_selectedRow][_selectedCol] == EMPTY) {
            if (_playerBlinkState) pattern[py * 5 + px] = 1;
        }
    }
    
    // 游戏结束显示获胜者
    if (_gameOver) {
        if (_winner == 1) {
            // 玩家赢：显示笑脸（O）
            uint8_t winPattern[25] = {
                0,1,1,1,0,
                1,0,0,0,1,
                1,0,1,0,1,
                1,0,0,0,1,
                0,1,1,1,0
            };
            for (int i = 0; i < 25; i++) pattern[i] = winPattern[i];
        } else if (_winner == 2) {
            // AI赢：显示X
            uint8_t winPattern[25] = {
                1,0,0,0,1,
                0,1,0,1,0,
                0,0,1,0,0,
                0,1,0,1,0,
                1,0,0,0,1
            };
            for (int i = 0; i < 25; i++) pattern[i] = winPattern[i];
        } else {
            // 平局：显示0
            uint8_t drawPattern[25] = {
                0,1,1,1,0,
                1,0,0,0,1,
                1,0,0,0,1,
                1,0,0,0,1,
                0,1,1,1,0
            };
            for (int i = 0; i < 25; i++) pattern[i] = drawPattern[i];
        }
    }
    
    _display.showPattern(pattern);
}
