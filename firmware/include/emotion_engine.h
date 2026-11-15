#ifndef AURA_EMOTION_ENGINE_H
#define AURA_EMOTION_ENGINE_H

#include <Arduino.h>
#include <math.h>
#include "kinematics.h"
#include "display_eyes.h"
#include "halo_leds.h"
#include "sensors.h"

/**
 * AURA-1 Affective Computing & Emotion Synthesis Engine
 * 
 * Implements a continuous 2D Valence-Arousal affect space (Russell circumplex)
 * mapping internal emotional states to multi-modal robotic actuation:
 * 1. 3-DOF zero-jerk quintic head kinematics (tilt, pitch, yaw)
 * 2. 12-LED procedural halo shader (chromaticity and pulse tempo)
 * 3. Dual GC9A01 circular eye micro-expressions (dilation, squint, blink rate)
 * 4. 25 Hz micro-vibration haptic purring on servo drive horns
 */

enum AffectState {
    AFFECT_IDLE_BREATHE,      // Calm homeostasis with harmonic breathing
    AFFECT_ATTENTIVE_LISTEN,  // Active focus toward acoustic/visual stimuli
    AFFECT_CURIOSITY_PONDER,  // Inquisitive head cock with eye saccades
    AFFECT_EMPATHY_PURR,      // Physical bonding response to sustained touch
    AFFECT_STARTLE_RECOIL,    // High-G shock or acoustic transient defense
    AFFECT_DEEP_SLEEP         // Quiescent power-saving restorative state
};

struct AffectVector {
    float valence;  // -1.0 (unpleasant/defensive) to +1.0 (pleasurable/bonding)
    float arousal;  //  0.0 (quiescent/slumber) to +1.0 (hyper-reactive/shocked)
};

class AuraEmotionEngine {
private:
    AffectState currentAffect;
    AffectVector currentCoords;
    AffectVector targetCoords;

    uint32_t stateEntryTime;
    uint32_t lastStimulusTime;
    uint32_t lastPurrToggleTime;
    bool     purrPhase;
    float    breathPhase;

    // Kinematic pose targets calculated by affect mapping
    float targetPanDeg;
    float targetPitchDeg;
    float targetRollDeg;
    float trajectoryDurationSec;

public:
    AuraEmotionEngine() :
        currentAffect(AFFECT_IDLE_BREATHE),
        stateEntryTime(0),
        lastStimulusTime(0),
        lastPurrToggleTime(0),
        purrPhase(false),
        breathPhase(0.0f),
        targetPanDeg(0.0f),
        targetPitchDeg(0.0f),
        targetRollDeg(0.0f),
        trajectoryDurationSec(0.6f) {
        currentCoords = {0.2f, 0.25f}; // Nominal peaceful baseline
        targetCoords  = currentCoords;
    }

    void init(uint32_t now) {
        stateEntryTime = now;
        lastStimulusTime = now;
        setAffect(AFFECT_IDLE_BREATHE, now);
    }

    void setAffect(AffectState newState, uint32_t now) {
        currentAffect = newState;
        stateEntryTime = now;

        switch (newState) {
            case AFFECT_IDLE_BREATHE:
                targetCoords = {0.20f, 0.25f};
                trajectoryDurationSec = 0.85f;
                break;
            case AFFECT_ATTENTIVE_LISTEN:
                targetCoords = {0.55f, 0.70f};
                trajectoryDurationSec = 0.35f;
                break;
            case AFFECT_CURIOSITY_PONDER:
                targetCoords = {0.65f, 0.50f};
                trajectoryDurationSec = 0.50f;
                break;
            case AFFECT_EMPATHY_PURR:
                targetCoords = {0.95f, 0.35f};
                trajectoryDurationSec = 0.40f;
                break;
            case AFFECT_STARTLE_RECOIL:
                targetCoords = {-0.85f, 0.98f};
                trajectoryDurationSec = 0.16f; // Rapid instinctive jerk
                break;
            case AFFECT_DEEP_SLEEP:
                targetCoords = {0.05f, 0.05f};
                trajectoryDurationSec = 1.40f; // Very slow heavy droop
                break;
        }
    }

