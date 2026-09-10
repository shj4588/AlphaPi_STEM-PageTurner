/*
 * SC7A20.cpp - SC7A20 加速度计驱动与摇晃检测实现
 * 
 * 按照 Python 版的摇晃检测逻辑重构：
 * - 读取 3 次取中位数，过滤 SoftI2C 单次读取位错误
 * - 三轴差值之和（total = abs(dx) + abs(dy) + abs(dz)）作为摇晃幅度
 * - 3 状态机：IDLE / SHAKING / WAIT_QUIET
 * - 摇晃时长 >= 最小摇晃时长才算有效摇晃
 * - 限制最小读取间隔 20ms
 * - 静止判断用 total < shake_threshold（摇晃减弱就算静止）
 */

#include "SC7A20.h"

SC7A20::SC7A20() {
    _initialized = false;
    _disabled = false;
    _readFailCount = 0;
    
    _lastX = 0;
    _lastY = 0;
    _lastZ = 0;
    _lastValidX = 0;
    _lastValidY = 0;
    _lastValidZ = 0;
    
    _lastReadTs = 0;
    
    // 默认参数（参考 Python 版）
    _shakeThreshold = 3000;
    _shakeMinDurationMs = 100;
    _quietDurationMs = 300;
    _shakeCooldownMs = 1000;
    _shakeCoolTimer = 0;
    
    _shakeState = SHAKE_IDLE;
    _shakeStartMs = 0;
    _quietStartMs = 0;
    _shakeEnable = true;
}

int16_t SC7A20::readAxisOnce(uint8_t reg) {
    // 单次读取
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission(true);
    Wire.requestFrom((uint8_t)I2C_ADDR, (uint8_t)2);
    if (Wire.available() < 2) return 0;
    uint8_t l = Wire.read();
    uint8_t h = Wire.read();
    int16_t val = (int16_t)((h << 8) | l);
    return val;
}

void SC7A20::readRawMedian(int16_t &x, int16_t &y, int16_t &z) {
    // 读取 3 次取中位数，过滤 SoftI2C 单次读取位错误
    int16_t r1x = readAxisOnce(REG_OUT_X_L);
    int16_t r1y = readAxisOnce(REG_OUT_Y_L);
    int16_t r1z = readAxisOnce(REG_OUT_Z_L);
    
    int16_t r2x = readAxisOnce(REG_OUT_X_L);
    int16_t r2y = readAxisOnce(REG_OUT_Y_L);
    int16_t r2z = readAxisOnce(REG_OUT_Z_L);
    
    int16_t r3x = readAxisOnce(REG_OUT_X_L);
    int16_t r3y = readAxisOnce(REG_OUT_Y_L);
    int16_t r3z = readAxisOnce(REG_OUT_Z_L);
    
    // 对每个轴取中位数
    int16_t xs[3] = {r1x, r2x, r3x};
    int16_t ys[3] = {r1y, r2y, r3y};
    int16_t zs[3] = {r1z, r2z, r3z};
    
    // 简单排序取中位数
    for (int i = 0; i < 2; i++) {
        for (int j = i + 1; j < 3; j++) {
            if (xs[i] > xs[j]) { int16_t t = xs[i]; xs[i] = xs[j]; xs[j] = t; }
            if (ys[i] > ys[j]) { int16_t t = ys[i]; ys[i] = ys[j]; ys[j] = t; }
            if (zs[i] > zs[j]) { int16_t t = zs[i]; zs[i] = zs[j]; zs[j] = t; }
        }
    }
    
    x = xs[1];
    y = ys[1];
    z = zs[1];
}

bool SC7A20::begin() {
    // 初始化 I2C（使用默认时钟频率 100kHz）
    Wire.begin(SDA_PIN, SCL_PIN);
    
    // 扫描 I2C 设备
    uint8_t error, address;
    bool found = false;
    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
            if (address == I2C_ADDR) {
                found = true;
            }
        }
    }
    
    if (!found) {
        Serial.println("SC7A20: device not found");
        return false;
    }
    
    // 读取 WHO_AM_I
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(REG_WHO_AM_I);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)I2C_ADDR, (uint8_t)1);
    if (Wire.available() < 1) {
        Serial.println("SC7A20: WHO_AM_I read failed");
        return false;
    }
    uint8_t who = Wire.read();
    if (who != 0x11) {
        Serial.printf("SC7A20: unexpected WHO_AM_I (0x%02X, expected 0x11)\n", who);
        return false;
    }
    
    // 配置 SC7A20：写入寄存器 0x20，值 0x57（100Hz ±2g）
    Wire.beginTransmission(I2C_ADDR);
    Wire.write(REG_CTRL1);
    Wire.write(0x57);
    error = Wire.endTransmission();
    if (error != 0) {
        Serial.println("SC7A20: init failed");
        return false;
    }
    
    // 等待传感器启动
    delay(50);
    
    // 读取初始值，建立基线
    readRawMedian(_lastX, _lastY, _lastZ);
    if (_lastX == 0 && _lastY == 0 && _lastZ == 0) {
        // 第一次读取可能失败，再试几次
        for (int i = 0; i < 10; i++) {
            readRawMedian(_lastX, _lastY, _lastZ);
            if (_lastX != 0 || _lastY != 0 || _lastZ != 0) {
                break;
            }
            delay(10);
        }
    }
    
    _lastValidX = _lastX;
    _lastValidY = _lastY;
    _lastValidZ = _lastZ;
    
    // 初始化变量
    _shakeCoolTimer = 0;
    _shakeState = SHAKE_IDLE;
    _readFailCount = 0;
    _lastReadTs = 0;
    
    _initialized = true;
    Serial.println("SC7A20: ready (median filter + total-delta + 3-state)");
    Serial.printf("SC7A20: initial = (%d, %d, %d)\n", _lastX, _lastY, _lastZ);
    return true;
}

