/*
 * BleHid.cpp - BLE HID 键盘和媒体键功能实现
 * 
 * 使用 HijelHID_BLEKeyboard 库实现
 */

#include "BleHid.h"

BleHid::BleHid() : _bleKeyboard("AlphaPi Turner") {
    _connected = false;
    _deviceName = "AlphaPi Turner";
    _connectionCallback = nullptr;
}

void BleHid::begin(const char* deviceName) {
    _deviceName = deviceName;
    
    // 直接调用 begin()，不创建新对象
    _bleKeyboard.begin();
    
    _connected = false;
}

void BleHid::end() {
    _bleKeyboard.end();
    _connected = false;
}

bool BleHid::isConnected() {
    bool newState = _bleKeyboard.isConnected();
    
    // 检查连接状态变化
    if (newState != _connected) {
        _connected = newState;
        if (_connectionCallback) {
            _connectionCallback(_connected);
        }
    }
    
    return _connected;
}

void BleHid::sendKey(uint8_t key) {
    if (!_connected) return;
    _bleKeyboard.tap(key);
}

void BleHid::sendCombination(uint8_t keys[3]) {
    if (!_connected) return;
    int count = 0;
    for (int i = 0; i < 3; i++) {
        if (keys[i] != 0) {
            _bleKeyboard.press(keys[i]);
            count++;
        }
    }
    if (count > 0) {
        delay(15);
        _bleKeyboard.releaseAll();
    }
}

void BleHid::sendCombination(uint8_t keys[3], uint8_t types[3]) {
    if (!_connected) return;
    
    // 先处理键盘键（同时按下）
    int kbCount = 0;
    for (int i = 0; i < 3; i++) {
        if (keys[i] != 0 && types[i] == 0) {
            _bleKeyboard.press(keys[i]);
            kbCount++;
        }
    }
    if (kbCount > 0) {
        delay(15);
        _bleKeyboard.releaseAll();
        delay(5);
    }
    
    // 再处理媒体键（单独发送，因为媒体键是Consumer Report）
    for (int i = 0; i < 3; i++) {
        if (keys[i] != 0 && types[i] == 1) {
            uint16_t mediaKey = keys[i];  // 明确转换为uint16_t，确保调用tap(uint16_t)重载
            _bleKeyboard.tap(mediaKey);
            delay(15);
        }
    }
}

void BleHid::sendMedia(uint16_t mediaKey) {
    if (!_connected) return;
    _bleKeyboard.tap(mediaKey);
}

void BleHid::setConnectionCallback(void (*callback)(bool connected)) {
    _connectionCallback = callback;
}

void BleHid::checkConnection() {
    isConnected();
}
