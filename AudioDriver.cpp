/*
 * AudioDriver.cpp - AlphaPi STEM 音频驱动实现
 * 
 * 基于翻页器固件 MatrixDisplay 的协议框架
 */

#include "AudioDriver.h"

// UART1 配置
#define UART_BAUD       460929
#define UART_TX         8
#define UART_RX         9

// 协议帧格式
#define FRAME_HEADER    0x90
#define DEFAULT_ADDR    8       // 点阵显示地址

// 音频参数（待确认）
#define AUDIO_SAMPLE_RATE   22050
#define AUDIO_SILENCE       128     // 8位无符号静音值

AudioDriver::AudioDriver() {
    _initialized = false;
    _playing = false;
    _uart = nullptr;
}

AudioDriver::~AudioDriver() {
    stop();
    if (_uart) {
        delete _uart;
        _uart = nullptr;
    }
}

bool AudioDriver::begin() {
    Serial.println("[Audio] 使用 Arduino HardwareSerial 配置 UART1...");
    
    _uart = new HardwareSerial(1);
    _uart->begin(UART_BAUD, SERIAL_8N1, UART_RX, UART_TX);
    delay(100);
    
    Serial.printf("[Audio] UART1 初始化完成 (%d baud, TX=%d, RX=%d)\n", 
                  UART_BAUD, UART_TX, UART_RX);
    
    // 清空接收缓冲区
    while (_uart->available()) {
        _uart->read();
    }
    
    _initialized = true;
    Serial.println("[Audio] 音频驱动初始化完成");
    return true;
}

uint8_t AudioDriver::calcChecksum(const uint8_t* buf, uint8_t len) {
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < len - 1; i++) {
        checksum += buf[i];
    }
    return checksum & 0xFF;
}

bool AudioDriver::sendCommand(uint8_t addr, const uint8_t* data, uint8_t len) {
    return sendCommandAndGetStatus(addr, data, len) == 0x05;
}

uint8_t AudioDriver::sendCommandAndGetStatus(uint8_t addr, const uint8_t* data, uint8_t len) {
    if (!_initialized || !_uart || !data || len == 0) {
        return 0x00;
    }
    
    // 构建帧：[0x90, addr, len(data), data..., checksum]
    uint8_t buf[32];
    uint8_t idx = 0;
    buf[idx++] = FRAME_HEADER;
    buf[idx++] = addr;
    buf[idx++] = len;
    for (uint8_t i = 0; i < len; i++) {
        buf[idx++] = data[i];
    }
    buf[idx++] = 0; // 校验和占位
    
    // 计算校验和
    buf[idx - 1] = calcChecksum(buf, idx);
    
    // 清空接收缓冲区
    while (_uart->available()) {
        _uart->read();
    }
    
    // 发送帧
    _uart->write(buf, idx);
    
    // 等待响应
    delay(20);
    
    // 读取所有可用的响应字节
    int available = _uart->available();
    if (available >= 3) {
        uint8_t resp[10];
        int read_len = min(available, 10);
        _uart->readBytes(resp, read_len);
        
        // 打印完整响应（调试用）
        Serial.printf("[Scan] addr=0x%02X, %d bytes: ", addr, read_len);
        for (int i = 0; i < read_len; i++) {
            Serial.printf("%02X ", resp[i]);
        }
        Serial.println();
        
        // 响应格式应该是 [0x91, addr, status]
        // 找帧头 0x91
        for (int i = 0; i < read_len - 2; i++) {
            if (resp[i] == 0x91 && resp[i+1] == addr) {
                return resp[i+2]; // 返回状态码
            }
        }
        // 没找到正确帧头，返回第一个字节
        return resp[0];
    }
    
    return 0x00; // 无响应
}

