/**
 * ============================================================================
 * @file AlarmManager.cpp
 * @brief Implementation of the AlarmManager class for Bell Motor PWM control.
 * ============================================================================
 */

#include "AlarmManager.h"

AlarmManager::AlarmManager()
    : _bellPin(27),
      _bellActive(false),
      _burstState(false),
      _lastCadenceMillis(0),
      _dutyPercent(30) {
}

void AlarmManager::init(uint8_t bellPin) {
    _bellPin = bellPin;
    pinMode(_bellPin, OUTPUT);
    analogWrite(_bellPin, 0); // Ensure MOSFET Gate is grounded (OFF) on boot
    _bellActive = false;
    _burstState = false;
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
    // If duty was previously dialed down to 0, restore default active level (30%)
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

void AlarmManager::update() {
    if (!_bellActive) {
        return;
    }

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
