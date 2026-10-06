#include <Arduino.h>
#include "TimeManager.h"
#include "InputManager.h"
#include "AlarmManager.h"
#include "DisplayManager.h"

TimeManager timeManager;
InputManager inputManager;
AlarmManager alarmManager;
DisplayManager displayManager;

uint32_t lastPrintMillis = 0;
int tempnum2 = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n==========================================");
    Serial.println("  Sunrise Alarm Clock - Bell Driver Test  ");
    Serial.println("==========================================");

    // 1. Initialize TimeManager (DS3231 RTC on GPIO 21 SDA, GPIO 22 SCL)
    if (!timeManager.init(21, 22)) {
        Serial.println(" [ERROR] Failed to initialize TimeManager / DS3231! Check wiring.");
        while (1) {
            delay(1000);
        }
    }
    Serial.println(" [+] TimeManager initialized successfully!");

    // 2. Initialize InputManager (Rotary Encoders on GPIO 25, 32, 33 and GPIO 4, 13, 23)
    inputManager.init();
    Serial.println(" [+] InputManager initialized successfully!");

    // 3. Initialize AlarmManager (MOSFET Gate on GPIO 27)
    alarmManager.init(27);
    Serial.println(" [+] AlarmManager initialized on GPIO 27!");
    Serial.printf(" [+] Default Bell PWM Duty Cycle: %d%% (~1.5V average from 5V)\n", alarmManager.getDutyPercent());

    // 4. Initialize DisplayManager (Adafruit 1.2" HT16K33 on shared I2C 0x70)
    if (!displayManager.init(0x70)) {
        Serial.println(" [WARNING] Adafruit 1.2\" Display not found at 0x70! Check wiring / I2C lines.");
    } else {
        Serial.println(" [+] DisplayManager initialized successfully at 0x70!");
    }

    Serial.println("------------------------------------------");
    Serial.println(" [Controls]:");
    Serial.println("  - Dial DOWN to 0%: Turns OFF completely");
    Serial.println("  - When OFF, Dial UP: Turns ON at minimum 5%");
    Serial.println("  - Click Knob 1: Toggle ON (30%) / OFF");
    Serial.println("  - Knob 2: Adjust Display Brightness (0 - 15)");
    Serial.println("  - Switches [1, 2, 3]: Arming toggles (GPIO 34, 35, 39)");
    Serial.println("------------------------------------------\n");
}

