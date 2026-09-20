/*
 * BleHid.cpp - BLE HID 键盘和媒体键功能实现
 * 
 * 使用 HijelHID_BLEKeyboard 库实现
 */

#include "BleHid.h"
#include "esp_gap_ble_api.h"
#include <NimBLEDevice.h>

// 把 esp_power_level_t 枚举换算回 dBm：相对 ESP_PWR_LVL_N0 的档数 × 3
// （IDF 4.x/5.x 枚举数值不同，但 N0 都是基准档，这样换算对两版都成立）
static int8_t pwrEnumToDbm(esp_power_level_t lvl) {
    return (int8_t)(((int)lvl - (int)ESP_PWR_LVL_N0) * 3);
}

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

void BleHid::setTxPower(int8_t dbm) {
    esp_power_level_t lvl;
    switch (dbm) {
        case -12: lvl = ESP_PWR_LVL_N12; break;
        case -9:  lvl = ESP_PWR_LVL_N9;  break;
        case -6:  lvl = ESP_PWR_LVL_N6;  break;
        case -3:  lvl = ESP_PWR_LVL_N3;  break;
        case 3:   lvl = ESP_PWR_LVL_P3;  break;
        case 6:   lvl = ESP_PWR_LVL_P6;  break;
        case 9:   lvl = ESP_PWR_LVL_P9;  break;
        case 0:
        default:  lvl = ESP_PWR_LVL_N0;  break;
    }
    // 广播、扫描、默认及所有连接句柄类型统一设置，覆盖广播期和连接期
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, lvl);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN, lvl);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, lvl);
    for (int i = 0; i <= 9; i++) {
        esp_ble_tx_power_set((esp_ble_power_type_t)(ESP_BLE_PWR_TYPE_CONN_HDL0 + i), lvl);
    }
    // 读回实际生效值打印到串口验证（串口调试可用）
    int8_t rdAdv = pwrEnumToDbm(esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_ADV));
    int8_t rdDef = pwrEnumToDbm(esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_DEFAULT));
    Serial.printf("BLE TX power set to %d dBm (readback adv=%d conn=%d)\n", dbm, rdAdv, rdDef);
}

void BleHid::applyAdvInterval(uint8_t mode) {
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    if (!adv) return;

    // 单位 0.625ms，与库内 setMinInterval(0x20)/setMaxInterval(0x40) 同一量纲
    uint16_t minU = 0x20, maxU = 0x40;              // 0: 秒连 20~40ms（库默认）
    if (mode == 1)      { minU = 400; maxU = 480; }  // 1: 平衡 250~300ms
    else if (mode == 2) { minU = 800; maxU = 960; }  // 2: 省电 500~600ms
    adv->setMinInterval(minU);
    adv->setMaxInterval(maxU);

    // 正在广播时重启广播使参数立即生效；已连接时新参数会在断连后的广播中使用
    if (!_connected) {
        adv->stop();
        NimBLEDevice::startAdvertising();
    }
    Serial.printf("BLE adv interval mode=%u (%u~%u units)\n", mode, minU, maxU);
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
