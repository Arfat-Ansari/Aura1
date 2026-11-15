/**
 * AURA-1 Companion Robot Master Dual-Core Firmware
 * Author: Arfat Ansari
 * Target: ESP32-S3-WROOM-1-N16R8 (Xtensa Dual-Core @ 240MHz, 16MB Flash, 8MB PSRAM)
 * Project: Maker Faire Shenzhen 2026 | Hack Club Fallen
 */

#include <Arduino.h>
#include "kinematics.h"
#include "servo_smooth.h"
#include "display_eyes.h"
#include "sensors.h"
#include "halo_leds.h"
#include "emotion_engine.h"

// Global Subsystem Instances
AuraKinematics    kinematics;
AuraEyeEngine     eyeEngine;
AuraSensors       sensorEngine;
AuraHaloLEDs      haloLEDs;
AuraEmotionEngine emotionEngine;

// FreeRTOS Task Handles
TaskHandle_t TaskHandle_Core0 = NULL;
TaskHandle_t TaskHandle_Core1 = NULL;

// ---------------------------------------------------------------------------
// CORE 0 TASK: Perception & Multi-Modal Sensor Polling (Priority 4)
// ---------------------------------------------------------------------------
void TaskCore0_Perception(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20); // 50 Hz

    for (;;) {
        sensorEngine.update();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// ---------------------------------------------------------------------------
// CORE 1 TASK: Affective Computing, Kinematics & Rendering (Priority 4)
// ---------------------------------------------------------------------------
void TaskCore1_KinematicsAndRender(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(16); // ~60 Hz (16.6ms)

    for (;;) {
        uint32_t now = millis();
        const SensorReadings &s = sensorEngine.getReadings();

        // 1. Process Multimodal Stimuli through Affective Emotion Engine
        emotionEngine.processPerception(s, now);

        // 2. Synthesize Physical and Visual Actuation
        emotionEngine.updateActuators(kinematics, eyeEngine, haloLEDs, now);

        // 3. Dynamic Optical Sensor Modulation & Actuator Interpolators
        eyeEngine.adaptToDistance(s.distanceMm);
        kinematics.update(0.12f);
        eyeEngine.update(now);
        haloLEDs.update(now);

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// ---------------------------------------------------------------------------
// ARDUINO SETUP & ENTRYPOINT
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println(F("=================================================="));
    Serial.println(F("   AURA-1: Autonomous Cybernetic Desk Companion   "));
    Serial.println(F("   Maker Faire Shenzhen 2026 Flagship Showcase   "));
    Serial.println(F("=================================================="));

    // Initialize Hardware Subsystems
    eyeEngine.init();
    kinematics.init();
    sensorEngine.init();
    haloLEDs.init();
    emotionEngine.init(millis());

    Serial.println(F("[SYSTEM] All hardware peripherals initialized successfully."));

    // Launch Dual-Core FreeRTOS Schedulers
    xTaskCreatePinnedToCore(
        TaskCore0_Perception,
        "Core0_Perception",
        4096,
        NULL,
        4,
        &TaskHandle_Core0,
        0 // Pin to Core 0
    );

    xTaskCreatePinnedToCore(
        TaskCore1_KinematicsAndRender,
        "Core1_Kinematics",
        8192,
        NULL,
        4,
        &TaskHandle_Core1,
        1 // Pin to Core 1
    );

    Serial.println(F("[SYSTEM] Dual-core FreeRTOS tasks active on Core 0 & Core 1."));
}

void loop() {
    // Primary execution managed by FreeRTOS tasks; idle power management in loop
    vTaskDelay(pdMS_TO_TICKS(1000));
}