void SC7A20::readRaw(int16_t &x, int16_t &y, int16_t &z) {
    if (!_initialized || _disabled) {
        x = _lastValidX;
        y = _lastValidY;
        z = _lastValidZ;
        return;
    }
    
    readRawMedian(x, y, z);
    if (x == 0 && y == 0 && z == 0) {
        _readFailCount++;
        if (_readFailCount >= MAX_READ_FAILS) {
            Serial.println("SC7A20: read failed too many times, disabling");
            _disabled = true;
        }
        x = _lastValidX;
        y = _lastValidY;
        z = _lastValidZ;
        return;
    }
    _readFailCount = 0;
    _lastValidX = x;
    _lastValidY = y;
    _lastValidZ = z;
}

bool SC7A20::checkShakeEvent() {
    if (!_shakeEnable || !_initialized || _disabled) {
        return false;
    }
    
    // 冷却时间检查
    if (_shakeCoolTimer > 0 && (millis() - _shakeCoolTimer < _shakeCooldownMs)) {
        return false;
    }
    
    // 限制最小读取间隔：主循环太快，相邻两次读取差值太小
    // 保证至少 20ms 才真正读取一次，这样加速度变化量足够大
    uint32_t now = millis();
    if (_lastReadTs > 0 && (now - _lastReadTs < MIN_READ_INTERVAL_MS)) {
        return false;
    }
    _lastReadTs = now;
    
    // 读取原始值（3 次取中位数）
    int16_t x, y, z;
    readRawMedian(x, y, z);
    if (x == 0 && y == 0 && z == 0) {
        // 读取失败，不更新 last，不改变状态
        _readFailCount++;
        if (_readFailCount >= MAX_READ_FAILS) {
            Serial.println("SC7A20: read failed too many times, disabling");
            _disabled = true;
        }
        return false;
    }
    _readFailCount = 0;
    _lastValidX = x;
    _lastValidY = y;
    _lastValidZ = z;
    
    // 计算与上一次读数的差值
    int16_t dx = x - _lastX;
    int16_t dy = y - _lastY;
    int16_t dz = z - _lastZ;
    // 更新上一次读数
    _lastX = x;
    _lastY = y;
    _lastZ = z;
    
    // 三轴差值之和
    int32_t total = abs(dx) + abs(dy) + abs(dz);
    bool isShaking = total > _shakeThreshold;
    // 注意：静止判断用 total < shake_threshold（摇晃减弱就算静止），
    // 不再用 quiet_threshold，因为摇晃后设备可能有微小晃动导致
    // total 一直在 quiet_threshold 和 shake_threshold 之间，无法进入静止
    
    // ========== 状态机 ==========
    switch (_shakeState) {
        case SHAKE_IDLE:
            // 等待摇晃开始
            if (isShaking) {
                _shakeState = SHAKE_SHAKING;
                _shakeStartMs = now;
            }
            return false;
            
        case SHAKE_SHAKING:
            // 摇晃中
            if (isShaking) {
                // 还在摇晃，持续累计
                return false;
            } else {
                // 摇晃减弱（total < shake_threshold），算摇晃结束
                uint32_t shakeDuration = now - _shakeStartMs;
                if (shakeDuration >= _shakeMinDurationMs) {
                    // 有效摇晃，进入等待静止
                    _shakeState = SHAKE_WAIT_QUIET;
                    _quietStartMs = now;
                } else {
                    // 摇晃时长不够，不算有效摇晃，回到 IDLE
                    _shakeState = SHAKE_IDLE;
                }
                return false;
            }
            
        case SHAKE_WAIT_QUIET:
            // 等待静止：只计时，不因为 total 变化而回到 SHAKING
            // （避免余震导致状态来回切换，永远无法达到静止时长）
            {
                uint32_t quietDuration = now - _quietStartMs;
                if (quietDuration >= _quietDurationMs) {
                    // 静止时长达到，触发一次翻页
                    _shakeState = SHAKE_IDLE;
                    _shakeCoolTimer = now;
                    return true;
                }
            }
            return false;
            
        default:
            _shakeState = SHAKE_IDLE;
            return false;
    }
}

bool SC7A20::isReady() {
    return _initialized && !_disabled;
}

void SC7A20::setShakeSens(int sens) {
    _shakeThreshold = sens;
}

void SC7A20::setShakeMinDurationMs(uint32_t ms) {
    _shakeMinDurationMs = ms;
}

void SC7A20::setQuietDurationMs(uint32_t ms) {
    _quietDurationMs = ms;
}

void SC7A20::setShakeCooldownMs(uint32_t ms) {
    _shakeCooldownMs = ms;
}

// 兼容旧接口
void SC7A20::setQuietSens(int sens) {
    // 不再使用，保留兼容
}

void SC7A20::setShakeWinMs(uint32_t ms) {
    // 不再使用，保留兼容
}

void SC7A20::setQuietHoldMs(uint32_t ms) {
    // 映射到 setQuietDurationMs
    _quietDurationMs = ms;
}

void SC7A20::setShakeNeedCnt(uint8_t cnt) {
    // 不再使用，保留兼容
}
