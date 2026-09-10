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
