/*
 * emotion_engine.h - AURA 1 Behavior and Reaction System
 * Arfat Ansari | Nov 2026
 *
 * simple state machine so the robot behaves like a real companion pet
 * reacts to:
 * - petting (touch sensor on top of head -> happy eyes + warm amber pulse + purr)
 * - close objects (laser distance sensor -> curious head tilt + looks at you)
 * - loud noises or table taps (mic / bump -> alert startled reaction)
 * - idle timeout (goes to sleep after 45 seconds of no interaction)
 */

#ifndef AURA_EMOTION_ENGINE_H
#define AURA_EMOTION_ENGINE_H

#include <Arduino.h>
#include <math.h>
#include "kinematics.h"
#include "display_eyes.h"
#include "halo_leds.h"
#include "sensors.h"

enum RobotMood {
    MOOD_IDLE,       // breathing motion, glances around occasionally
    MOOD_LISTENING,  // hears sound, tilts head up to listen
    MOOD_CURIOUS,    // sees something nearby, cocks head left/right
    MOOD_HAPPY,      // getting petted on the head, eyes squint, purrs
    MOOD_STARTLED,   // sudden loud sound or table bump, pulls head back
    MOOD_SLEEPING    // nobody around, eyes shut, dim LEDs
};

class AuraEmotionEngine {
private:
    RobotMood currentMood;
    uint32_t stateStartTime;
    uint32_t lastInteractionTime;
    uint32_t lastPurrToggleTime;
    bool     purrPhase;
    float    breathPhase;

    // target angles for head servos (degrees)
    float targetPan;
    float targetPitch;
    float targetRoll;
    float moveDuration;

public:
    AuraEmotionEngine() :
        currentMood(MOOD_IDLE),
        stateStartTime(0),
        lastInteractionTime(0),
        lastPurrToggleTime(0),
        purrPhase(false),
        breathPhase(0.0f),
        targetPan(0.0f),
        targetPitch(0.0f),
        targetRoll(0.0f),
        moveDuration(0.6f) {}

    void init(uint32_t now) {
        stateStartTime = now;
        lastInteractionTime = now;
        setMood(MOOD_IDLE, now);
    }

    void setMood(RobotMood newMood, uint32_t now) {
        currentMood = newMood;
        stateStartTime = now;

        switch (newMood) {
            case MOOD_IDLE:
                moveDuration = 0.85f;
                break;
            case MOOD_LISTENING:
                moveDuration = 0.35f;
                break;
            case MOOD_CURIOUS:
                moveDuration = 0.50f;
                break;
            case MOOD_HAPPY:
                moveDuration = 0.40f;
                break;
            case MOOD_STARTLED:
                moveDuration = 0.16f; // quick jump back
                break;
            case MOOD_SLEEPING:
                moveDuration = 1.40f; // slow relaxed drop
                break;
        }
    }

    // check sensors and decide if mood should change
    void processPerception(const SensorReadings &sensors, uint32_t now) {
        // 1. loud tap or bump gets immediate attention
        if (sensors.tableBumped || sensors.clapDetected) {
            lastInteractionTime = now;
            setMood(MOOD_STARTLED, now);
            return;
        }

        // 2. petting head takes priority over curiosity
        if (sensors.headPetted) {
            lastInteractionTime = now;
            if (currentMood != MOOD_HAPPY) {
                setMood(MOOD_HAPPY, now);
            }
            return;
        }

        // 3. someone talking
        if (sensors.speechDetected) {
            lastInteractionTime = now;
            if (currentMood != MOOD_LISTENING && currentMood != MOOD_HAPPY) {
                setMood(MOOD_LISTENING, now);
            }
            return;
        }

        // 4. laser distance sensor sees someone close (under 800mm)
        if (sensors.visitorClose) {
            lastInteractionTime = now;
            if (currentMood == MOOD_IDLE || currentMood == MOOD_SLEEPING) {
                setMood(MOOD_CURIOUS, now);
            }
            return;
        }

        if (sensors.visitorPresent) {
            lastInteractionTime = now;
            if (currentMood == MOOD_SLEEPING) {
                setMood(MOOD_IDLE, now);
            }
            return;
        }

        // timers to fall back to idle or sleep
        uint32_t timeInState = now - stateStartTime;
        uint32_t timeSinceInteraction = now - lastInteractionTime;

        if (currentMood == MOOD_STARTLED) {
            if (timeInState > 1200) {
                setMood(MOOD_LISTENING, now);
            }
        } else if (currentMood == MOOD_HAPPY) {
            if (!sensors.headPetted && timeInState > 2000) {
                setMood(MOOD_CURIOUS, now);
            }
        } else if (currentMood == MOOD_LISTENING) {
            if (timeInState > 4000) {
                setMood(MOOD_CURIOUS, now);
            }
        } else if (currentMood == MOOD_CURIOUS) {
            if (timeInState > 5000) {
                setMood(MOOD_IDLE, now);
            }
        } else if (currentMood == MOOD_IDLE) {
            // go to sleep after 45s of nothing happening
            if (timeSinceInteraction > 45000) {
                setMood(MOOD_SLEEPING, now);
            }
        }
    }

