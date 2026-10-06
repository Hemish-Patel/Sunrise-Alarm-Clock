/**
 * ============================================================================
 * @file DisplayManager.cpp
 * @brief Implementation of the DisplayManager class for HT16K33 7-Segment.
 * ============================================================================
 */

#include "DisplayManager.h"

DisplayManager::DisplayManager()
    : _i2cAddress(0x70),
      _brightness(10), // default comfortable medium-high brightness
      _initialized(false) {
}

bool DisplayManager::init(uint8_t address) {
    _i2cAddress = address;

    // Check if device responds on I2C bus
    Wire.beginTransmission(_i2cAddress);
    byte error = Wire.endTransmission();
    if (error != 0) {
        _initialized = false;
        return false;
    }

    _matrix.begin(_i2cAddress);
    _matrix.setBrightness(_brightness);
    _matrix.clear();
    _matrix.writeDisplay();
    _initialized = true;
    return true;
}

void DisplayManager::showTime(uint8_t hour, uint8_t minute, bool colon, bool leadingZero) {
    if (!_initialized) return;

    // Digit 0: Tens of hours
    if (hour < 10 && !leadingZero) {
        _matrix.writeDigitRaw(0, 0x00); // Blank leading zero
    } else {
        _matrix.writeDigitNum(0, (hour / 10) % 10);
    }

    // Digit 1: Ones of hours
    _matrix.writeDigitNum(1, hour % 10);

    // Digit 2: Center colon on Adafruit 7-segment backpack
    _matrix.drawColon(colon);

    // Digit 3: Tens of minutes
    _matrix.writeDigitNum(3, (minute / 10) % 10);

    // Digit 4: Ones of minutes
    _matrix.writeDigitNum(4, minute % 10);

    _matrix.writeDisplay();
}

void DisplayManager::showNumber(int number, bool colon) {
    if (!_initialized) return;
    _matrix.print(number, DEC);
    _matrix.drawColon(colon);
    _matrix.writeDisplay();
}

void DisplayManager::setBrightness(uint8_t level) {
    if (level > 15) level = 15;
    _brightness = level;
    if (_initialized) {
        _matrix.setBrightness(_brightness);
        _matrix.writeDisplay();
    }
}

void DisplayManager::clear() {
    if (!_initialized) return;
    _matrix.clear();
    _matrix.writeDisplay();
}