    void processPerception(const SensorReadings &sensors, uint32_t now) {
        // High-priority emergency reflex: mechanical impact or loud shock
        if (sensors.tableBumped || sensors.clapDetected) {
            lastStimulusTime = now;
            setAffect(AFFECT_STARTLE_RECOIL, now);
            return;
        }

        // Tactile bonding takes immediate precedence over wandering
        if (sensors.headPetted) {
            lastStimulusTime = now;
            if (currentAffect != AFFECT_EMPATHY_PURR) {
                setAffect(AFFECT_EMPATHY_PURR, now);
            }
            return;
        }

        // Acoustic vocal detection (human voice speech detected in band)
        if (sensors.speechDetected) {
            lastStimulusTime = now;
            if (currentAffect != AFFECT_ATTENTIVE_LISTEN && currentAffect != AFFECT_EMPATHY_PURR) {
                setAffect(AFFECT_ATTENTIVE_LISTEN, now);
            }
            return;
        }

        // Proximity optical tracking via laser ToF
        if (sensors.visitorClose) {
            lastStimulusTime = now;
            if (currentAffect == AFFECT_IDLE_BREATHE || currentAffect == AFFECT_DEEP_SLEEP) {
                setAffect(AFFECT_CURIOSITY_PONDER, now);
            }
            return;
        }

        if (sensors.visitorPresent) {
            lastStimulusTime = now;
            if (currentAffect == AFFECT_DEEP_SLEEP) {
                setAffect(AFFECT_IDLE_BREATHE, now);
            }
            return;
        }

        // Temporal Decay and State Transitions
        uint32_t dwellTime = now - stateEntryTime;
        uint32_t idleDwellTime = now - lastStimulusTime;

        if (currentAffect == AFFECT_STARTLE_RECOIL) {
            if (dwellTime > 1200) {
                // Recover from fright into hyper-vigilant attention
                setAffect(AFFECT_ATTENTIVE_LISTEN, now);
            }
        } else if (currentAffect == AFFECT_EMPATHY_PURR) {
            if (!sensors.headPetted && dwellTime > 2000) {
                // Linger in pleased curiosity after petting stops
                setAffect(AFFECT_CURIOSITY_PONDER, now);
            }
        } else if (currentAffect == AFFECT_ATTENTIVE_LISTEN) {
            if (dwellTime > 4000) {
                setAffect(AFFECT_CURIOSITY_PONDER, now);
            }
        } else if (currentAffect == AFFECT_CURIOSITY_PONDER) {
            if (dwellTime > 5000) {
                setAffect(AFFECT_IDLE_BREATHE, now);
            }
        } else if (currentAffect == AFFECT_IDLE_BREATHE) {
            // After 45 seconds of absolute silence and zero touch, enter sleep
            if (idleDwellTime > 45000) {
                setAffect(AFFECT_DEEP_SLEEP, now);
            }
        }
    }

