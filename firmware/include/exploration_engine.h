/*
 * exploration_engine.h - AURA 1 Autonomous Environmental Exploration & Gaze Engine
 * Arfat Ansari | Nov 2026
 *
 * Implements autonomous desk exploration, ambient light tracking, and inquisitive gaze:
 * - 3-DOF spherical ambient gaze waypoints across desktop work area (-35 to +35 deg pan)
 * - Inquisitive head roll tilts (-8 to +8 deg) when inspecting nearby objects
 * - Ambient room illumination monitoring via VL53L1X ambient photon count rate
 * - Automatic GC9A01 display backlight brightness and pupil dilation modulation
 * - Directional acoustic gaze fusion with smooth orientation hold
 * - Motor thermal duty-cycle throttling to prevent servo overheating
 */

#ifndef AURA_EXPLORATION_ENGINE_H
#define AURA_EXPLORATION_ENGINE_H

#include <Arduino.h>
#include <math.h>
#include "sensors.h"
#include "kinematics.h"

enum ExplorationState {
    EXPLORE_SCANNING,        // Scanning desktop environment with smooth fixations
    EXPLORE_FIXATED_OBJECT,  // Gazing curiously at a detected nearby object
    EXPLORE_ACOUSTIC_LOCK,   // Oriented toward a localized sound source
    EXPLORE_IDLE_REST        // Thermal duty-cycle rest to keep servos cool
};

struct ExplorationConfig {
    float minPanDeg;          // Leftmost scan angle (-35.0f)
    float maxPanDeg;          // Rightmost scan angle (+35.0f)
    float minPitchDeg;        // Downward tilt looking at desk surface (-12.0f)
    float maxPitchDeg;        // Upward glance (+15.0f)
    uint32_t minFixateMs;     // Minimum dwell on a point of interest (800ms)
    uint32_t maxFixateMs;     // Maximum dwell on a point of interest (2400ms)
    uint16_t objectThresholdMm;// Distance below which an object triggers curiosity (450mm)
};

class AuraExplorationEngine {
private:
    ExplorationState state;
    ExplorationConfig config;

    uint32_t stateEnterTimeMs;
    uint32_t lastGazeShiftMs;
    uint32_t currentDwellDurationMs;
    uint32_t lastRestTimeMs;

    // Current exploration kinematic targets
    float currentPan;
    float currentPitch;
    float currentRoll;
    float targetPan;
    float targetPitch;
    float targetRoll;

    // Ambient light tracking (via VL53L1X ambient photon rate)
    float ambientLightSmoothed;  // Rolling average photon count (lux equivalent)
    float displayBacklightDuty;  // 0.2 to 1.0 PWM ratio
    float pupilDilationRatio;    // 0.75 (bright) to 1.30 (dim light)

    // Acoustic orientation tracking
    float acousticHoldAzimuth;
    uint32_t acousticHoldUntilMs;

    // Servo duty-cycle governor
    float cumulativeDegreesMoved;
    bool  isInThermalRest;

    // Pseudo-random helper
    float randomFloat(float minVal, float maxVal) {
        return minVal + (float)random(1000) / 1000.0f * (maxVal - minVal);
    }

public:
    AuraExplorationEngine() {
        state = EXPLORE_SCANNING;
        config.minPanDeg = -35.0f;
        config.maxPanDeg = 35.0f;
        config.minPitchDeg = -12.0f;
        config.maxPitchDeg = 15.0f;
        config.minFixateMs = 800;
        config.maxFixateMs = 2400;
        config.objectThresholdMm = 450;

        currentPan = 0.0f;
        currentPitch = 0.0f;
        currentRoll = 0.0f;
        targetPan = 0.0f;
        targetPitch = 0.0f;
        targetRoll = 0.0f;

        ambientLightSmoothed = 120.0f; // Typical indoor room baseline
        displayBacklightDuty = 0.85f;
        pupilDilationRatio = 1.0f;

        acousticHoldAzimuth = 0.0f;
        acousticHoldUntilMs = 0;
        cumulativeDegreesMoved = 0.0f;
        isInThermalRest = false;

        stateEnterTimeMs = 0;
        lastGazeShiftMs = 0;
        currentDwellDurationMs = 1200;
        lastRestTimeMs = 0;
    }

    void init(uint32_t nowMs) {
        stateEnterTimeMs = nowMs;
        lastGazeShiftMs = nowMs;
        lastRestTimeMs = nowMs;
        pickNextWaypoint(nowMs);
    }

