#pragma once

#include <Arduino.h>

/**
 * ============================================================================
 * @file InputManager.h
 * @brief Subsystem Manager for Rotary Encoders and Toggle Switches.
 * 
 * Part of the ESP32 Sunrise Alarm Clock Project.
 * 
 * Features:
 *  - Hardware-interrupt-driven quadrature encoder decoding (zero missed steps).
 *  - Software debouncing for encoder pushbuttons.
 *  - Event-driven consumption (returns delta and resets for state machines).
 * ============================================================================
 */
class InputManager {
public:
    InputManager();

    /**
     * @brief Configure GPIOs, internal pull-ups, and attach hardware interrupts.
     */
    void init();

    /**
     * @brief Periodic polling method for button debouncing. Call regularly in loop().
     */
    void update();

    /**
     * @brief Flushes any accumulated rotation ticks and button events on all encoders.
     * Call when transitioning between state machine states to prevent input bleeding.
     */
    void clearDeltas();

    // ========================================================================
    // ROTARY ENCODER 1 (Time Knob: GPIO 25 CLK, GPIO 32 DT, GPIO 33 SW)
    // ========================================================================

    /**
     * @brief Get the accumulated rotation step delta for Encoder 1 since last read.
     * @return Positive for Clockwise (+1, +2...), Negative for Counter-Clockwise (-1, -2...), or 0.
     */
    int getEncoder1Delta();

    /**
     * @brief Checks if Encoder 1 button was clicked (pressed and released).
     * @return true once per click event.
     */
    bool isButton1Clicked();

    /**
     * @brief Checks if Encoder 1 button is currently being held down.
     */
    bool isButton1Down() const { return _btn1Pressed; }

    // ========================================================================
    // ROTARY ENCODER 2 (Menu Knob: GPIO 4 CLK, GPIO 13 DT, GPIO 23 SW)
    // ========================================================================

    /**
     * @brief Get the accumulated rotation step delta for Encoder 2 since last read.
     * @return Positive for Clockwise (+1, +2...), Negative for Counter-Clockwise (-1, -2...), or 0.
     */
    int getEncoder2Delta();

    /**
     * @brief Checks if Encoder 2 button was clicked (pressed and released).
     * @return true once per click event.
     */
    bool isButton2Clicked();

    /**
     * @brief Checks if Encoder 2 button is currently being held down.
     */
    bool isButton2Down() const { return _btn2Pressed; }

private:
    // Static Interrupt Service Routines (ISRs stored in IRAM on ESP32)
    static void IRAM_ATTR isrEncoder1();
    static void IRAM_ATTR isrEncoder2();

    // Volatile state variables accessed inside ISRs
    static volatile int _encoder1Ticks;
    static volatile uint8_t _lastClk1;

    static volatile int _encoder2Ticks;
    static volatile uint8_t _lastClk2;

    // Local tracking for consumed deltas
    int _lastReadTicks1;
    int _lastReadTicks2;

    // Pushbutton debouncing state (Encoder 1)
    bool _btn1Pressed;
    bool _btn1LastReading;
    uint32_t _btn1LastDebounceTime;
    bool _btn1ClickEvent;

    // Pushbutton debouncing state (Encoder 2)
    bool _btn2Pressed;
    bool _btn2LastReading;
    uint32_t _btn2LastDebounceTime;
    bool _btn2ClickEvent;
};
