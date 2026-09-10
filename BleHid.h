/*
 * BleHid.h - BLE HID 键盘和媒体键功能
 * 
 * 使用 HijelHID_BLEKeyboard 库实现
 * 
 * 支持：
 * - 键盘键（方向键、Page Up/Down 等）
 * - 媒体键（音量加减、播放暂停、上一曲/下一曲等）
 */

#ifndef BLEHID_H
#define BLEHID_H

#include <Arduino.h>
#include <HijelHID_BLEKeyboard.h>

// 键盘修饰键
#define KEY_MOD_NONE    0x00
#define KEY_MOD_CTRL    0x01
#define KEY_MOD_SHIFT   0x02
#define KEY_MOD_ALT     0x04
#define KEY_MOD_WIN     0x08

// 常用键盘键码（使用库中定义的键码）
// KEY_PAGE_UP, KEY_PAGE_DOWN, KEY_RIGHT_ARROW, KEY_LEFT_ARROW, KEY_UP_ARROW, KEY_DOWN_ARROW 等

// 媒体键码（使用库中定义的键码）
// MEDIA_NEXT_TRACK, MEDIA_PREV_TRACK, MEDIA_STOP, MEDIA_PLAY_PAUSE, 
// MEDIA_MUTE, MEDIA_VOLUME_UP, MEDIA_VOLUME_DOWN 等

class BleHid {
public:
    BleHid();
    
    // 初始化 BLE HID
    void begin(const char* deviceName = "AlphaPi Turner");
    
    // 停止 BLE HID
    void end();
    
    // 是否连接
    bool isConnected();
    
    // 发送键盘键（按下并释放）
    void sendKey(uint8_t key);
    
    // 发送媒体键（按下并释放）
    void sendMedia(uint16_t mediaKey);
    
    // 设置连接状态回调
    void setConnectionCallback(void (*callback)(bool connected));
    
private:
    HijelHID_BLEKeyboard _bleKeyboard;
    bool _connected;
    const char* _deviceName;
    
    void (*_connectionCallback)(bool connected);
    
    // 检查连接状态变化
    void checkConnection();
};

#endif // BLEHID_H
