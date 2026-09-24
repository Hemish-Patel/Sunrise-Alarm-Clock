#include <Arduino.h>
#include "TimeManager.h"
#include "InputManager.h"

TimeManager timeManager;
InputManager inputManager;

uint32_t lastPrintMillis = 0;
int tempnum = 0; // Global variable so it accumulates across loop cycles
int tempnum2 = 0; // Global variable so it accumulates across loop cycles

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n==========================================");
    Serial.println("  Sunrise Alarm Clock - Time & Input Test ");
    Serial.println("==========================================");

    // Initialize TimeManager on hardware I2C pins (GPIO 21 SDA, GPIO 22 SCL)
    if (!timeManager.init(21, 22)) {
        Serial.println(" [ERROR] Failed to initialize TimeManager / DS3231! Check wiring.");
        while (1) {
            delay(1000);
        }
    }
    Serial.println(" [+] TimeManager initialized successfully!");

    // Initialize InputManager (attaches hardware interrupts to GPIO 25, 32, 33)
    inputManager.init();
    Serial.println(" [+] InputManager initialized successfully!");
    Serial.println("------------------------------------------\n");
}

void loop() {
    // Non-blocking update (polls RTC efficiently in the background)
    timeManager.update();

    // Print cleanly every 1 second
    uint32_t currentMillis = millis();
    if (currentMillis - lastPrintMillis >= 1000) {
        lastPrintMillis = currentMillis;

        Serial.printf("[TimeManager] Date: %s | Time: %s (%02d:%02d %s) | Temp: %.1f °C\n",
                      timeManager.getFormattedDate(),
                      timeManager.getFormattedTime(),
                      timeManager.getHour12(),
                      timeManager.getMinute(),
                      timeManager.isPM() ? "PM" : "AM",
                      timeManager.getTemperature());
    }

    // Poll input manager for button debouncing and encoder ticks
    inputManager.update();

    // Read accumulated rotation delta
    int delta = inputManager.getEncoder1Delta();
    if (delta != 0) {
        tempnum += delta; // Add or subtract whatever delta was turned!
        Serial.printf(" >>> Temp num: %d (Delta: %+d)\n", tempnum, delta);
    }

    // Check if knob was pressed
    if (inputManager.isButton1Clicked()) {
        Serial.println(" *** [BUTTON CLICKED] Knob Pressed! ***");
    }
    // Read accumulated rotation delta
    int delta2 = inputManager.getEncoder2Delta();
    if (delta2 != 0) {
        tempnum2 += delta2; // Add or subtract whatever delta was turned!
        Serial.printf(" >>> Temp num2: %d (Delta: %+d)\n", tempnum2, delta2);
    }

    // Check if knob was pressed
    if (inputManager.isButton2Clicked()) {
        Serial.println(" *** [BUTTON2 CLICKED] Knob Pressed! ***");
    }
}
