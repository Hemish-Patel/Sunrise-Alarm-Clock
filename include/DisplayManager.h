#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>

/**
 * ============================================================================
 * @file DisplayManager.h
 * @brief Subsystem Manager for Adafruit 1.2" 7-Segment HT16K33 Display.
 * 
 * Part of the ESP32 Sunrise Alarm Clock Project.
 * 
 * Features:
 *  - Communicates over shared I2C bus (Default address: 0x70).
 *  - Formats time (12-hour or 24-hour) with blinking colon support.
 *  - Blanking of leading zero for clean aesthetic (e.g. " 9:30" instead of "09:30").
 *  - Brightness control (levels 0 to 15).
 * ============================================================================
 */
class DisplayManager {
public:
    DisplayManager();

    /**
     * @brief Initialize the HT16K33 display at the specified I2C address.
     * @param address I2C address (Default: 0x70).
     * @return true if display acknowledges on I2C bus, false otherwise.
     */
    bool init(uint8_t address = 0x70);

    /**
     * @brief Display time in HH:MM format with colon control.
     * @param hour Hours (0 - 23 or 1 - 12).
     * @param minute Minutes (0 - 59).
     * @param colon Whether the center colon is turned ON.
     * @param leadingZero If true, prints "09:30"; if false, prints " 9:30".
     */
    void showTime(uint8_t hour, uint8_t minute, bool colon = true, bool leadingZero = false);

    /**
     * @brief Display an integer number (-999 to 9999).
     * @param number The number to display.
     * @param colon Whether colon is lit.
     */
    void showNumber(int number, bool colon = false);

    /**
     * @brief Set display brightness level.
     * @param level Brightness level from 0 (dimmest) to 15 (brightest).
     */
    void setBrightness(uint8_t level);

    /**
     * @brief Get current brightness level (0 - 15).
     */
    uint8_t getBrightness() const { return _brightness; }

    /**
     * @brief Clear all segments on the display.
     */
    void clear();

    /**
     * @brief Check if display was found and initialized on I2C.
     */
    bool isConnected() const { return _initialized; }

private:
    Adafruit_7segment _matrix;
    uint8_t _i2cAddress;
    uint8_t _brightness;
    bool _initialized;
};

