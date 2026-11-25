/*
 * main.cpp - AURA 1 Master Companion Robot Firmware (Dual-Core FreeRTOS Pipeline)
 * Arfat Ansari | Nov 2026
 *
 * ESP32-S3 Dual-Core FreeRTOS Multithreaded Architecture:
 * - CORE 0 (Perception & Communications, Priority 4):
 *   * 50Hz sensor polling (VL53L1X ToF laser distance & gestures, INMP441 audio FFT)
 *   * Capacitive touch petting engine & 25Hz haptic baseline tracking
 *   * 18650 Li-ion battery voltage ADC monitoring and TP4056 charging state
 *   * Onboard SoftAP Web Server & WebSockets REST API for live telemetry and servo trims
 *   * Thread-safe atomic sensor state publishing via FreeRTOS mutex
 *
 * - CORE 1 (Real-Time Actuation, Motion & Visuals, Priority 5):
 *   * 60 FPS dual GC9A01 240x240 circular IPS screen rendering via SPI DMA
 *   * Saccadic eye micro-movements, pupil dilation, dynamic squinting, and blinking
 *   * 3-DOF spherical neck kinematics calculation with cubic ease-in/out smoothing
 *   * Autonomous desktop behavior tree (idle fidgets, curiosity, tiered sleep cycle)
 *   * WS2812B 12-LED RGB neck halo animations and I2S audio voice synthesizer
 */

#include <Arduino.h>
#include <esp_task_wdt.h>
#include "kinematics.h"
#include "servo_smooth.h"
#include "display_eyes.h"
#include "sensors.h"
#include "halo_leds.h"
#include "emotion_engine.h"
#include "voice_synth.h"
#include "behavior_engine.h"
#include "power_mgr.h"
#include "web_dashboard.h"

// Hardware subsystem modules
AuraKinematics      kinematics;
AuraEyeEngine       eyeEngine;
AuraSensors         sensorEngine;
AuraHaloLEDs        haloLEDs;
AuraEmotionEngine   emotionEngine;
AuraVoiceSynth      voiceEngine;
AuraBehaviorEngine  behaviorEngine;
AuraPowerManager    powerManager;
AuraWebDashboard    webDashboard;

// FreeRTOS Task Handles & Synchronization
TaskHandle_t TaskCore0_Handle = NULL;
TaskHandle_t TaskCore1_Handle = NULL;
SemaphoreHandle_t sensorMutex = NULL;

// Thread-safe shared snapshot of sensor data
SensorReadings sharedSensorData;

// ---------------------------------------------------------------------------
// CORE 0: Perception, Communications & System Telemetry Task (50Hz)
// ---------------------------------------------------------------------------
void TaskCore0_PerceptionAndComms(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t interval = pdMS_TO_TICKS(20); // 20ms = 50 Hz

    // Register Core 0 task with the Task Watchdog Timer (3.0s timeout)
    esp_task_wdt_add(NULL);

    for (;;) {
        uint32_t now = millis();

        // 1. Poll primary hardware sensors (ToF distance, gestures, audio FFT, touch)
        sensorEngine.update();
        const SensorReadings &localReadings = sensorEngine.getReadings();

        // 2. Poll 18650 battery power management
        powerManager.update(now);

        // 3. Thread-safe copy of sensor data for Core 1 consumer
        if (xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(4)) == pdTRUE) {
            sharedSensorData = localReadings;
            sharedSensorData.batteryVoltage = powerManager.getVoltage();
            sharedSensorData.batteryPercent = powerManager.getPercentage();
            sharedSensorData.isCharging     = powerManager.isCharging();
            sharedSensorData.isLowBattery   = powerManager.isLowBattery();
            xSemaphoreGive(sensorMutex);
        }

        // 4. Update SoftAP Web Server (handle REST API requests, client telemetry)
        webDashboard.update(now);
        webDashboard.updateTelemetry(
            localReadings,
            emotionEngine.getCurrentMood(),
            powerManager.getVoltage(),
            powerManager.getPercentage()
        );

        // 5. Petting purr haptic vibration injection
        if (localReadings.headPetted && localReadings.purrVibration != 0.0f) {
            kinematics.setTrimOffsets(
                webDashboard.getPanTrim(),
                webDashboard.getPitchTrim() + localReadings.purrVibration,
                webDashboard.getRollTrim() + (localReadings.purrVibration * 0.5f)
            );
        } else {
            kinematics.setTrimOffsets(
                webDashboard.getPanTrim(),
                webDashboard.getPitchTrim(),
                webDashboard.getRollTrim()
            );
        }

        // Feed the watchdog
        esp_task_wdt_reset();

        vTaskDelayUntil(&lastWakeTime, interval);
    }
}

