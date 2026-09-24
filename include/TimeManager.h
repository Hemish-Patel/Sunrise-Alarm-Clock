#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <Preferences.h>

/**
 * ============================================================================
 * @file TimeManager.h
 * @brief Subsystem Manager for the DS3231 High-Precision Real-Time Clock (RTC).
 * 
 * Part of the ESP32 Sunrise Alarm Clock Project.
 * 
 * Responsibilities:
 *  1. Hardware abstraction: Interfacing with the DS3231 over I2C (GPIO 21 SDA, GPIO 22 SCL).
 *  2. Non-blocking time caching: Throttles physical I2C reads to prevent bus congestion.
 *  3. Human-readable formatting: Provides formatted strings for the Serial Monitor & LED Display.
 *  4. Flash Memory Fallback: Saves the last configured time into ESP32 NVS (Preferences)
 *     so that even if the clock loses all power without a backup battery, it falls back
 *     to the last configured time rather than an ancient compilation date.
 *  5. Time calibration: Adjusts the live clock when the user turns the time-setting knob.
 * ============================================================================
 */
class TimeManager {
public:
    /**
     * @brief Construct a new Time Manager object.
     * Initializes internal string buffers and sets default state.
     */
    TimeManager();

    /**
     * @brief Initialize the hardware I2C bus and communicate with the DS3231 RTC.
     * 
     * @param sdaPin GPIO pin number for I2C Data line (Default: GPIO 21).
     * @param sclPin GPIO pin number for I2C Clock line (Default: GPIO 22).
     * @return true if DS3231 is detected and responding on address 0x68.
     * @return false if the RTC module is not detected (wiring fault or power missing).
     */
    bool init(uint8_t sdaPin = 21, uint8_t sclPin = 22);

    /**
     * @brief Non-blocking state update method. Must be called regularly in loop().
     * Polls the physical RTC once every 250 milliseconds and caches the result
     * in _currentTime to keep the shared I2C bus free.
     */
    void update();

    // ========================================================================
    // DIRECT TIME & STATUS GETTERS
    // ========================================================================

    /** @brief Get the full cached DateTime object (contains year, month, day, hour, min, sec). */
    DateTime getCurrentTime() const { return _currentTime; }

    /** @brief Get the current hour in 24-hour format [0 - 23]. */
    uint8_t getHour24() const       { return _currentTime.hour(); }

    /** 
     * @brief Get the current hour in 12-hour format [1 - 12] for standard clock displays.
     * Converts midnight (0) and noon (12) correctly.
     */
    uint8_t getHour12() const {
        uint8_t h = _currentTime.hour() % 12;
        return (h == 0) ? 12 : h;
    }

    /** @brief Get the current minute [0 - 59]. */
    uint8_t getMinute() const       { return _currentTime.minute(); }

    /** @brief Get the current second [0 - 59]. */
    uint8_t getSecond() const       { return _currentTime.second(); }

    /** @brief Returns true if current time is 12:00 PM or later (Noon to 23:59). */
    bool isPM() const               { return _currentTime.hour() >= 12; }

    /**
     * @brief Read the DS3231's internal temperature-compensated crystal sensor.
     * @return Ambient temperature in degrees Celsius (°C) with 0.25°C resolution.
     */
    float getTemperature();

    // ========================================================================
    // PRE-FORMATTED STRING GETTERS
    // ========================================================================

    /** @brief Returns time formatted as "HH:MM:SS" (e.g. "18:30:45"). */
    const char* getFormattedTime();

    /** @brief Returns date formatted as "YYYY-MM-DD" (e.g. "2026-09-24"). */
    const char* getFormattedDate();

    /** @brief Returns short time formatted as "HH:MM" (e.g. "18:30") for 4-digit displays. */
    const char* getShortTime();

    // ========================================================================
    // TIME ADJUSTMENT & FLASH PERSISTENCE
    // ========================================================================

    /**
     * @brief Increments or decrements the current hour by a delta, wrapping 0-23.
     * Automatically saves updated time to both RTC registers and ESP32 Flash.
     * @param delta Signed integer (+1 for clockwise turn, -1 for counter-clockwise).
     */
    void adjustHours(int delta);

    /**
     * @brief Increments or decrements the current minute by a delta, wrapping 0-59.
     * Automatically saves updated time to both RTC registers and ESP32 Flash.
     * @param delta Signed integer (+1 for clockwise turn, -1 for counter-clockwise).
     */
    void adjustMinutes(int delta);

    /**
     * @brief Sets the RTC clock to an explicit date and time.
     * Writes to both the DS3231 silicon registers and non-volatile ESP32 Flash.
     */
    void setTime(uint16_t year, uint8_t month, uint8_t day, 
                 uint8_t hour, uint8_t minute, uint8_t second = 0);

    /**
     * @brief Persists the current time snapshot into ESP32 Flash (NVS Preferences).
     */
    void saveTimeToFlash();

private:
    RTC_DS3231 _rtc;            ///< Hardware driver object from RTClib
    DateTime _currentTime;      ///< Locally cached time snapshot
    uint32_t _lastPollMillis;   ///< Timestamp of the last physical I2C read
    Preferences _prefs;         ///< ESP32 Non-Volatile Storage (Flash) driver

    // Pre-allocated internal character buffers to prevent heap fragmentation
    char _timeStrBuffer[16];    ///< Stores "HH:MM:SS"
    char _dateStrBuffer[16];    ///< Stores "YYYY-MM-DD"
    char _shortTimeBuffer[8];   ///< Stores "HH:MM"
};
