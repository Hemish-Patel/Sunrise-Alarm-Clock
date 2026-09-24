/**
 * ============================================================================
 * @file InputManager.cpp
 * @brief Implementation of the InputManager class for Encoders & Switches.
 * ============================================================================
 */

#include "InputManager.h"

// ----------------------------------------------------------------------------
// Pin Assignments from Master Hardware Pinout (HardwareSpecification.md)
// ----------------------------------------------------------------------------
// Rotary Encoder 1 (Time Knob)
#define ENC1_CLK_PIN 25  // Rotary Enc 1 CLK (Interrupt)
#define ENC1_DT_PIN  32  // Rotary Enc 1 DT
#define ENC1_SW_PIN  33  // Rotary Enc 1 Pushbutton Switch

// Rotary Encoder 2 (Menu Knob)
#define ENC2_CLK_PIN 4   // Rotary Enc 2 CLK (Interrupt)
#define ENC2_DT_PIN  13  // Rotary Enc 2 DT
#define ENC2_SW_PIN  23  // Rotary Enc 2 Pushbutton Switch

#define DEBOUNCE_DELAY_MS 40 // Debounce window for tactile pushbuttons

// Static member variable definitions (Encoder 1)
volatile int InputManager::_encoder1Ticks = 0;
volatile uint8_t InputManager::_lastClk1 = HIGH;

// Static member variable definitions (Encoder 2)
volatile int InputManager::_encoder2Ticks = 0;
volatile uint8_t InputManager::_lastClk2 = HIGH;

/**
 * @brief Hardware Interrupt Service Routine for Encoder 1.
 */
void IRAM_ATTR InputManager::isrEncoder1() {
    uint8_t clk = digitalRead(ENC1_CLK_PIN);
    uint8_t dt  = digitalRead(ENC1_DT_PIN);

    if (clk != _lastClk1) {
        _lastClk1 = clk;
        if (clk == LOW) {
            if (dt == HIGH) {
                _encoder1Ticks++; // Clockwise
            } else {
                _encoder1Ticks--; // Counter-Clockwise
            }
        }
    }
}

/**
 * @brief Hardware Interrupt Service Routine for Encoder 2.
 */
void IRAM_ATTR InputManager::isrEncoder2() {
    uint8_t clk = digitalRead(ENC2_CLK_PIN);
    uint8_t dt  = digitalRead(ENC2_DT_PIN);

    if (clk != _lastClk2) {
        _lastClk2 = clk;
        if (clk == LOW) {
            if (dt == HIGH) {
                _encoder2Ticks++; // Clockwise
            } else {
                _encoder2Ticks--; // Counter-Clockwise
            }
        }
    }
}

InputManager::InputManager()
    : _lastReadTicks1(0),
      _lastReadTicks2(0),
      _btn1Pressed(false),
      _btn1LastReading(HIGH),
      _btn1LastDebounceTime(0),
      _btn1ClickEvent(false),
      _btn2Pressed(false),
      _btn2LastReading(HIGH),
      _btn2LastDebounceTime(0),
      _btn2ClickEvent(false) {
}

void InputManager::init() {
    // --- Configure Rotary Encoder 1 (Time Knob) ---
    pinMode(ENC1_CLK_PIN, INPUT);
    pinMode(ENC1_DT_PIN,  INPUT);
    pinMode(ENC1_SW_PIN,  INPUT_PULLUP);

    _lastClk1 = digitalRead(ENC1_CLK_PIN);
    attachInterrupt(digitalPinToInterrupt(ENC1_CLK_PIN), isrEncoder1, CHANGE);

    // --- Configure Rotary Encoder 2 (Menu Knob) ---
    pinMode(ENC2_CLK_PIN, INPUT);
    pinMode(ENC2_DT_PIN,  INPUT);
    pinMode(ENC2_SW_PIN,  INPUT_PULLUP);

    _lastClk2 = digitalRead(ENC2_CLK_PIN);
    attachInterrupt(digitalPinToInterrupt(ENC2_CLK_PIN), isrEncoder2, CHANGE);
}

void InputManager::update() {
    uint32_t now = millis();

    // ------------------------------------------------------------------------
    // Pushbutton 1 Debounce State Machine (Encoder 1)
    // ------------------------------------------------------------------------
    bool reading1 = (digitalRead(ENC1_SW_PIN) == LOW); // true = pressed

    if (reading1 != _btn1LastReading) {
        _btn1LastDebounceTime = now;
        _btn1LastReading = reading1;
    }

    if ((now - _btn1LastDebounceTime) > DEBOUNCE_DELAY_MS) {
        if (reading1 && !_btn1Pressed) {
            _btn1ClickEvent = true;
        }
        _btn1Pressed = reading1;
    }

    // ------------------------------------------------------------------------
    // Pushbutton 2 Debounce State Machine (Encoder 2)
    // ------------------------------------------------------------------------
    bool reading2 = (digitalRead(ENC2_SW_PIN) == LOW); // true = pressed

    if (reading2 != _btn2LastReading) {
        _btn2LastDebounceTime = now;
        _btn2LastReading = reading2;
    }

    if ((now - _btn2LastDebounceTime) > DEBOUNCE_DELAY_MS) {
        if (reading2 && !_btn2Pressed) {
            _btn2ClickEvent = true;
        }
        _btn2Pressed = reading2;
    }
}

int InputManager::getEncoder1Delta() {
    noInterrupts();
    int currentTicks = _encoder1Ticks;
    int delta = currentTicks - _lastReadTicks1;
    _lastReadTicks1 = currentTicks;
    interrupts();

    return delta;
}

bool InputManager::isButton1Clicked() {
    if (_btn1ClickEvent) {
        _btn1ClickEvent = false;
        return true;
    }
    return false;
}

int InputManager::getEncoder2Delta() {
    noInterrupts();
    int currentTicks = _encoder2Ticks;
    int delta = currentTicks - _lastReadTicks2;
    _lastReadTicks2 = currentTicks;
    interrupts();

    return delta;
}

bool InputManager::isButton2Clicked() {
    if (_btn2ClickEvent) {
        _btn2ClickEvent = false;
        return true;
    }
    return false;
}

void InputManager::clearDeltas() {
    noInterrupts();
    _lastReadTicks1 = _encoder1Ticks;
    _lastReadTicks2 = _encoder2Ticks;
    _btn1ClickEvent = false;
    _btn2ClickEvent = false;
    interrupts();
}