// ---------------------------------------------------------------------------
// CORE 1: High-Speed Actuation, Visuals & Kinematics Pipeline (~60 FPS)
// ---------------------------------------------------------------------------
void TaskCore1_RenderAndActuation(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t interval = pdMS_TO_TICKS(16); // 16.6ms = 60 FPS

    esp_task_wdt_add(NULL);

    SensorReadings currentSensors;

    for (;;) {
        uint32_t now = millis();

        // 1. Thread-safe snapshot fetch from Core 0
        if (xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(3)) == pdTRUE) {
            currentSensors = sharedSensorData;
            xSemaphoreGive(sensorMutex);
        }

        // 2. Update autonomous desktop behavior tree (fidgets, sleep/wake, startle)
        behaviorEngine.update(currentSensors, now);

        // 3. Process emotional states and voice synthesis triggers
        emotionEngine.processPerception(currentSensors, voiceEngine, now);

        // 4. Update kinematic targets and facial expressions
        emotionEngine.updateActuators(kinematics, eyeEngine, haloLEDs, now);

        // 5. Modulate pupil dilation, squint, and eyelids based on behavior state
        if (behaviorEngine.isSleeping()) {
            eyeEngine.setSquint(behaviorEngine.getSquint());
            haloLEDs.setBrightness(behaviorEngine.getBreathBrightness());
        } else if (behaviorEngine.getState() == BEHAVIOR_STARTLED_WAKE) {
            eyeEngine.setDilation(behaviorEngine.getDilation());
            haloLEDs.setEffect(HALO_ALERT_ELECTRIC_BLUE);
        } else {
            eyeEngine.adaptToDistance(currentSensors.distanceMm);
        }

        // 6. Audio-visual synchrony (jaw/eye/halo modulation from sound amplitude)
        voiceEngine.update(now);
        eyeEngine.syncWithVoice(voiceEngine.getAudioLevel(), voiceEngine.getPitchRatio());
        haloLEDs.syncWithVoice(voiceEngine.getAudioLevel());

        // 7. Execute 3-DOF spherical kinematics and ease-in/out smoothing
        kinematics.update(0.12f);

        // 8. Stream dual 240x240 circular IPS frames via SPI DMA
        eyeEngine.update(now);

        // 9. Update WS2812B 12-LED neck halo animations
        haloLEDs.update(now);

        esp_task_wdt_reset();

        vTaskDelayUntil(&lastWakeTime, interval);
    }
}

// ---------------------------------------------------------------------------
// Hardware System Setup & FreeRTOS Dual-Core Launch
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(400);

    Serial.println("==================================================");
    Serial.println("  AURA 1 Companion Robot - Master FreeRTOS Firmware");
    Serial.println("  Author: Arfat Ansari | Maker Faire Shenzhen 2026");
    Serial.println("==================================================");

    // Initialize mutex for thread-safe sensor state sharing
    sensorMutex = xSemaphoreCreateMutex();

    // 1. Initialize display pipeline on Core 1
    eyeEngine.init();
    Serial.println("[AURA] Dual GC9A01 circular IPS displays initialized");

    // 2. Initialize 3-DOF spherical neck kinematics & servos
    kinematics.init();
    Serial.println("[AURA] 3-DOF spherical kinematics & servos online");

    // 3. Initialize perception sensors & battery ADC
    sensorEngine.init();
    powerManager.init();
    Serial.println("[AURA] Sensors, ToF, and power management online");

    // 4. Initialize halo LEDs & I2S voice synthesis
    haloLEDs.init();
    voiceEngine.init();
    Serial.println("[AURA] Halo LEDs and MAX98357A voice synth online");

    // 5. Initialize emotion state machine and behavior tree
    uint32_t startupTime = millis();
    emotionEngine.init(startupTime);
    behaviorEngine.init(startupTime);
    Serial.println("[AURA] Behavior and emotion engine online");

    // 6. Initialize SoftAP Web Dashboard (192.168.4.1)
    webDashboard.init();
    Serial.println("[AURA] SoftAP Web Dashboard active (SSID: AURA-1-COMPANION)");

    // Initialize FreeRTOS Task Watchdog Timer (TWDT)
    esp_task_wdt_init(3, true); // 3-second watchdog panic reset

    // Launch Core 0 Task: Perception, Comms & Web Server (Priority 4)
    xTaskCreatePinnedToCore(
        TaskCore0_PerceptionAndComms,
        "Task_Core0_Perception",
        8192,
        NULL,
        4,
        &TaskCore0_Handle,
        0 // Core 0
    );

    // Launch Core 1 Task: Real-Time Render, Motion & Actuation (Priority 5)
    xTaskCreatePinnedToCore(
        TaskCore1_RenderAndActuation,
        "Task_Core1_Render",
        12288,
        NULL,
        5,
        &TaskCore1_Handle,
        1 // Core 1
    );

    Serial.println("[AURA] Dual-Core FreeRTOS master pipeline active!");
}

void loop() {
    // Both cores run in independent FreeRTOS tasks; idle the Arduino loop
    vTaskDelay(pdMS_TO_TICKS(1000));
}
