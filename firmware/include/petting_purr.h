/*
 * petting_purr.h - AURA 1 Capacitive Petting & Purr Vibration System
 * Arfat Ansari | Nov 2026
 *
 * Implements capacitive touch sensing and haptic purr feedback:
 * - Capacitive touch sensing through the 3D printed skull dome shell
 * - Dynamic baseline drift tracking for ambient temperature/humidity immunity
 * - 25 Hz micro-dither servo purring with amplitude ramping (0.6 deg to 1.8 deg)
 * - Stroke duration and velocity classification (gentle stroke vs energetic pat)
 * - Happy squint anime eye synchronization and purr audio triggering
 */

#ifndef AURA_PETTING_PURR_H
#define AURA_PETTING_PURR_H

#include <Arduino.h>
#include <math.h>

// Touch pad pin on top crown of skull dome (ESP32-S3 Touch Pad GPIO 1)
#define PIN_HEAD_TOUCH 1

// Purr frequency: 25 Hz = 40ms period (20ms forward, 20ms reverse)
#define PURR_HALF_PERIOD_MS  20
#define PURR_MIN_AMPLITUDE   0.65f // Subtle onset vibration in degrees
#define PURR_MAX_AMPLITUDE   1.85f // Fully relaxed purr vibration in degrees

enum PetStrokeType {
    STROKE_NONE,
    STROKE_GENTLE_PET,
    STROKE_ENERGETIC_PAT
};

struct PettingState {
    bool isBeingPetted;
    uint32_t petStartTime;
    uint32_t petDurationMs;
    uint16_t touchRaw;
    float touchFiltered;
    float touchBaseline;
    float purrAngleOffset;
    float purrIntensity; // 0.0 to 1.0 ramping factor
    PetStrokeType strokeType;
};

class AuraPettingEngine {
private:
    PettingState state;
    uint32_t lastPurrToggleMs;
    bool purrDirection;
    uint8_t touchConfidence;
    float lastTouchDelta;

public:
    AuraPettingEngine() :
        lastPurrToggleMs(0), purrDirection(false), touchConfidence(0), lastTouchDelta(0.0f) {
        state.isBeingPetted   = false;
        state.petStartTime    = 0;
        state.petDurationMs   = 0;
        state.touchRaw        = 0;
        state.touchFiltered   = 40000.0f;
        state.touchBaseline   = 40000.0f;
        state.purrAngleOffset = 0.0f;
        state.purrIntensity   = 0.0f;
        state.strokeType      = STROKE_NONE;
    }

    void init() {
        // Sample baseline capacitance across 25 samples during startup
        uint32_t sum = 0;
        for (int i = 0; i < 25; i++) {
            sum += touchRead(PIN_HEAD_TOUCH);
            delay(4);
        }
        state.touchBaseline = (float)(sum / 25);
        if (state.touchBaseline < 5000.0f) state.touchBaseline = 35000.0f;
        state.touchFiltered = state.touchBaseline;
    }

    // Run every 10-20ms from the Core 0 perception loop
    void update(uint32_t now) {
        state.touchRaw = touchRead(PIN_HEAD_TOUCH);

        // Low-pass filter (alpha = 0.22) to suppress motor EMI spikes
        float prevFiltered = state.touchFiltered;
        state.touchFiltered += ((float)state.touchRaw - state.touchFiltered) * 0.22f;
        lastTouchDelta = fabsf(state.touchFiltered - prevFiltered);

        // Human hand presence decreases reading by ~25-35%
        bool touched = (state.touchFiltered < (state.touchBaseline * 0.72f));

        if (touched) {
            if (touchConfidence < 5) touchConfidence++;
        } else {
            if (touchConfidence > 0) touchConfidence--;
        }

        bool wasPetted = state.isBeingPetted;
        state.isBeingPetted = (touchConfidence >= 3);

        if (state.isBeingPetted) {
            if (!wasPetted) {
                state.petStartTime = now;
                state.purrIntensity = 0.2f;
            }
            state.petDurationMs = now - state.petStartTime;

            // Classify stroke dynamics
            if (lastTouchDelta > 120.0f) {
                state.strokeType = STROKE_ENERGETIC_PAT;
            } else {
                state.strokeType = STROKE_GENTLE_PET;
            }

            // Smoothly ramp up purr intensity over 1.5 seconds of continuous petting
            float targetIntensity = (state.petDurationMs > 1500) ? 1.0f : ((float)state.petDurationMs / 1500.0f);
            state.purrIntensity += (targetIntensity - state.purrIntensity) * 0.12f;

            // 25 Hz micro-dither servo flutter for realistic organic purring
            if (now - lastPurrToggleMs >= PURR_HALF_PERIOD_MS) {
                lastPurrToggleMs = now;
                purrDirection = !purrDirection;
                float currentAmp = PURR_MIN_AMPLITUDE + state.purrIntensity * (PURR_MAX_AMPLITUDE - PURR_MIN_AMPLITUDE);
                state.purrAngleOffset = purrDirection ? currentAmp : -currentAmp;
            }
        } else {
            state.petDurationMs   = 0;
            state.purrAngleOffset = 0.0f;
            state.purrIntensity   = 0.0f;
            state.strokeType      = STROKE_NONE;

            // Slow ambient baseline adaptation when hand is removed
            state.touchBaseline += (state.touchFiltered - state.touchBaseline) * 0.003f;
        }
    }

    bool isPetted() const { return state.isBeingPetted; }
    float getPurrOffset() const { return state.purrAngleOffset; }
    float getPurrIntensity() const { return state.purrIntensity; }
    uint32_t getPetDuration() const { return state.petDurationMs; }
    float getTouchLevel() const { return state.touchFiltered; }
    PetStrokeType getStrokeType() const { return state.strokeType; }
};

#endif // AURA_PETTING_PURR_H
