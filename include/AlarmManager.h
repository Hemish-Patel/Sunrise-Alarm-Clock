#pragma once

#include <Arduino.h>

/**
 * ============================================================================
 * @file AlarmManager.h
 * @brief Subsystem Manager for Alarm Outputs (Bell Motor MOSFET & Audio).
 * 
 * Part of the ESP32 Sunrise Alarm Clock Project.
 * 
 * Features (Phase 1 - Motor Driver):
 *  - PWM Voltage Chopping (~30% duty cycle) to step down 5V bus to ~1.5V
 *    for safe operation of the mechanical twin-bell clapper motor without overheating.
 *  - Non-blocking cadence timing (e.g. 2 seconds ringing, 1 second pause).
 * ============================================================================
 */
class AlarmManager {
public:
    AlarmManager();

    /**
     * @brief Initialize the MOSFET motor driver pin.
     * @param bellPin GPIO pin connected to the MOSFET gate (Default: GPIO 27).
     */
    void init(uint8_t bellPin = 27);

    /**
     * @brief Non-blocking state update method. Must be called regularly in loop().
     * Handles cadence timing (alternating between ringing burst and rest pause).
     */
    void update();

    // ========================================================================
    // BELL MOTOR CONTROLS
    // ========================================================================

    /**
     * @brief Start the mechanical bell ringing cadence.
     */
    void startBell();

    /**
     * @brief Stop the mechanical bell immediately and ensure PWM is 0.
     */
    void stopBell();

    /**
     * @brief Toggles between startBell() and stopBell(). Convenient for button testing.
     */
    void toggleBell();

    /**
     * @brief Check if the bell alarm is currently active.
     */
    bool isBellActive() const { return _bellActive; }

    /**
     * @brief Set the PWM chopping duty cycle percentage (0 - 100%).
     * Nominal for the 1.5V-2.0V clapper motor from 5V is 30% - 35%.
     */
    void setDutyPercent(uint8_t percent);
    uint8_t getDutyPercent() const { return _dutyPercent; }

private:
    uint8_t _bellPin;
    bool _bellActive;           // Overall cadence active flag
    bool _burstState;           // Currently in ringing burst (true) vs pause (false)
    uint32_t _lastCadenceMillis;// Timestamp of last burst/pause transition
    uint8_t _dutyPercent;       // Configured duty cycle (default: 30%)

    // Cadence timing constants (in milliseconds)
    const uint32_t BURST_DURATION_MS = 2000; // 2 seconds ringing burst
    const uint32_t PAUSE_DURATION_MS = 1000; // 1 second rest pause
};

