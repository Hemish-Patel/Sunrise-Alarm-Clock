#pragma once

#include <Arduino.h>
#include <cmath>
#include <Preferences.h>
#include <DFRobotDFPlayerMini.h>

/**
 * ============================================================================
 * @file AlarmManager.h
 * @brief Subsystem Manager for Alarm Outputs (Bell Motor MOSFET & Audio),
 *        Alarm Persistence, and Customizable Sunrise/Audio Math Curves.
 * 
 * Part of the ESP32 Sunrise Alarm Clock Project.
 * ============================================================================
 */
class AlarmManager {
public:
    // ========================================================================
    // CONFIGURABLE ALARM & SUNRISE CONSTANTS (EASY TO TWEAK)
    // ========================================================================
    static const uint16_t SUNRISE_DURATION_MINS   = 30;  // Minutes before alarm that light ramp begins
    static const uint8_t  SUNRISE_MAX_BRIGHTNESS  = 100; // Target peak brightness % (Physical hardware max: 100%)
    static const uint8_t  SUNRISE_MIN_BRIGHTNESS  = 1;   // Initial lowest light glow % (1%)

    static const uint16_t AUDIO_RAMP_MINS         = 5;   // Minutes before alarm that bird audio begins
    static const uint8_t  AUDIO_MAX_VOLUME        = 15;  // Target wake volume (DFPlayer hardware max: 30, keep <= 18 on USB)
    static const uint8_t  AUDIO_MIN_VOLUME        = 2;   // Starting whisper volume (0 - 30)

    static const uint8_t  BELL_PWM_DUTY_PERCENT   = 30;  // Clapper motor PWM % (Hardware max 100%, keep 30-35% = 1.5V)
    static const uint32_t AUTO_SHUTOFF_MILLIS     = 5 * 60 * 1000UL; // 5 minutes max ringing/playback timeout

    AlarmManager();

    /**
     * @brief Initialize the MOSFET motor driver pin and restore alarm settings from Flash.
     * @param bellPin GPIO pin connected to the MOSFET gate (Default: GPIO 27).
     */
    void init(uint8_t bellPin = 27);

    /**
     * @brief Initialize HardwareSerial2 and the DFPlayer Mini module.
     * @param rxPin ESP32 RX pin connected to DFPlayer TX (Default: GPIO 16).
     * @param txPin ESP32 TX pin connected through 1k resistor to DFPlayer RX (Default: GPIO 17).
     * @return true if communication was established, false otherwise.
     */
    bool initAudio(uint8_t rxPin = 16, uint8_t txPin = 17);

    /**
     * @brief Non-blocking state update method. Must be called regularly in loop().
     * Handles bell cadence timing and processes DFPlayer UART messages.
     */
    void update();

    // ========================================================================
    // ALARM TIMING & MATH CURVES (CUSTOMIZABLE EQUATIONS)
    // ========================================================================

    /**
     * @brief Calculates how many minutes remain until the alarm time.
     * Handles midnight wraparound automatically.
     * @return 0 if at alarm minute, or 1 to 1439 minutes remaining.
     */
    int getMinutesUntilAlarm(uint8_t currentHour, uint8_t currentMinute) const;

    /**
     * @brief Single customizable equation for Sunrise Light Brightness (1 - 100%).
     * @param t Minutes elapsed into the sunrise sequence (from 0 up to SUNRISE_DURATION_MINS).
     * @return Brightness level from 1 to SUNRISE_MAX_BRIGHTNESS (clamped).
     */
    uint8_t calculateSunriseBrightness(float t) const;

    /**
     * @brief Single customizable equation for Audio Fade-in Volume (0 - 30).
     * @param t Minutes elapsed into the audio sequence (from 0 up to AUDIO_RAMP_MINS).
     * @return Volume level from AUDIO_MIN_VOLUME to AUDIO_MAX_VOLUME (clamped).
     */
    uint8_t calculateAudioVolume(float t) const;

    // ========================================================================
    // ALARM CONFIGURATION & PERSISTENCE
    // ========================================================================

    void setAlarmTime(uint8_t hour, uint8_t minute);
    uint8_t getAlarmHour() const { return _alarmHour; }
    uint8_t getAlarmMinute() const { return _alarmMinute; }

    bool isAlarmTime(uint8_t currentHour, uint8_t currentMinute) const {
        return (currentHour == _alarmHour && currentMinute == _alarmMinute);
    }

    const char* getFormattedAlarmTime(bool format12Hour = true);
    void getSunriseStartTime(uint16_t durationMinutes, uint8_t &sunriseHour, uint8_t &sunriseMinute) const;

    // ========================================================================
    // BELL MOTOR CONTROLS
    // ========================================================================

    void startBell();
    void stopBell();
    void toggleBell();
    bool isBellActive() const { return _bellActive; }

    void setDutyPercent(uint8_t percent);
    uint8_t getDutyPercent() const { return _dutyPercent; }

    // ========================================================================
    // DFPLAYER MINI AUDIO CONTROLS
    // ========================================================================

    void playTrack(uint16_t trackNumber = 1, bool loop = true);
    void stopAudio();

    void setVolume(uint8_t volume);
    uint8_t getVolume() const { return _volume; }

    bool isAudioConnected() const { return _audioConnected; }
    bool isAudioActive() const { return _audioActive; }

private:
    // Bell motor members
    uint8_t _bellPin;
    bool _bellActive;
    bool _burstState;
    uint32_t _lastCadenceMillis;
    uint8_t _dutyPercent;

    const uint32_t BURST_DURATION_MS = 2000; // 2 seconds ringing burst
    const uint32_t PAUSE_DURATION_MS = 1000; // 1 second rest pause

    // Audio members
    DFRobotDFPlayerMini _dfplayer;
    bool _audioConnected;
    bool _audioActive;
    bool _loopAudio;
    uint16_t _currentTrack;
    uint8_t _volume;

    // Alarm time configuration & persistence
    uint8_t _alarmHour;
    uint8_t _alarmMinute;
    Preferences _prefs;
    char _alarmTimeBuffer[16];
};