    // Selects an organic new desktop glance waypoint
    void pickNextWaypoint(uint32_t nowMs) {
        lastGazeShiftMs = nowMs;
        currentDwellDurationMs = random(config.minFixateMs, config.maxFixateMs);

        // Pick pan angle with subtle center-bias (more likely to look near user)
        float u1 = randomFloat(-1.0f, 1.0f);
        float u2 = randomFloat(-1.0f, 1.0f);
        float panBias = (u1 + u2) * 0.5f; // Triangular distribution
        targetPan = panBias * config.maxPanDeg;

        // Downward bias for desk viewing
        targetPitch = randomFloat(config.minPitchDeg, config.maxPitchDeg * 0.7f);

        // Subtle inquisitive head tilt matching the look direction
        targetRoll = (targetPan / config.maxPanDeg) * randomFloat(-4.5f, 4.5f);

        // Track mechanical distance moved for thermal safety
        float delta = fabsf(targetPan - currentPan) + fabsf(targetPitch - currentPitch);
        cumulativeDegreesMoved += delta;

        // Enforce thermal rest every ~350 degrees of cumulative motion
        if (cumulativeDegreesMoved > 350.0f) {
            cumulativeDegreesMoved = 0.0f;
            isInThermalRest = true;
            state = EXPLORE_IDLE_REST;
            stateEnterTimeMs = nowMs;
            currentDwellDurationMs = 1500; // 1.5s peaceful pause
        }
    }

    void update(const SensorReadings &sensors, uint32_t nowMs) {
        // 1. Process Ambient Room Illumination (Adaptive Backlight & Pupils)
        // sensors.ambientCount represents ambient photon flux from VL53L1X
        float rawAmbient = (float)sensors.ambientCount;
        if (rawAmbient < 5.0f) rawAmbient = 5.0f;
        ambientLightSmoothed = (ambientLightSmoothed * 0.96f) + (rawAmbient * 0.04f);

        // Map ambient lux to display backlight (bright room -> 100% BL, dim room -> 35% BL)
        displayBacklightDuty = constrain(0.35f + (ambientLightSmoothed / 400.0f) * 0.65f, 0.35f, 1.0f);

        // Map ambient lux to biological pupil dilation (bright room -> small pupil, dark -> wide)
        pupilDilationRatio = constrain(1.30f - (ambientLightSmoothed / 500.0f) * 0.55f, 0.75f, 1.30f);

        // 2. Sound Orientation Lock Check
        if (sensors.soundLocalized && nowMs < sensors.soundLockUntilMs) {
            state = EXPLORE_ACOUSTIC_LOCK;
            acousticHoldAzimuth = sensors.soundAzimuthDeg;
            targetPan = constrain(acousticHoldAzimuth, config.minPanDeg, config.maxPanDeg);
            targetPitch = 3.5f; // Look up attentively
            targetRoll = (targetPan > 0) ? 6.5f : -6.5f; // Curious head tilt toward sound
            lastGazeShiftMs = nowMs;
        }

        // 3. Proximity Object Inspection
        else if (sensors.distanceMm > 50 && sensors.distanceMm < config.objectThresholdMm) {
            if (state != EXPLORE_FIXATED_OBJECT) {
                state = EXPLORE_FIXATED_OBJECT;
                stateEnterTimeMs = nowMs;
                // Center attention on the nearby object
                targetPan = 0.0f;
                targetPitch = -8.0f; // Look down at object
                // Inquisitive inquisitive roll tilt
                targetRoll = (random(2) == 0) ? 8.0f : -8.0f;
                lastGazeShiftMs = nowMs;
                currentDwellDurationMs = 2800; // Linger to examine
            }
        }

        // 4. Ambient Scanning & Waypoint Stepping
        else {
            if (state == EXPLORE_IDLE_REST) {
                if (nowMs - stateEnterTimeMs > currentDwellDurationMs) {
                    state = EXPLORE_SCANNING;
                    pickNextWaypoint(nowMs);
                }
            } else {
                state = EXPLORE_SCANNING;
                if (nowMs - lastGazeShiftMs > currentDwellDurationMs) {
                    pickNextWaypoint(nowMs);
                }
            }
        }

        // 5. Exponential Smoothing toward Target Angles
        float smoothFactor = (state == EXPLORE_ACOUSTIC_LOCK) ? 0.18f : 0.08f;
        currentPan   += (targetPan - currentPan) * smoothFactor;
        currentPitch += (targetPitch - currentPitch) * smoothFactor;
        currentRoll  += (targetRoll - currentRoll) * smoothFactor;
    }

    // Kinematic target accessors
    float getPan() const { return currentPan; }
    float getPitch() const { return currentPitch; }
    float getRoll() const { return currentRoll; }

    // Visual modulation accessors
    float getBacklightDuty() const { return displayBacklightDuty; }
    float getPupilDilation() const { return pupilDilationRatio; }
    ExplorationState getState() const { return state; }
    bool isAcousticLocked() const { return state == EXPLORE_ACOUSTIC_LOCK; }
};

#endif // AURA_EXPLORATION_ENGINE_H