void loop() {
    // Non-blocking subsystem updates
    timeManager.update();
    inputManager.update();
    alarmManager.update();

    // ------------------------------------------------------------------------
    // Rotary Encoder 1: Smooth "Dim-to-Off" and "Dial-Up to 5%" Logic
    // ------------------------------------------------------------------------
    int delta1 = inputManager.getEncoder1Delta();
    if (delta1 != 0) {
        if (!alarmManager.isBellActive()) {
            // CURRENTLY OFF:
            // If the user rotates clockwise (dial up), turn it ON at minimum (5%)
            if (delta1 > 0) {
                alarmManager.setDutyPercent(5);
                alarmManager.startBell();
                Serial.printf(" >>> [DIALED UP] Turned ON at lowest brightness: 5%% (~%.2f V)\n", 5.0 * 0.05);
            }
        } else {
            // CURRENTLY ON:
            int currentDuty = (int)alarmManager.getDutyPercent();
            int newDuty = currentDuty + (delta1 * 5); // Step by 5%

            if (newDuty <= 0) {
                // User dialed all the way down to 0: turn OFF completely!
                alarmManager.setDutyPercent(0);
                alarmManager.stopBell();
                Serial.println(" >>> [DIALED TO 0%] Reached 0% -> Turned OFF completely!");
            } else {
                // Normal adjustment between 5% and 100%
                newDuty = constrain(newDuty, 5, 100);
                alarmManager.setDutyPercent(newDuty);
                Serial.printf(" >>> [DUTY ADJUSTED] %d%% (Effective: ~%.2f V from 5V bus)\n", 
                              newDuty, 5.0 * (newDuty / 100.0));
            }
        }
    }

    // ------------------------------------------------------------------------
    // Pushbutton 1: Toggle Bell Cadence (Manual ON/OFF toggle)
    // ------------------------------------------------------------------------
    if (inputManager.isButton1Clicked()) {
        alarmManager.toggleBell();
        Serial.printf(" *** [BUTTON 1 CLICKED] State: %s (Duty: %d%%) ***\n",
                      alarmManager.isBellActive() ? "ACTIVE (Ringing/Pulsing)" : "OFF",
                      alarmManager.getDutyPercent());
    }

    // ------------------------------------------------------------------------
    // Rotary Encoder 2: Display Brightness Control (0 - 15) & Test Mode Toggle
    // ------------------------------------------------------------------------
    static bool testModeActive = false;

    int delta2 = inputManager.getEncoder2Delta();
    if (delta2 != 0) {
        int newBright = constrain((int)displayManager.getBrightness() + delta2, 0, 15);
        displayManager.setBrightness(newBright);
        Serial.printf(" >>> Display Brightness: %d / 15 (Delta: %+d)\n", newBright, delta2);
    }

    if (inputManager.isButton2Clicked()) {
        testModeActive = !testModeActive;
        Serial.printf(" *** [BUTTON 2 CLICKED] Display Mode: %s ***\n",
                      testModeActive ? "SEGMENT TEST (88:88)" : "LIVE CLOCK");
    }

    // ------------------------------------------------------------------------
    // Switches State Change Detection (Immediate Feedback)
    // ------------------------------------------------------------------------
    static bool lastSw1 = inputManager.switchState(1);
    static bool lastSw2 = inputManager.switchState(2);
    static bool lastSw3 = inputManager.switchState(3);

    bool curSw1 = inputManager.switchState(1);
    bool curSw2 = inputManager.switchState(2);
    bool curSw3 = inputManager.switchState(3);

    if (curSw1 != lastSw1) {
        lastSw1 = curSw1;
        Serial.printf(" *** [SWITCH 1 - Light] -> %s ***\n", curSw1 ? "ON (ARMED)" : "OFF (DISARMED)");
    }
    if (curSw2 != lastSw2) {
        lastSw2 = curSw2;
        Serial.printf(" *** [SWITCH 2 - Audio] -> %s ***\n", curSw2 ? "ON (ARMED)" : "OFF (DISARMED)");
    }
    if (curSw3 != lastSw3) {
        lastSw3 = curSw3;
        Serial.printf(" *** [SWITCH 3 - Bell]  -> %s ***\n", curSw3 ? "ON (ARMED)" : "OFF (DISARMED)");
    }

    // ------------------------------------------------------------------------
    // Display Update (Non-blocking, 250ms cadence with blinking colon)
    // ------------------------------------------------------------------------
    uint32_t currentMillis = millis();
    static uint32_t lastDisplayUpdate = 0;
    if (currentMillis - lastDisplayUpdate >= 250) {
        lastDisplayUpdate = currentMillis;

        if (testModeActive) {
            displayManager.showNumber(8888, true); // All digits lit with colon
        } else {
            bool colon = (currentMillis / 500) % 2 == 0; // 1-second period blinking colon
            displayManager.showTime(timeManager.getHour12(), timeManager.getMinute(), colon, false);
        }
    }

    // Periodic time heartbeat every 2 seconds
    if (currentMillis - lastPrintMillis >= 2000) {
        lastPrintMillis = currentMillis;

        Serial.printf("[RTC LIVE] %s | Time: %s (%02d:%02d %s) | SW[1,2,3]: [%d, %d, %d] | Bell: %s\n",
                      timeManager.getFormattedDate(),
                      timeManager.getFormattedTime(),
                      timeManager.getHour12(),
                      timeManager.getMinute(),
                      timeManager.isPM() ? "PM" : "AM",
                      curSw1, curSw2, curSw3,
                      alarmManager.isBellActive() ? "ACTIVE" : "OFF");
    }
}
