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
    alarmManager.setDutyPercent(30); // Default 30% duty cycle for bell motor
    Serial.printf(" [+] Default Bell PWM Duty Cycle: %d%% (~1.5V average from 5V)\n", alarmManager.getDutyPercent());

    // 4. Initialize DisplayManager (Adafruit 1.2" HT16K33 on shared I2C 0x70)
    if (!displayManager.init(0x70)) {
        Serial.println(" [WARNING] Adafruit 1.2\" Display not found at 0x70! Check wiring / I2C lines.");
    } else {
        Serial.println(" [+] DisplayManager initialized successfully at 0x70!");
    }

    // 5. Initialize DFPlayer Mini Audio (GPIO 16 RX2, GPIO 17 TX2)
    if (!alarmManager.initAudio(16, 17)) {
        Serial.println(" [WARNING] DFPlayer Mini not responding on UART2! Check wiring (RX/TX, 1k resistor, 5V power).");
    } else {
        Serial.println(" [+] DFPlayer Mini initialized successfully on UART2 (GPIO 16/17)!");
        Serial.printf(" [+] Default Audio Volume: %d / 30 (USB Safe)\n", alarmManager.getVolume());
    }

    // 6. Display Loaded Alarm Configuration from Flash
    uint8_t sHour, sMin;
    alarmManager.getSunriseStartTime(30, sHour, sMin); // 30 mins prior
    Serial.printf(" [+] Loaded Alarm Time (from Flash): %s\n", alarmManager.getFormattedAlarmTime(true));
    Serial.printf(" [+] Sunrise Light Ramp triggers 30m prior at: %02d:%02d\n", sHour, sMin);

    Serial.println("------------------------------------------");
    Serial.println(" [Controls]:");
    Serial.println("  - Knob 1: Adjust Hours (when in set mode)");
    Serial.println("  - Click Knob 1: Toggle Bell Test");
    Serial.println("  - Knob 2: Adjust Display Brightness (0 - 15)");
    Serial.println("  - Click Knob 2: Toggle 88:88 Test / Live Clock");
    Serial.println("  - Switch 1: Light Arm (GPIO 34)");
    Serial.println("  - Switch 2: Audio Arm (GPIO 35)");
    Serial.println("  - Switch 3: Bell Arm  (GPIO 39)");
    Serial.println("------------------------------------------\n");
}