bool AudioDriver::testMatrix() {
    if (!_initialized) return false;
    
    Serial.println("[Audio] 测试点阵显示...");
    
    // 点阵全亮（5字节，每字节0xFF = 5行全亮，左移3位）
    // 注意：MatrixDisplay中每列数据左移3位，所以全亮是0xF8
    uint8_t all_on[5] = {0xF8, 0xF8, 0xF8, 0xF8, 0xF8};
    
    Serial.println("[Audio] 点阵全亮...");
    bool result = sendCommand(DEFAULT_ADDR, all_on, 5);
    
    delay(500);
    
    // 点阵全灭
    uint8_t all_off[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    Serial.println("[Audio] 点阵全灭...");
    sendCommand(DEFAULT_ADDR, all_off, 5);
    
    return result;
}

bool AudioDriver::playTone(uint16_t freq, uint16_t duration, uint8_t volume) {
    // 默认用地址 0x14 播放（待确认）
    return playToneWithAddr(0x14, freq, duration, volume);
}

bool AudioDriver::playToneWithAddr(uint8_t addr, uint16_t freq, uint16_t duration, uint8_t volume) {
    if (!_initialized) {
        return false;
    }
    
    Serial.printf("[Audio] 播放测试音: addr=0x%02X, %d Hz, %d ms, 音量 %d%%\n", addr, freq, duration, volume);
    
    // 生成正弦波 - 8位无符号PCM，静音值128，采样率22050Hz
    uint32_t pcm_samples = (uint32_t)AUDIO_SAMPLE_RATE * duration / 1000;
    
    // 计算振幅（音量控制，最大127）
    int32_t amplitude = (int32_t)(127L * volume / 100);
    
    Serial.printf("[Audio] PCM数据: %d 字节, 振幅: %d\n", pcm_samples, amplitude);
    
    _playing = true;
    
    // 分块生成并播放（每包最多28字节数据，因为帧有4字节开销）
    uint8_t chunk_buffer[28];
    uint32_t samples_sent = 0;
    uint32_t start_time = millis();
    
    while (samples_sent < pcm_samples && _playing) {
        size_t chunk_len = 28;
        if (samples_sent + chunk_len > pcm_samples) {
            chunk_len = pcm_samples - samples_sent;
        }
        
        for (size_t i = 0; i < chunk_len; i++) {
            float t = (float)(samples_sent + i) / AUDIO_SAMPLE_RATE;
            float sine = sin(2.0 * PI * freq * t);
            
            int32_t sample = AUDIO_SILENCE + (int32_t)(sine * amplitude);
            if (sample < 0) sample = 0;
            if (sample > 255) sample = 255;
            
            chunk_buffer[i] = (uint8_t)sample;
        }
        
        // 用指定地址发送音频数据
        sendCommand(addr, chunk_buffer, chunk_len);
        samples_sent += chunk_len;
        
        delay(5);
    }
    
    uint32_t elapsed = millis() - start_time;
    Serial.printf("[Audio] 测试音播放完成, 用时 %d ms\n", elapsed);
    
    _playing = false;
    return true;
}

bool AudioDriver::playFile(const char* filename) {
    if (!_initialized || !filename) {
        return false;
    }
    
    if (!LittleFS.begin(true)) {
        Serial.println("[Audio] LittleFS 挂载失败");
        return false;
    }
    
    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.printf("[Audio] 无法打开文件: %s\n", filename);
        return false;
    }
    
    size_t file_size = file.size();
    Serial.printf("[Audio] 播放文件: %s (%d 字节)\n", filename, file_size);
    
    _playing = true;
    
    uint8_t chunk_buffer[28];
    uint32_t start_time = millis();
    
    while (file.available() && _playing) {
        size_t bytes_read = file.read(chunk_buffer, 28);
        if (bytes_read > 0) {
            sendCommand(0x14, chunk_buffer, bytes_read);
            delay(5);
        }
    }
    
    file.close();
    
    uint32_t elapsed = millis() - start_time;
    Serial.printf("[Audio] 文件播放完成, 用时 %d ms\n", elapsed);
    
    _playing = false;
    return true;
}

void AudioDriver::stop() {
    if (_playing) {
        _playing = false;
        Serial.println("[Audio] 停止播放");
    }
}
