/**
 * ============================================================================
 * @file AlarmManager.cpp
 * @brief Implementation of the AlarmManager class for Bell Motor, DFPlayer Audio,
 *        and Flash-Persisted Alarm Configuration.
 * ============================================================================
 */

#include "AlarmManager.h"

AlarmManager::AlarmManager()
    : _bellPin(27),
      _bellActive(false),
      _burstState(false),
      _lastCadenceMillis(0),
      _dutyPercent(30),
      _audioConnected(false),
      _audioActive(false),
      _loopAudio(false),
      _currentTrack(1),
      _volume(15),       // Safe default volume for USB power
      _alarmHour(7),     // Default: 07:00 AM
      _alarmMinute(0) {
    memset(_alarmTimeBuffer, 0, sizeof(_alarmTimeBuffer));
}

void AlarmManager::init(uint8_t bellPin) {
    _bellPin = bellPin;
    pinMode(_bellPin, OUTPUT);
    analogWrite(_bellPin, 0); // Ensure MOSFET Gate is grounded (OFF) on boot
    _bellActive = false;
    _burstState = false;

    // Load saved alarm time from ESP32 Flash memory (Preferences / NVS)
    _prefs.begin("alarm_cfg", false);
    _alarmHour = _prefs.getUChar("alarm_h", 7);   // Default: 07:00 AM
    _alarmMinute = _prefs.getUChar("alarm_m", 0);
    _prefs.end();
}

void AlarmManager::setAlarmTime(uint8_t hour, uint8_t minute) {
    _alarmHour = hour % 24;
    _alarmMinute = minute % 60;

    // Persist new alarm time to Flash memory
    _prefs.begin("alarm_cfg", false);
    _prefs.putUChar("alarm_h", _alarmHour);
    _prefs.putUChar("alarm_m", _alarmMinute);
    _prefs.end();
}

const char* AlarmManager::getFormattedAlarmTime(bool format12Hour) {
    if (format12Hour) {
        uint8_t h12 = _alarmHour % 12;
        if (h12 == 0) h12 = 12;
        snprintf(_alarmTimeBuffer, sizeof(_alarmTimeBuffer), "%02d:%02d %s",
                 h12, _alarmMinute, (_alarmHour >= 12) ? "PM" : "AM");
    } else {
        snprintf(_alarmTimeBuffer, sizeof(_alarmTimeBuffer), "%02d:%02d",
                 _alarmHour, _alarmMinute);
    }
    return _alarmTimeBuffer;
}

void AlarmManager::getSunriseStartTime(uint16_t durationMinutes, uint8_t &sunriseHour, uint8_t &sunriseMinute) const {
    int alarmTotalMins = _alarmHour * 60 + _alarmMinute;
    // Calculate start time, with wrap-around across midnight
    int sunriseTotalMins = (alarmTotalMins - (int)durationMinutes + 1440) % 1440;
    sunriseHour = (uint8_t)(sunriseTotalMins / 60);
    sunriseMinute = (uint8_t)(sunriseTotalMins % 60);
}

int AlarmManager::getMinutesUntilAlarm(uint8_t currentHour, uint8_t currentMinute) const {
    int currentMins = currentHour * 60 + currentMinute;
    int alarmMins = _alarmHour * 60 + _alarmMinute;
    return (alarmMins - currentMins + 1440) % 1440;
}

uint8_t AlarmManager::calculateSunriseBrightness(float t) const {
    // ------------------------------------------------------------------------
    // CUSTOM SUNRISE LIGHT CURVE:
    // t: minutes elapsed into the sunrise sequence (0 <= t <= SUNRISE_DURATION_MINS)
    // Formula: starts at ~0.5% and curves exponentially up to 100% at t = 30
    // ------------------------------------------------------------------------
    float raw = 0.5f * exp(0.176f * t);

    // Automatic clamping to configured bounds
    if (raw < SUNRISE_MIN_BRIGHTNESS) raw = SUNRISE_MIN_BRIGHTNESS;
    if (raw > SUNRISE_MAX_BRIGHTNESS) raw = SUNRISE_MAX_BRIGHTNESS;

    return (uint8_t)round(raw);
}

uint8_t AlarmManager::calculateAudioVolume(float t) const {
    // ------------------------------------------------------------------------
    // CUSTOM AUDIO VOLUME CURVE:
    // t: minutes elapsed into audio window (0 <= t <= AUDIO_RAMP_MINS)
    // Formula: linear rise of ~2.6 steps/min (at t=0 -> 2, at t=5 -> 15)
    // ------------------------------------------------------------------------
    float raw = 2.0f + 2.6f * t;

    // Automatic clamping to configured bounds
    if (raw < AUDIO_MIN_VOLUME) raw = AUDIO_MIN_VOLUME;
    if (raw > AUDIO_MAX_VOLUME) raw = AUDIO_MAX_VOLUME;

    return (uint8_t)round(raw);
}

bool AlarmManager::initAudio(uint8_t rxPin, uint8_t txPin) {
    // Start HardwareSerial2 on designated pins (GPIO 16 RX2, GPIO 17 TX2)
    Serial2.begin(9600, SERIAL_8N1, rxPin, txPin);
    delay(500); // Give DFPlayer power-on boot time

    // Attempt connection with ACK and Reset
    if (!_dfplayer.begin(Serial2, /*isACK = */true, /*doReset = */true)) {
        // If initial handshake with Reset failed (e.g. no SD card inserted), try without ACK
        if (!_dfplayer.begin(Serial2, /*isACK = */false, /*doReset = */false)) {
            _audioConnected = false;
            return false;
        }
    }

    _audioConnected = true;
    _dfplayer.volume(_volume);
    return true;
}

