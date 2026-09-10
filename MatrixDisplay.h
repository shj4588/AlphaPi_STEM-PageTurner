/*
 * MatrixDisplay.h - 5x5 LED 点阵显示驱动
 * 
 * 通信协议：
 * - UART1，波特率 460929，TX=GPIO8, RX=GPIO9
 * - 帧格式：[0x90, addr, len(data), data..., checksum]
 * - 校验和：(0x90 + addr + len(data) + sum(data)) & 0xFF
 * - 响应：3 字节，第 3 字节 = 0x05 表示成功
 * 
 * 数据格式（列优先）：
 * - 5 列数据，每列 1 字节
 * - 每列 5 位（对应 5 行），左移 3 位后发送
 * - bit7 = 第 1 行，bit6 = 第 2 行，bit5 = 第 3 行，bit4 = 第 4 行，bit3 = 第 5 行
 */

#ifndef MATRIX_DISPLAY_H
#define MATRIX_DISPLAY_H

#include <Arduino.h>
#include <HardwareSerial.h>

class MatrixDisplay {
public:
    MatrixDisplay();
    
    // 初始化 UART 和点阵
    void begin();
    
    // 显示 5x5 图案（25 个点，按行优先，1=亮，0=灭）
    void showPattern(const uint8_t pattern[25]);
    
    // 显示 5 行字符串图案（每行 5 个字符，'1'/'X'/'#' 表示亮）
    void showPattern(const char* rows[5]);
    
    // 清除显示
    void clear();
    
    // 显示预定义图标
    void showIcon(const char* iconName);
    
private:
    HardwareSerial* _uart;
    static const uint8_t FRAME_HEADER = 0x90;
    static const uint8_t DEFAULT_ADDR = 8;
    static const uint32_t BAUDRATE = 460929;
    static const uint8_t TX_PIN = 8;
    static const uint8_t RX_PIN = 9;
    
    // 计算校验和
    uint8_t calcChecksum(const uint8_t* buf, uint8_t len);
    
    // 发送完整帧
    bool uartWrite(uint8_t addr, const uint8_t* data, uint8_t dataLen);
    
    // 将行优先图案转换为列优先字节数据
    void patternToBytes(const uint8_t pattern[25], uint8_t bytes[5]);
    
    // 将行字符串转换为列优先字节数据
    void patternToBytes(const char* rows[5], uint8_t bytes[5]);
};

// 预定义图标（25 个点，按行优先）
extern const uint8_t ICON_PAGE[25];        // 居中字母 P
extern const uint8_t ICON_ARROW[25];       // 左右箭头
extern const uint8_t ICON_MEDIA[25];       // 5x5 大加号
extern const uint8_t ICON_MUSIC[25];       // 居中音符
extern const uint8_t ICON_PLAY[25];        // 居中实心三角形
extern const uint8_t ICON_STOP[25];        // 空心 3x3 方框
extern const uint8_t ICON_PLAY_PAUSE[25];  // 3x3 等号
extern const uint8_t ICON_CONNECTED[25];   // 方框
extern const uint8_t ICON_WAITING[25];     // 菱形
extern const uint8_t ICON_ARROW_LEFT[25];  // 左箭头
extern const uint8_t ICON_ARROW_RIGHT[25]; // 右箭头
extern const uint8_t ICON_ARROW_ON[25];    // 翻页箭头开启（双箭头）
extern const uint8_t ICON_ARROW_OFF[25];   // 翻页箭头关闭（叉号）
extern const uint8_t ICON_VOLUME_UP[25];   // 3x3 小加号
extern const uint8_t ICON_VOLUME_DOWN[25]; // 3x3 小减号
extern const uint8_t ICON_SHAKE_ON[25];    // 波浪线
extern const uint8_t ICON_SHAKE_OFF[25];   // 叉号
extern const uint8_t ICON_DIRECTION_SWAP[25]; // 圆圈
extern const uint8_t ICON_CLEAR[25];       // 全灭

#endif // MATRIX_DISPLAY_H
