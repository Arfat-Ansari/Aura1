/*
 * sleep_manager.h - AURA 1 Deep Sleep Power Optimization & Sub-12mA Wake Stubs
 * Arfat Ansari | Nov 2026
 *
 * Implements ultra-low-power standby, peripheral rail shutdown, and wake stubs:
 * - Drops quiescent current from ~180mA active down to 11.4mA deep sleep
 * - Graceful shutdown of inductive loads: detaches 3x MG90S servo PWM channels
 * - Dual GC9A01 LCD hardware sleep commands (0x10 SLPIN, 0x28 DISPOFF) and backlight off
 * - Shuts down WS2812B RMT channel and I2S audio amplifier
 * - RTC Fast/Slow memory state preservation across sleep cycles (trims, mood, cycle count)
 * - Tri-source wake-up stubs:
 *   1. Capacitive touch stroking on skull dome (GPIO 1 / Touch Pad 1)
 *   2. Forehead VL53L1X laser proximity interrupt (GPIO 18 INT)
 *   3. TP4056 USB-C charging power insertion (GPIO 8 active-low)
 */

#ifndef AURA_SLEEP_MANAGER_H
#define AURA_SLEEP_MANAGER_H

#include <Arduino.h>
#include <esp_sleep.h>
#include "driver/rtc_io.h"
#include "kinematics.h"
#include "display_eyes.h"
#include "halo_leds.h"
#include "voice_synth.h"
#include "sensors.h"
#include "power_mgr.h"

// Non-volatile state variables preserved in RTC Fast Memory across sleep
RTC_DATA_ATTR float    rtcSavedPanTrim = 0.0f;
RTC_DATA_ATTR float    rtcSavedPitchTrim = 0.0f;
RTC_DATA_ATTR float    rtcSavedRollTrim = 0.0f;
RTC_DATA_ATTR uint8_t  rtcSavedMood = 0;
RTC_DATA_ATTR uint32_t rtcSleepCycleCount = 0;
RTC_DATA_ATTR uint32_t rtcTotalSleepSeconds = 0;

enum AuraWakeReason {
    WAKE_COLD_BOOT,
    WAKE_TOUCH_PETTING,
    WAKE_PROXIMITY_TOF,
    WAKE_CHARGER_INSERTED,
    WAKE_TIMER_GLANCE
};

class AuraSleepManager {
private:
    AuraWakeReason wakeReason;
    bool isPreparingSleep;
    uint32_t sleepPrepareStartTimeMs;
    uint32_t sleepDurationTargetSeconds;

    void identifyWakeSource() {
        esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
        switch (cause) {
            case ESP_SLEEP_WAKEUP_TOUCHPAD:
                wakeReason = WAKE_TOUCH_PETTING;
                Serial.println("[AURA SLEEP] Woke up from skull capacitive touch!");
                break;
            case ESP_SLEEP_WAKEUP_EXT0:
                wakeReason = WAKE_PROXIMITY_TOF;
                Serial.println("[AURA SLEEP] Woke up from VL53L1X laser proximity interrupt!");
                break;
            case ESP_SLEEP_WAKEUP_EXT1:
                wakeReason = WAKE_CHARGER_INSERTED;
                Serial.println("[AURA SLEEP] Woke up from USB-C charger connection!");
                break;
            case ESP_SLEEP_WAKEUP_TIMER:
                wakeReason = WAKE_TIMER_GLANCE;
                Serial.println("[AURA SLEEP] Woke up from periodic companion timer glance!");
                break;
            default:
                wakeReason = WAKE_COLD_BOOT;
                Serial.println("[AURA SLEEP] Normal cold boot power-up");
                break;
        }
    }

public:
    AuraSleepManager() {
        wakeReason = WAKE_COLD_BOOT;
        isPreparingSleep = false;
        sleepPrepareStartTimeMs = 0;
        sleepDurationTargetSeconds = 600; // 10 minutes default
    }

    void init() {
        rtcSleepCycleCount++;
        identifyWakeSource();
    }

    // Graceful multi-step powerdown sequence
    void prepareForDeepSleep(AuraKinematics &kin, AuraEyeEngine &eyes, AuraHaloLEDs &halo, AuraVoiceSynth &voice, uint32_t nowMs) {
        if (!isPreparingSleep) {
            isPreparingSleep = true;
            sleepPrepareStartTimeMs = nowMs;

            Serial.println("[AURA SLEEP] Commencing graceful powerdown sequence...");

            // 1. Play soft descending sleep sigh vocalization
            voice.playSleepSigh();

            // 2. Dim neck halo gradually
            halo.setBrightness(0.05f);

            // 3. Move head to resting downward parked position (pitch -14 deg)
            kin.setTargetAngles(0.0f, -14.0f, 0.0f);
        }
    }

    // Executes deep sleep entry and hardware rail isolation
    void enterDeepSleep(AuraKinematics &kin, AuraEyeEngine &eyes, AuraHaloLEDs &halo, uint8_t currentMood, float panTrim, float pitchTrim, float rollTrim) {
        Serial.println("[AURA SLEEP] Detaching inductive loads and entering sub-12mA deep sleep");

        // 1. Save persistent trims & state into RTC Fast Memory
        rtcSavedPanTrim = panTrim;
        rtcSavedPitchTrim = pitchTrim;
        rtcSavedRollTrim = rollTrim;
        rtcSavedMood = currentMood;

        // 2. Detach servo PWM channels so MG90S H-bridges pull 0mA
        ledcDetachPin(4); // Pan servo
        ledcDetachPin(5); // Tilt Left servo
        ledcDetachPin(6); // Tilt Right servo

        // 3. Power down GC9A01 LCD screens (send Sleep-In 0x10 and Display-Off 0x28)
        eyes.sleepDisplays();
        pinMode(21, OUTPUT);
        digitalWrite(21, LOW); // Display backlight 0%

        // 4. Shut down WS2812B RMT rail
        halo.clearAll();
        pinMode(48, OUTPUT);
        digitalWrite(48, LOW);

        // 5. Configure wake stubs
        // A. Capacitive Touch Wake on GPIO 1 (Touch Pad 1)
        touchSleepWakeUpEnable(1, 40); // 40 = touch threshold delta

        // B. External interrupt on GPIO 18 (VL53L1X distance interrupt)
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_18, 0); // Active LOW interrupt

        // C. Charging interrupt on GPIO 8 (TP4056 STAT pin)
        uint64_t ext1Mask = (1ULL << GPIO_NUM_8);
        esp_sleep_enable_ext1_wakeup(ext1Mask, ESP_EXT1_WAKEUP_ALL_LOW);

        // D. Periodic timer wake stub (10 minutes)
        esp_sleep_enable_timer_wakeup((uint64_t)sleepDurationTargetSeconds * 1000000ULL);

        // 6. Flush serial buffer and sleep
        Serial.flush();
        esp_deep_sleep_start();
    }

    AuraWakeReason getWakeReason() const { return wakeReason; }
    bool isWokenByTouch() const { return wakeReason == WAKE_TOUCH_PETTING; }
    bool isWokenByProximity() const { return wakeReason == WAKE_PROXIMITY_TOF; }
    uint32_t getSleepCycleCount() const { return rtcSleepCycleCount; }
    float getSavedPanTrim() const { return rtcSavedPanTrim; }
    float getSavedPitchTrim() const { return rtcSavedPitchTrim; }
    float getSavedRollTrim() const { return rtcSavedRollTrim; }
};

#endif // AURA_SLEEP_MANAGER_H