void AlarmManager::setDutyPercent(uint8_t percent) {
    if (percent > 100) percent = 100;
    _dutyPercent = percent;

    // If currently actively in a burst, update the live output
    if (_bellActive && _burstState) {
        uint8_t pwmValue = (uint8_t)((_dutyPercent * 255) / 100);
        analogWrite(_bellPin, pwmValue);
    }
}

void AlarmManager::startBell() {
    if (_dutyPercent == 0) {
        _dutyPercent = 5;
    }

    _bellActive = true;
    _burstState = true;
    _lastCadenceMillis = millis();

    // Calculate 8-bit duty cycle (e.g. 30% of 255 = ~76)
    uint8_t pwmValue = (uint8_t)((_dutyPercent * 255) / 100);
    analogWrite(_bellPin, pwmValue);
}

void AlarmManager::stopBell() {
    _bellActive = false;
    _burstState = false;
    analogWrite(_bellPin, 0); // Force MOSFET fully off
}

void AlarmManager::toggleBell() {
    if (_bellActive) {
        stopBell();
    } else {
        startBell();
    }
}

void AlarmManager::playTrack(uint16_t trackNumber, bool loop) {
    if (!_audioConnected) return;
    _audioActive = true;
    _loopAudio = loop;
    _currentTrack = trackNumber;
    _dfplayer.playMp3Folder(trackNumber);
    if (loop) {
        _dfplayer.enableLoop();
    } else {
        _dfplayer.disableLoop();
    }
}

void AlarmManager::stopAudio() {
    if (!_audioConnected) return;
    _audioActive = false;
    _loopAudio = false;
    _dfplayer.disableLoop();
    _dfplayer.stop();
}

void AlarmManager::setVolume(uint8_t volume) {
    if (volume > 30) volume = 30;
    _volume = volume;
    if (_audioConnected) {
        _dfplayer.volume(_volume);
    }
}

void AlarmManager::update() {
    // ------------------------------------------------------------------------
    // 1. Bell Motor Cadence Timing
    // ------------------------------------------------------------------------
    if (_bellActive) {
        uint32_t currentMillis = millis();

        if (_burstState) {
            // In ringing burst: check if burst duration has elapsed
            if (currentMillis - _lastCadenceMillis >= BURST_DURATION_MS) {
                _burstState = false; // Transition to pause
                _lastCadenceMillis = currentMillis;
                analogWrite(_bellPin, 0); // Turn off MOSFET for pause
            }
        } else {
            // In rest pause: check if pause duration has elapsed
            if (currentMillis - _lastCadenceMillis >= PAUSE_DURATION_MS) {
                _burstState = true; // Transition back to ringing burst
                _lastCadenceMillis = currentMillis;
                uint8_t pwmValue = (uint8_t)((_dutyPercent * 255) / 100);
                analogWrite(_bellPin, pwmValue); // Fire burst
            }
        }
    }

    // ------------------------------------------------------------------------
    // 2. DFPlayer UART Message Processing (Non-blocking status & diagnostics)
    // ------------------------------------------------------------------------
    if (_audioConnected && _dfplayer.available()) {
        uint8_t type = _dfplayer.readType();
        int value = _dfplayer.read();

        switch (type) {
            case TimeOut:
                Serial.println(F(" [DFPlayer] Serial Timeout"));
                break;
            case WrongStack:
                Serial.println(F(" [DFPlayer] Stack Wrong"));
                break;
            case DFPlayerCardInserted:
                Serial.println(F(" [DFPlayer] >>> MicroSD Card Inserted! <<<"));
                break;
            case DFPlayerCardRemoved:
                Serial.println(F(" [DFPlayer] >>> MicroSD Card Removed! <<<"));
                _audioActive = false;
                break;
            case DFPlayerCardOnline:
                Serial.println(F(" [DFPlayer] >>> MicroSD Card Online & Ready! <<<"));
                break;
            case DFPlayerPlayFinished:
                Serial.printf(" [DFPlayer] Track #%d playback finished.\n", value);
                if (_audioActive && _loopAudio) {
                    Serial.printf(" [DFPlayer] Seamlessly looping Track #%d...\n", _currentTrack);
                    _dfplayer.playMp3Folder(_currentTrack);
                } else {
                    _audioActive = false;
                }
                break;
            case DFPlayerError:
                Serial.print(F(" [DFPlayer Status] "));
                switch (value) {
                    case Busy:
                        Serial.println(F("Card not found / Module Busy"));
                        break;
                    case Sleeping:
                        Serial.println(F("Sleeping"));
                        break;
                    case SerialWrongStack:
                        Serial.println(F("Serial Wrong Stack"));
                        break;
                    case CheckSumNotMatch:
                        Serial.println(F("Checksum Mismatch"));
                        break;
                    case FileIndexOut:
                        Serial.println(F("File Index Out of Bounds"));
                        break;
                    case FileMismatch:
                        Serial.println(F("File Mismatch / Not Found"));
                        break;
                    case Advertise:
                        Serial.println(F("In Advertise"));
                        break;
                    default:
                        Serial.printf("Status Code: %d\n", value);
                        break;
                }
                break;
            default:
                break;
        }
    }
}