//Main loop() function: Non-blocking updates for all subsystems, rotary encoder polling, and display refresh
void loop() {
    // Live light brightness state (0 - 100%)
    static uint8_t currentLightBrightness = 0;

    // Non-blocking subsystem updates
    timeManager.update();
    inputManager.update();
    alarmManager.update();

    // ------------------------------------------------------------------------
    // Rotary Encoder 1: Manual Light Dimming (Knob 1)
    // ------------------------------------------------------------------------
    int delta1 = inputManager.getEncoder1Delta();
    if (delta1 != 0) {
        int newB = (int)currentLightBrightness + delta1 * 5; // 5% step per encoder detent
        if (newB < 0) newB = 0;
        if (newB > 100) newB = 100;
        currentLightBrightness = (uint8_t)newB;
        Serial.printf(" *** [DIAL 1 - Manual Light] Brightness: %u%% ***\n", currentLightBrightness);
    } 
    if (inputManager.isButton1Clicked()) {
        Serial.println(" *** [BUTTON 1 CLICKED] ***");
    }



    // ------------------------------------------------------------------------
    // Rotary Encoder 2:
    // ------------------------------------------------------------------------
    int delta2 = inputManager.getEncoder2Delta();
    if (delta2 != 0) {
        if (delta2 > 0) {
                //dial rotated clockwise do stuff here
            }else {
                //dial rotated counter clockwise do stuff here
            }
    }

    if (inputManager.isButton2Clicked()) {
        Serial.printf(" *** [BUTTON 2 CLICKED]");
        //Dial 2 is clicked
    }



    // ------------------------------------------------------------------------
    // Alarm Dismiss / Deactivation & Timeout State Variables
    // ------------------------------------------------------------------------
    static bool bellDismissed = false;
    static bool audioDismissed = false;
    static uint32_t bellStartMillis = 0;
    static uint32_t audioStartMillis = 0;

    bool isAtAlarmTime = (timeManager.getHour24() == alarmManager.getAlarmHour() && 
                          timeManager.getMinute() == alarmManager.getAlarmMinute());
    int minsUntilAlarm = alarmManager.getMinutesUntilAlarm(timeManager.getHour24(), timeManager.getMinute());

    // Single-minute toggle reset: as soon as the alarm minute eclipses, clear bell dismissal!
    if (!isAtAlarmTime) {
        bellDismissed = false;
    }

    // Reset audio dismissal once outside the audio/alarm window
    if (minsUntilAlarm > AlarmManager::AUDIO_RAMP_MINS && !isAtAlarmTime) {
        audioDismissed = false;
        audioStartMillis = 0;
    }

    // ------------------------------------------------------------------------
    // Switches State Change Detection (Immediate Feedback)
    // ------------------------------------------------------------------------
    static bool lastSw1 = inputManager.switchState(1);
    static bool lastSw2 = inputManager.switchState(2);
    static bool lastSw3 = inputManager.switchState(3);

    // Current switch states are read each loop iteration, compare to last known states
    bool curSw1 = inputManager.switchState(1);
    bool curSw2 = inputManager.switchState(2);
    bool curSw3 = inputManager.switchState(3);

    if (curSw1 != lastSw1) {
        lastSw1 = curSw1;
        Serial.printf(" *** [SWITCH 1 - Light Arm] -> %s ***\n", curSw1 ? "ARMED" : "DISARMED");
        // NOTE: Flipping Switch 1 down disarms future sunrise alarms, but does NOT turn off the physical lamp!
    }
    if (curSw2 != lastSw2) {
        lastSw2 = curSw2;
        Serial.printf(" *** [SWITCH 2 - Audio Arm] -> %s ***\n", curSw2 ? "ARMED" : "DISARMED");
        if (!curSw2) {
            if (alarmManager.isAudioActive()) {
                Serial.println(" >>> [AUDIO] Disarmed -> Stopping audio playback...");
                alarmManager.stopAudio();
            }
            audioDismissed = true; // Dismissed for this alarm event; re-arming won't replay immediately
            audioStartMillis = 0;
        }
    }
    if (curSw3 != lastSw3) {
        lastSw3 = curSw3;
        Serial.printf(" *** [SWITCH 3 - Bell Arm]  -> %s ***\n", curSw3 ? "ARMED" : "DISARMED");
        if (!curSw3) {
            if (alarmManager.isBellActive()) {
                Serial.println(" >>> [BELL] Disarmed -> Stopping bell cadence...");
                alarmManager.stopBell();
            }
            if (isAtAlarmTime) {
                bellDismissed = true; // Prevents re-ringing if flipped back ON within this same minute!
            }
        }
    }

    // ------------------------------------------------------------------------
    // Display Update (Non-blocking, 250ms cadence with blinking colon)
    // ------------------------------------------------------------------------
    uint32_t currentMillis = millis();
    static uint32_t lastDisplayUpdate = 0;
    if (currentMillis - lastDisplayUpdate >= 250) {
        lastDisplayUpdate = currentMillis;
        bool colon = (currentMillis / 500) % 2 == 0; // 1-second period blinking colon

        // Update display with current time in 12-hour format
        displayManager.showTime(timeManager.getHour12(), timeManager.getMinute(), colon, false);
    }

    // Periodic time heartbeat every 2 seconds
    if (currentMillis - lastPrintMillis >= 2000) {
        lastPrintMillis = currentMillis;

        const char* audioStatusStr = alarmManager.isAudioActive() ? "PLAYING" : 
                                    (alarmManager.isAudioConnected() ? "READY" : "OFFLINE");

        Serial.printf("[RTC LIVE] %s | Time: %s (%02d:%02d %s) | SW[1,2,3]: [%d, %d, %d] | Bell: %s | Audio: %s | Light: %u%%\n",
                      timeManager.getFormattedDate(),
                      timeManager.getFormattedTime(),
                      timeManager.getHour12(),
                      timeManager.getMinute(),
                      timeManager.isPM() ? "PM" : "AM",
                      curSw1, curSw2, curSw3,
                      alarmManager.isBellActive() ? "ACTIVE" : "OFF",
                      audioStatusStr,
                      currentLightBrightness);
    }

    // ------------------------------------------------------------------------
    // Sunrise & Alarm Timeline Engine (Non-blocking)
    // ------------------------------------------------------------------------

    // 1. Sunrise Light Ramp (Dynamic timing from SUNRISE_DURATION_MINS)
    if (minsUntilAlarm > 0 && minsUntilAlarm <= AlarmManager::SUNRISE_DURATION_MINS) {
        if (curSw1) { // Light switch is ARMED
            float t_light = (float)(AlarmManager::SUNRISE_DURATION_MINS - minsUntilAlarm);
            uint8_t calculatedBrightness = alarmManager.calculateSunriseBrightness(t_light);

            // Manual adjustment protection: only brighten, never dim if set higher manually!
            if (calculatedBrightness > currentLightBrightness) {
                currentLightBrightness = calculatedBrightness;
            }

            static uint8_t lastReportedBrightness = 255;
            if (currentLightBrightness != lastReportedBrightness) {
                lastReportedBrightness = currentLightBrightness;
                Serial.printf(" >>> [SUNRISE LIGHT] t = %.0f min | Brightness: %u%% <<<\n", 
                              t_light, currentLightBrightness);
            }
            // Future LightController hook: lightController.setBrightness(currentLightBrightness);
        }
    }
    // When minsUntilAlarm == 0 or after, the math equation no longer runs and light locks at its peak.

    // 2. Audio Nature / Bird Sound Ramp (Dynamic timing from AUDIO_RAMP_MINS)
    if (minsUntilAlarm > 0 && minsUntilAlarm <= AlarmManager::AUDIO_RAMP_MINS) {
        if (curSw2 && !audioDismissed) { // Audio switch is ARMED and not dismissed
            float t_audio = (float)(AlarmManager::AUDIO_RAMP_MINS - minsUntilAlarm);
            uint8_t volume = alarmManager.calculateAudioVolume(t_audio);
            alarmManager.setVolume(volume);

            if (!alarmManager.isAudioActive()) {
                Serial.printf(" >>> [SUNRISE AUDIO] Starting birds at vol %u (t = %.0f min) <<<\n", volume, t_audio);
                alarmManager.playTrack(1, true); // Loop continuously
                if (audioStartMillis == 0) audioStartMillis = millis();
            }
        }
    }

    // 3. Alarm Time Hit (0 mins remaining) -> Twin-Bell Clapper Motor Cadence & Audio Hold
    if (minsUntilAlarm == 0) {
        static uint8_t lastAlarmTriggerMinute = 255;
        if (timeManager.getMinute() != lastAlarmTriggerMinute) {
            lastAlarmTriggerMinute = timeManager.getMinute();
            Serial.printf(" >>> [ALARM TRIGGERED] Target time reached (%s)! <<<\n",
                          alarmManager.getFormattedAlarmTime(true));
        }

        // Bell motor cadence
        if (curSw3 && !bellDismissed) { // Bell switch is ARMED and not dismissed
            if (!alarmManager.isBellActive()) {
                Serial.println(" >>> [ALARM BELL] Cadence motor clapper ringing! <<<");
                alarmManager.startBell();
                bellStartMillis = millis();
            }
        }

        // Audio: Keep birds playing on seamless loop at peak volume alongside bell if armed & not dismissed
        if (curSw2 && !audioDismissed) {
            alarmManager.setVolume(AlarmManager::AUDIO_MAX_VOLUME);
            if (!alarmManager.isAudioActive()) {
                Serial.printf(" >>> [ALARM AUDIO] Playing birds at peak vol %u <<<\n", AlarmManager::AUDIO_MAX_VOLUME);
                alarmManager.playTrack(1, true); // Loop continuously
                if (audioStartMillis == 0) audioStartMillis = millis();
            }
        }
    }

    // ------------------------------------------------------------------------
    // 4. Auto-Timeouts (5-minute safety shutoff for Bell & Audio)
    // ------------------------------------------------------------------------
    if (alarmManager.isBellActive() && (millis() - bellStartMillis >= AlarmManager::AUTO_SHUTOFF_MILLIS)) {
        Serial.println(" >>> [ALARM BELL] 5-minute timeout reached -> Auto stopped (still armed). <<<");
        alarmManager.stopBell();
        bellDismissed = true; // Keep silenced for the rest of this alarm minute
    }

    if (alarmManager.isAudioActive() && (millis() - audioStartMillis >= AlarmManager::AUTO_SHUTOFF_MILLIS)) {
        Serial.println(" >>> [AUDIO] 5-minute timeout reached -> Auto stopped (still armed). <<<");
        alarmManager.stopAudio();
        audioDismissed = true;
    }
}