    // update head servos, eyes, and LEDs based on active mood
    void updateActuators(AuraKinematics &kinematics,
                         AuraEyeEngine &eyeEngine,
                         AuraHaloLEDs &haloLEDs,
                         uint32_t now) {
        switch (currentMood) {
            case MOOD_IDLE: {
                // gentle breathing sway
                breathPhase += 0.035f;
                if (breathPhase > 2.0f * PI) breathPhase -= 2.0f * PI;

                targetPan = sinf(breathPhase * 0.45f) * 12.0f;
                targetPitch = sinf(breathPhase) * 6.5f;
                targetRoll = 0.0f;

                kinematics.setTargetPose(targetPan, targetPitch, targetRoll, moveDuration);
                eyeEngine.setExpression(EXPR_NORMAL);
                eyeEngine.setGaze(sinf(breathPhase * 0.3f) * 14.0f, 0.0f);
                haloLEDs.setEffect(HALO_IDLE_CYAN_BREATHE);
                break;
            }

            case MOOD_LISTENING: {
                // chin tilted up slightly and head cocked to listen
                targetPan = 0.0f;
                targetPitch = 8.0f;
                targetRoll = -12.0f;
                kinematics.setTargetPose(targetPan, targetPitch, targetRoll, moveDuration);

                eyeEngine.setExpression(EXPR_ALERT);
                eyeEngine.setGaze(0.0f, 0.0f);
                haloLEDs.setEffect(HALO_ALERT_ELECTRIC_BLUE);
                break;
            }

            case MOOD_CURIOUS: {
                // curious tilt side to side
                targetPan = sinf((float)now * 0.001f) * 18.0f;
                targetPitch = 10.0f;
                targetRoll = 16.0f;
                kinematics.setTargetPose(targetPan, targetPitch, targetRoll, moveDuration);

                eyeEngine.setExpression(EXPR_CURIOUS);
                haloLEDs.setEffect(HALO_ALERT_ELECTRIC_BLUE);
                break;
            }

            case MOOD_HAPPY: {
                // purr vibration: rapid tiny servo flutter (25 Hz)
                if (now - lastPurrToggleTime >= 20) {
                    purrPhase = !purrPhase;
                    lastPurrToggleTime = now;
                }
                float purr = purrPhase ? 1.6f : -1.6f;

                targetPan = 0.0f;
                targetPitch = -9.0f + purr;
                targetRoll = purr * 0.7f;
                kinematics.setTargetPose(targetPan, targetPitch, targetRoll, 0.08f);

                eyeEngine.setExpression(EXPR_HAPPY);
                haloLEDs.setEffect(HALO_PET_WARM_AMBER);
                break;
            }

            case MOOD_STARTLED: {
                // pull head back fast
                targetPan = 0.0f;
                targetPitch = 24.0f;
                targetRoll = 0.0f;
                kinematics.setTargetPose(targetPan, targetPitch, targetRoll, moveDuration);

                eyeEngine.setExpression(EXPR_ALERT);
                haloLEDs.setEffect(HALO_ALERT_ELECTRIC_BLUE);
                break;
            }

            case MOOD_SLEEPING: {
                // head rests down, eyes closed, LEDs dim
                targetPan = 0.0f;
                targetPitch = -18.0f;
                targetRoll = 5.0f;
                kinematics.setTargetPose(targetPan, targetPitch, targetRoll, moveDuration);

                eyeEngine.setExpression(EXPR_SLEEPY);
                haloLEDs.setEffect(HALO_IDLE_CYAN_BREATHE);
                break;
            }
        }
    }

    RobotMood getMood() const { return currentMood; }
};

#endif // AURA_EMOTION_ENGINE_H
