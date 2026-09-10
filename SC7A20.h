/*
 * SC7A20.h - SC7A20 加速度计驱动与摇晃检测
 * 
 * 按照 Python 版的摇晃检测逻辑重构：
 * - 读取 3 次取中位数，过滤 SoftI2C 单次读取位错误
 * - 三轴差值之和（total = abs(dx) + abs(dy) + abs(dz)）作为摇晃幅度
 * - 3 状态机：IDLE / SHAKING / WAIT_QUIET
 * - 摇晃时长 >= 最小摇晃时长才算有效摇晃
 * - 限制最小读取间隔 20ms
 * - 静止判断用 total < shake_threshold（摇晃减弱就算静止）
 * 
 * 硬件：
 * - SoftI2C，地址 0x18，SDA=GPIO6, SCL=GPIO7
 * - 输出寄存器 OUT_X=0x02/OUT_Y=0x04/OUT_Z=0x06
 */

#ifndef SC7A20_H
#define SC7A20_H

#include <Arduino.h>
#include <Wire.h>

class SC7A20 {
public:
    SC7A20();
    
    // 初始化加速度计
    bool begin();
    
    // 读取原始加速度值（x, y, z）
    void readRaw(int16_t &x, int16_t &y, int16_t &z);
    
    // 摇晃检测（按照 Python 版逻辑实现）
    // 返回：true=检测到摇晃事件，false=无事件
    bool checkShakeEvent();
    
    // 加速度计是否初始化成功
    bool isReady();
    
    // 设置摇晃参数
    void setShakeSens(int sens);           // 摇晃阈值（三轴差值之和超过此值算摇晃中）
    void setShakeMinDurationMs(uint32_t ms); // 最小摇晃时长（摇晃至少持续这么久才算有效摇晃）
    void setQuietDurationMs(uint32_t ms);    // 静止时长（摇晃结束后静止这么久才触发翻页）
    void setShakeCooldownMs(uint32_t ms);    // 冷却时间
    
    // 兼容旧接口（内部映射到新参数）
    void setQuietSens(int sens);           // 不再使用，保留兼容
    void setShakeWinMs(uint32_t ms);       // 不再使用，保留兼容
    void setQuietHoldMs(uint32_t ms);      // 映射到 setQuietDurationMs
    void setShakeNeedCnt(uint8_t cnt);     // 不再使用，保留兼容
    
private:
    // I2C 地址
    static const uint8_t I2C_ADDR = 0x18;
    static const uint8_t REG_CTRL1 = 0x20;
    static const uint8_t REG_WHO_AM_I = 0x0F;
    static const uint8_t REG_OUT_X_L = 0x02;
    static const uint8_t REG_OUT_Y_L = 0x04;
    static const uint8_t REG_OUT_Z_L = 0x06;
    
    // I2C 引脚
    static const uint8_t SDA_PIN = 6;
    static const uint8_t SCL_PIN = 7;
    
    // 读取失败处理
    static const uint8_t MAX_READ_FAILS = 20;
    
    // 最小读取间隔（ms）
    static const uint32_t MIN_READ_INTERVAL_MS = 20;
    
    // 初始化状态
    bool _initialized;
    bool _disabled;
    uint8_t _readFailCount;
    
    // 上一次读数
    int16_t _lastX, _lastY, _lastZ;
    int16_t _lastValidX, _lastValidY, _lastValidZ;
    
    // 上次读取时间戳，用于限制最小读取间隔
    uint32_t _lastReadTs;
    
    // 摇晃参数
    int _shakeThreshold;           // 摇晃阈值
    uint32_t _shakeMinDurationMs;  // 最小摇晃时长
    uint32_t _quietDurationMs;     // 静止时长
    uint32_t _shakeCooldownMs;     // 冷却时间
    uint32_t _shakeCoolTimer;      // 冷却计时器
    
    // 摇晃状态机
    enum ShakeState {
        SHAKE_IDLE,
        SHAKE_SHAKING,
        SHAKE_WAIT_QUIET,
    };
    ShakeState _shakeState;
    uint32_t _shakeStartMs;        // 摇晃开始时间
    uint32_t _quietStartMs;        // 静止开始时间
    bool _shakeEnable;
    
    // 读取单个轴（单次读取）
    int16_t readAxisOnce(uint8_t reg);
    
    // 读取原始加速度值（3 次取中位数）
    void readRawMedian(int16_t &x, int16_t &y, int16_t &z);
};

#endif // SC7A20_H
