/*
 * Buttons.h - 按键处理（消抖、短按/长按）
 * 
 * 硬件：
 * - 按键 A=GPIO10, B=GPIO1, C=GPIO3
 * - 下拉输入，按下高电平
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>

class Button {
public:
    Button(uint8_t pin, const char* name);
    
    // 更新按键状态（需要在主循环中持续调用）
    void update();
    
    // 是否被按下（短按，触发后自动清除）
    bool pressed();
    
    // 是否被长按（触发后自动清除）
    bool longPressed();
    
    // 是否正在按下
    bool isDown();
    
    // 设置长按时间（毫秒）
    void setLongPressTime(uint32_t ms);
    
    // 获取按键名称
    const char* getName();
    
private:
    uint8_t _pin;
    const char* _name;
    
    // 消抖
    bool _lastState;
    bool _stableState;
    uint32_t _lastChangeTime;
    static const uint32_t DEBOUNCE_MS = 20;
    
    // 长按
    uint32_t _pressStartTime;
    uint32_t _longPressTime;
    bool _longPressTriggered;
    
    // 事件
    bool _pressedEvent;
    bool _longPressedEvent;
};

class Buttons {
public:
    Buttons();
    
    // 初始化所有按键
    void begin();
    
    // 更新所有按键状态（需要在主循环中持续调用）
    void update();
    
    // 按键对象
    Button A;
    Button B;
    Button C;
};

#endif // BUTTONS_H