    void updateActuators(AuraKinematics &kinematics,
                         AuraEyeEngine &eyeEngine,
                         AuraHaloLEDs &haloLEDs,
                         uint32_t now) {
        // Continuous smooth convergence of valence and arousal coordinates
        currentCoords.valence += (targetCoords.valence - currentCoords.valence) * 0.08f;
        currentCoords.arousal += (targetCoords.arousal - currentCoords.arousal) * 0.08f;

        // Actuation Synthesis based on Current State
        switch (currentAffect) {
            case AFFECT_IDLE_BREATHE: {
                breathPhase += 0.035f; // ~0.28 Hz natural respiratory cycle
                if (breathPhase > 2.0f * PI) breathPhase -= 2.0f * PI;

                float breathPitch = sinf(breathPhase) * 6.5f;
                float driftPan    = sinf(breathPhase * 0.45f) * 12.0f;
                targetPanDeg = driftPan;
                targetPitchDeg = breathPitch;
                targetRollDeg = 0.0f;

                kinematics.setTargetPose(targetPanDeg, targetPitchDeg, targetRollDeg, trajectoryDurationSec);
                eyeEngine.setExpression(EXPR_NORMAL);
                eyeEngine.setGaze(sinf(breathPhase * 0.3f) * 14.0f, 0.0f);
                haloLEDs.setEffect(HALO_IDLE_CYAN_BREATHE);
                break;
            }

            case AFFECT_ATTENTIVE_LISTEN: {
                targetPanDeg = 0.0f;
                targetPitchDeg = 8.0f;   // Chin slightly elevated
                targetRollDeg  = -12.0f; // Expressive listening head cock
                kinematics.setTargetPose(targetPanDeg, targetPitchDeg, targetRollDeg, trajectoryDurationSec);

                eyeEngine.setExpression(EXPR_ALERT);
                eyeEngine.setGaze(0.0f, 0.0f);
                haloLEDs.setEffect(HALO_ALERT_ELECTRIC_BLUE);
                break;
            }

            case AFFECT_CURIOSITY_PONDER: {
                targetPanDeg = sinf((float)now * 0.001f) * 18.0f;
                targetPitchDeg = 10.0f;
                targetRollDeg  = 16.0f; // Opposite head tilt
                kinematics.setTargetPose(targetPanDeg, targetPitchDeg, targetRollDeg, trajectoryDurationSec);

                eyeEngine.setExpression(EXPR_CURIOUS);
                haloLEDs.setEffect(HALO_ALERT_ELECTRIC_BLUE);
                break;
            }

            case AFFECT_EMPATHY_PURR: {
                // 25 Hz micro-vibration purr generated via high-frequency servo dither
                if (now - lastPurrToggleTime >= 20) { // 25 Hz half-period (20ms)
                    purrPhase = !purrPhase;
                    lastPurrToggleTime = now;
                }
                float purrOffset = purrPhase ? 1.6f : -1.6f;

                targetPanDeg = 0.0f;
                targetPitchDeg = -9.0f + purrOffset; // Nuzzling back into the hand
                targetRollDeg  = purrOffset * 0.7f;
                kinematics.setTargetPose(targetPanDeg, targetPitchDeg, targetRollDeg, 0.08f);

                eyeEngine.setExpression(EXPR_HAPPY);
                haloLEDs.setEffect(HALO_PET_WARM_AMBER);
                break;
            }

            case AFFECT_STARTLE_RECOIL: {
                targetPanDeg = 0.0f;
                targetPitchDeg = 24.0f; // Pull skull back sharply
                targetRollDeg  = 0.0f;
                kinematics.setTargetPose(targetPanDeg, targetPitchDeg, targetRollDeg, trajectoryDurationSec);

                eyeEngine.setExpression(EXPR_ALERT);
                haloLEDs.setEffect(HALO_SURPRISE_WHITE_FLASH);
                break;
            }

            case AFFECT_DEEP_SLEEP: {
                targetPanDeg = 0.0f;
                targetPitchDeg = -18.0f; // Head resting down
                targetRollDeg  = 0.0f;
                kinematics.setTargetPose(targetPanDeg, targetPitchDeg, targetRollDeg, trajectoryDurationSec);

                eyeEngine.setExpression(EXPR_SLEEPY);
                haloLEDs.setEffect(HALO_SLEEPY_DIM_VIOLET);
                break;
            }
        }
    }

    AffectState getCurrentAffect() const { return currentAffect; }
    AffectVector getCoords() const { return currentCoords; }
    float getTargetPitch() const { return targetPitchDeg; }
    float getTargetRoll() const { return targetRollDeg; }
};

#endif // AURA_EMOTION_ENGINE_H
