/**
 * ============================================================================
 * @file TimeManager.cpp
 * @brief Implementation of the TimeManager class for the Sunrise Alarm Clock.
 * ============================================================================
 */

#include "TimeManager.h"

/**
 * @brief Constructor
 * Initializes internal state and zeros out string buffers.
 */
TimeManager::TimeManager()
    : _currentTime(2026, 1, 1, 0, 0, 0),
      _lastPollMillis(0) {
    memset(_timeStrBuffer, 0, sizeof(_timeStrBuffer));
    memset(_dateStrBuffer, 0, sizeof(_dateStrBuffer));
    memset(_shortTimeBuffer, 0, sizeof(_shortTimeBuffer));
}

/**
 * @brief Initializes the hardware I2C peripheral and verifies DS3231 communication.
 * If the RTC lost power (e.g. no coin cell battery), it checks ESP32 Flash
 * memory first to restore the last user-configured time, falling back to compile time.
 */
bool TimeManager::init(uint8_t sdaPin, uint8_t sclPin) {
    // Start ESP32 hardware I2C bus on the designated pins (GPIO 21 & GPIO 22)
    Wire.begin(sdaPin, sclPin);

    // Attempt communication with the DS3231 at I2C address 0x68
    if (!_rtc.begin(&Wire)) {
        return false; // Hardware not found or disconnected
    }

    // Check the Oscillator Stop Flag (OSF) in the DS3231 Status Register.
    // If the battery died or this is the very first power-on, the chip raises this flag.
    if (_rtc.lostPower()) {
        // Query ESP32 Non-Volatile Storage (Flash) for the last saved user timestamp
        _prefs.begin("clock_time", true); // Open in read-only mode
        uint32_t savedEpoch = _prefs.getUInt("last_set_epoch", 0);
        _prefs.end();

        if (savedEpoch > 0) {
            // Restore from the last saved user-configured time in Flash!
            DateTime restoredTime(savedEpoch);
            _rtc.adjust(restoredTime);
        } else {
            // First time ever booted (no Flash record): default to compile timestamp
            _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
    }

    // Capture initial time snapshot and record timestamp
    _currentTime = _rtc.now();
    _lastPollMillis = millis();
    return true;
}

/**
 * @brief Non-blocking polling method called inside loop().
 * Reads the physical RTC once every 250ms (4 times per second).
 * This ensures the seconds tick accurately while keeping the I2C bus 99% idle.
 */
void TimeManager::update() {
    uint32_t currentMillis = millis();
    
    // Check if 250 milliseconds have elapsed since our last physical read
    if (currentMillis - _lastPollMillis >= 250) {
        _lastPollMillis = currentMillis;
        _currentTime = _rtc.now(); // Read fresh time snapshot from DS3231 registers
    }
}

/**
 * @brief Reads the DS3231 internal temperature sensor.
 */
float TimeManager::getTemperature() {
    return _rtc.getTemperature();
}

/**
 * @brief Generates a standard formatted 24-hour time string: "HH:MM:SS"
 */
const char* TimeManager::getFormattedTime() {
    snprintf(_timeStrBuffer, sizeof(_timeStrBuffer), "%02d:%02d:%02d",
             _currentTime.hour(), _currentTime.minute(), _currentTime.second());
    return _timeStrBuffer;
}

/**
 * @brief Generates an ISO-8601 formatted date string: "YYYY-MM-DD"
 */
const char* TimeManager::getFormattedDate() {
    snprintf(_dateStrBuffer, sizeof(_dateStrBuffer), "%04d-%02d-%02d",
             _currentTime.year(), _currentTime.month(), _currentTime.day());
    return _dateStrBuffer;
}

/**
 * @brief Generates a short 4-character time string: "HH:MM"
 * Formatted specifically for 4-digit 7-segment LED displays.
 */
const char* TimeManager::getShortTime() {
    snprintf(_shortTimeBuffer, sizeof(_shortTimeBuffer), "%02d:%02d",
             _currentTime.hour(), _currentTime.minute());
    return _shortTimeBuffer;
}

/**
 * @brief Increments or decrements the current hour when the user rotates the time knob.
 * Uses modulo-24 arithmetic to wrap smoothly between 23 <-> 0.
 */
void TimeManager::adjustHours(int delta) {
    int newHour = (int)_currentTime.hour() + delta;
    while (newHour < 0) newHour += 24; // Handle negative wrapping (e.g. 0 - 1 = 23)
    newHour %= 24;

    setTime(_currentTime.year(), _currentTime.month(), _currentTime.day(),
            (uint8_t)newHour, _currentTime.minute(), _currentTime.second());
}

/**
 * @brief Increments or decrements the current minute when the user rotates the time knob.
 * Uses modulo-60 arithmetic to wrap smoothly between 59 <-> 0.
 */
void TimeManager::adjustMinutes(int delta) {
    int newMinute = (int)_currentTime.minute() + delta;
    while (newMinute < 0) newMinute += 60; // Handle negative wrapping (e.g. 0 - 1 = 59)
    newMinute %= 60;

    setTime(_currentTime.year(), _currentTime.month(), _currentTime.day(),
            _currentTime.hour(), (uint8_t)newMinute, _currentTime.second());
}

/**
 * @brief Writes new time values directly into the DS3231 hardware registers
 * and persists the timestamp into ESP32 Flash memory (Preferences).
 */
void TimeManager::setTime(uint16_t year, uint8_t month, uint8_t day,
                          uint8_t hour, uint8_t minute, uint8_t second) {
    DateTime newTime(year, month, day, hour, minute, second);
    _rtc.adjust(newTime);   // Transmit updated bytes over I2C to DS3231 registers
    _currentTime = newTime; // Immediately update local cache
    saveTimeToFlash();      // Persist to ESP32 Flash memory!
}

/**
 * @brief Saves the current Unix epoch timestamp into ESP32 Non-Volatile Flash.
 */
void TimeManager::saveTimeToFlash() {
    _prefs.begin("clock_time", false); // Open in read-write mode
    _prefs.putUInt("last_set_epoch", _currentTime.unixtime());
    _prefs.end();
}
