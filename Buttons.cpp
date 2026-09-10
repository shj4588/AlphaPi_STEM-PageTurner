/*
 * Buttons.cpp - 按键处理实现（消抖、短按/长按）
 */

#include "Buttons.h"

// 按键引脚定义
#define PIN_A 10
#define PIN_B 1
#define PIN_C 3

Button::Button(uint8_t pin, const char* name) {
    _pin = pin;
    _name = name;
    _lastState = false;
    _stableState = false;
    _lastChangeTime = 0;
    _pressStartTime = 0;
    _longPressTime = 800;  // 默认长按时间 800ms
    _longPressTriggered = false;
    _pressedEvent = false;
    _longPressedEvent = false;
}

void Button::update() {
    // 读取当前状态（高电平表示按下）
    bool currentState = digitalRead(_pin) == HIGH;
    
    // 消抖
    if (currentState != _lastState) {
        _lastChangeTime = millis();
        _lastState = currentState;
    }
    
    if ((millis() - _lastChangeTime) > DEBOUNCE_MS) {
        if (currentState != _stableState) {
            _stableState = currentState;
            
            if (_stableState) {
                // 按下
                _pressStartTime = millis();
                _longPressTriggered = false;
            } else {
                // 松开
                if (!_longPressTriggered) {
                    // 短按事件
                    _pressedEvent = true;
                }
            }
        }
    }
    
    // 长按检测
    if (_stableState && !_longPressTriggered) {
        if (millis() - _pressStartTime >= _longPressTime) {
            _longPressTriggered = true;
            _longPressedEvent = true;
        }
    }
}

bool Button::pressed() {
    if (_pressedEvent) {
        _pressedEvent = false;
        return true;
    }
    return false;
}

bool Button::longPressed() {
    if (_longPressedEvent) {
        _longPressedEvent = false;
        return true;
    }
    return false;
}

bool Button::isDown() {
    return _stableState;
}

void Button::setLongPressTime(uint32_t ms) {
    _longPressTime = ms;
}

const char* Button::getName() {
    return _name;
}

Buttons::Buttons() : A(PIN_A, "A"), B(PIN_B, "B"), C(PIN_C, "C") {
}

void Buttons::begin() {
    // 设置按键引脚为输入（下拉输入，按下高电平）
    pinMode(PIN_A, INPUT);
    pinMode(PIN_B, INPUT);
    pinMode(PIN_C, INPUT);
}

void Buttons::update() {
    A.update();
    B.update();
    C.update();
}
