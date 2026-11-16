/*
 * main.cpp - AURA 1 Companion Robot Master Firmware
 * Arfat Ansari | Nov 2026
 *
 * ESP32-S3 dual core setup:
 * - Core 0: reads sensors in the background (ToF laser distance, capacitive touch, mic)
 * - Core 1: runs the servo kinematics and draws eye animations on dual GC9A01 screens
 *
 * this way the displays never stutter or lag when sensors are reading over I2C
 */

#include <Arduino.h>
#include "kinematics.h"
#include "servo_smooth.h"
#include "display_eyes.h"
#include "sensors.h"
#include "halo_leds.h"
#include "emotion_engine.h"

// hardware modules
AuraKinematics    kinematics;
AuraEyeEngine     eyeEngine;
AuraSensors       sensorEngine;
AuraHaloLEDs      haloLEDs;
AuraEmotionEngine emotionEngine;

// tasks for both ESP32 cores
TaskHandle_t TaskCore0 = NULL;
TaskHandle_t TaskCore1 = NULL;

// ---------------------------------------------------------------------------
// CORE 0: Sensor polling task (50Hz)
// ---------------------------------------------------------------------------
void TaskCore0_Perception(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t interval = pdMS_TO_TICKS(20); // 50 Hz

    for (;;) {
        // update laser distance sensor and head touch pad
        sensorEngine.update();
        vTaskDelayUntil(&lastWakeTime, interval);
    }
}

// ---------------------------------------------------------------------------
// CORE 1: Movement and eye display rendering (~60fps)
// ---------------------------------------------------------------------------
void TaskCore1_KinematicsAndRender(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t interval = pdMS_TO_TICKS(16); // 16ms = ~60 Hz

    for (;;) {
        uint32_t now = millis();
        const SensorReadings &s = sensorEngine.getReadings();

        // 1. check what the robot should do based on distance and touch
        emotionEngine.processPerception(s, now);

        // 2. update target angles and eye moods
        emotionEngine.updateActuators(kinematics, eyeEngine, haloLEDs, now);

        // 3. dilate pupils if someone gets close and smooth servo movement
        eyeEngine.adaptToDistance(s.distanceMm);
        kinematics.update(0.12f);
        eyeEngine.update(now);
        haloLEDs.update(now);

        vTaskDelayUntil(&lastWakeTime, interval);
    }
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("=====================================");
    Serial.println("  AURA 1 Companion Robot starting up ");
    Serial.println("  Author: Arfat Ansari               ");
    Serial.println("=====================================");

    // start all subsystems
    eyeEngine.init();
    kinematics.init();
    sensorEngine.init();
    haloLEDs.init();
    emotionEngine.init(millis());

    Serial.println("[AURA] all hardware modules started");

    // launch tasks on each core
    xTaskCreatePinnedToCore(
        TaskCore0_Perception,
        "Sensors_Core0",
        4096,
        NULL,
        4,
        &TaskCore0,
        0 // run on Core 0
    );

    xTaskCreatePinnedToCore(
        TaskCore1_KinematicsAndRender,
        "Render_Core1",
        8192,
        NULL,
        4,
        &TaskCore1,
        1 // run on Core 1
    );

    Serial.println("[AURA] dual core tasks running");
}

void loop() {
    // nothing needed here, FreeRTOS tasks handle everything
    vTaskDelay(pdMS_TO_TICKS(1000));
}
