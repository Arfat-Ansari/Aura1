/*
 * petting_purr.h - AURA 1 Capacitive Petting & Purr Vibration System
 * Arfat Ansari | Nov 2026
 *
 * handles:
 * - capacitive touch sensing through the 3D printed skull shell
 * - 25 Hz micro-dither servo purring (feels like a cat purring under your hand)
 * - happy squint anime eye curves (^ _ ^)
 * - warm amber breathing halo light
 */

#ifndef AURA_PETTING_PURR_H
#define AURA_PETTING_PURR_H

#include <Arduino.h>
#include <math.h>

// touch pad pin on top of head
#define PIN_HEAD_TOUCH 1

// purr frequency: 25 Hz = 40ms full period (20ms high, 20ms low)
#define PURR_HALF_PERIOD_MS 20
#define PURR_AMPLITUDE_DEG  1.8f // tiny servo vibration in degrees

struct PettingState {
    bool isBeingPetted;
    uint32_t petStartTime;
    uint32_t petDurationMs;
    uint16_t touchRaw;
    float touchFiltered;
    float touchBaseline;
    float purrAngleOffset;
};

class AuraPettingEngine {
private:
    PettingState state;
    uint32_t lastPurrToggleMs;
    bool purrDirection;
    uint8_t touchConfidence;

public:
    AuraPettingEngine() :
        lastPurrToggleMs(0), purrDirection(false), touchConfidence(0) {
        state.isBeingPetted = false;
        state.petStartTime = 0;
        state.petDurationMs = 0;
        state.touchRaw = 0;
        state.touchFiltered = 40000.0f;
        state.touchBaseline = 40000.0f;
        state.purrAngleOffset = 0.0f;
    }

    void init() {
        // take initial baseline readings to calibrate for ambient capacitance
        uint32_t sum = 0;
        for (int i = 0; i < 20; i++) {
            sum += touchRead(PIN_HEAD_TOUCH);
            delay(5);
        }
        state.touchBaseline = (float)(sum / 20);
        if (state.touchBaseline < 5000.0f) state.touchBaseline = 35000.0f;
        state.touchFiltered = state.touchBaseline;
    }

    // run this every 10-20ms from the perception loop
    void update(uint32_t now) {
        state.touchRaw = touchRead(PIN_HEAD_TOUCH);

        // low pass filter to avoid noise spikes
        state.touchFiltered += ((float)state.touchRaw - state.touchFiltered) * 0.20f;

        // touch threshold: human hand drops capacitance reading by ~25-35%
        bool touched = (state.touchFiltered < (state.touchBaseline * 0.72f));

        if (touched) {
            if (touchConfidence < 4) touchConfidence++;
        } else {
            if (touchConfidence > 0) touchConfidence--;
        }

        bool wasPetted = state.isBeingPetted;
        state.isBeingPetted = (touchConfidence >= 3);

        if (state.isBeingPetted) {
            if (!wasPetted) {
                state.petStartTime = now;
            }
            state.petDurationMs = now - state.petStartTime;

            // 25 Hz micro-dither servo flutter for realistic purring
            if (now - lastPurrToggleMs >= PURR_HALF_PERIOD_MS) {
                lastPurrToggleMs = now;
                purrDirection = !purrDirection;
                state.purrAngleOffset = purrDirection ? PURR_AMPLITUDE_DEG : -PURR_AMPLITUDE_DEG;
            }
        } else {
            state.petDurationMs = 0;
            state.purrAngleOffset = 0.0f;

            // slow drift adaptation for baseline when not touched
            state.touchBaseline += (state.touchFiltered - state.touchBaseline) * 0.002f;
        }
    }

    bool isPetted() const { return state.isBeingPetted; }
    float getPurrOffset() const { return state.purrAngleOffset; }
    uint32_t getPetDuration() const { return state.petDurationMs; }
    float getTouchLevel() const { return state.touchFiltered; }
};

#endif // AURA_PETTING_PURR_H
