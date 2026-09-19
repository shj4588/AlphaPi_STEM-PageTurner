/*
 * AudioDriver.h - AlphaPi STEM 音频驱动
 * 
 * 基于翻页器固件 MatrixDisplay 的协议框架
 * 
 * 硬件：
 * - 协控芯片通过 UART1 连接
 * - 波特率：460929
 * - TX = GPIO8, RX = GPIO9
 * 
 * 协议框架（来自翻页器固件 MatrixDisplay）：
 * - 帧格式：[0x90, addr, len(data), data..., checksum]
 * - 校验和：(0x90 + addr + len + sum(data)) & 0xFF
 * - 响应：3字节，第3字节=0x05表示成功
 * 
 * 已知地址：
 * - addr=8: 点阵显示（5字节数据，每字节对应一列）
 * - 音频地址待探索
 */

#ifndef AudioDriver_h
#define AudioDriver_h

#include <Arduino.h>
#include <HardwareSerial.h>
#include <LittleFS.h>

class AudioDriver {
public:
    AudioDriver();
    ~AudioDriver();
    
    // 初始化音频驱动（UART1）
    bool begin();
    
    // 测试点阵显示（验证通信是否正常）
    bool testMatrix();
    
    // 播放正弦波测试音（用于测试扬声器）
    // freq: 频率（Hz）
    // duration: 时长（毫秒）
    // volume: 音量（0-100）
    bool playTone(uint16_t freq = 440, uint16_t duration = 1000, uint8_t volume = 50);
    
    // 用指定地址播放测试音（用于地址扫描）
    bool playToneWithAddr(uint8_t addr, uint16_t freq, uint16_t duration, uint8_t volume);
    
    // 发送命令并返回响应状态码（用于地址扫描）
    // 返回: 响应的第3字节（0x05=成功，其他=失败或无响应=0x00）
    uint8_t sendCommandAndGetStatus(uint8_t addr, const uint8_t* data, uint8_t len);
    
    // 播放WAV文件
    bool playFile(const char* filename);
    
    // 停止播放
    void stop();
    
    // 是否正在播放
    bool isPlaying() { return _playing; }
    
    // 发送原始命令（用于调试）
    bool sendCommand(uint8_t addr, const uint8_t* data, uint8_t len);
    
private:
    bool _initialized;
    bool _playing;
    HardwareSerial* _uart;
    
    // 计算校验和
    uint8_t calcChecksum(const uint8_t* buf, uint8_t len);
};

#endif
