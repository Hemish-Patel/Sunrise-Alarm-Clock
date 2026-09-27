#include <Arduino.h>
#include "TimeManager.h"
#include "InputManager.h"
#include "AlarmManager.h"

TimeManager timeManager;
InputManager inputManager;
AlarmManager alarmManager;

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

    Serial.println("------------------------------------------");
    Serial.println(" [Controls]:");
    Serial.println("  - Dial DOWN to 0%: Turns OFF completely");
    Serial.println("  - When OFF, Dial UP: Turns ON at minimum 5%");
    Serial.println("  - Click Knob 1: Toggle ON (30%) / OFF");
    Serial.println("  - Knob 2: General Temp Counter 2");
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
    // Rotary Encoder 2: Menu / General Counter Test
    // ------------------------------------------------------------------------
    int delta2 = inputManager.getEncoder2Delta();
    if (delta2 != 0) {
        tempnum2 += delta2;
        Serial.printf(" >>> Temp num2: %d (Delta: %+d)\n", tempnum2, delta2);
    }

    if (inputManager.isButton2Clicked()) {
        Serial.println(" *** [BUTTON 2 CLICKED] Knob 2 Pressed! ***");
    }

    // Periodic time heartbeat every 2 seconds
    uint32_t currentMillis = millis();
    if (currentMillis - lastPrintMillis >= 2000) {
        lastPrintMillis = currentMillis;

        Serial.printf("[RTC LIVE] %s | Time: %s (%02d:%02d %s) | Bell: %s\n",
                      timeManager.getFormattedDate(),
                      timeManager.getFormattedTime(),
                      timeManager.getHour12(),
                      timeManager.getMinute(),
                      timeManager.isPM() ? "PM" : "AM",
                      alarmManager.isBellActive() ? "ACTIVE" : "OFF");
    }
}
