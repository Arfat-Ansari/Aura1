/*
 * gesture_engine.h - AURA 1 Optical Gesture Recognition & Multi-Zone ToF
 * Arfat Ansari | Nov 2026
 *
 * uses the ST VL53L1X time-of-flight laser sensor multi-zone ROI capability
 * by alternating between left (SPAD 199) and right (SPAD 231) 8x16 optical zones at 50Hz:
 * - Detects horizontal hand swipe gestures (left-to-right, right-to-left)
 * - Detects hand push/pull movements
 * - Detects interactive peek-a-boo: covering the eyes triggers peekaboo hide,
 *   and uncovering triggers celebratory surprise animation!
 */

#ifndef AURA_GESTURE_ENGINE_H
#define AURA_GESTURE_ENGINE_H

#include <Arduino.h>
#include <math.h>

enum OpticalGestureType {
    GESTURE_NONE = 0,
    GESTURE_SWIPE_LEFT,     // hand swiped right-to-left
    GESTURE_SWIPE_RIGHT,    // hand swiped left-to-right
    GESTURE_PUSH_FORWARD,   // hand pushed close rapidly
    GESTURE_PULL_BACK,      // hand retreated rapidly
    GESTURE_PEEKABOO_COVER, // hand covering face (< 60mm)
    GESTURE_PEEKABOO_REVEAL // hand suddenly pulled away after cover
};

enum PeekabooState {
    PEEKABOO_IDLE,
    PEEKABOO_WAITING_COVER,
    PEEKABOO_COVERED,
    PEEKABOO_REVEALED
};

struct GestureEvent {
    OpticalGestureType type;
    uint32_t           timestamp;
    int16_t            speedMmPerSec;
    uint16_t           minDistanceMm;
    float              panReactionDeg;
};

class AuraGestureEngine {
private:
    // multi-zone optical thresholds
    static const uint16_t GESTURE_ACTIVE_DIST_MM  = 380; // max interaction distance
    static const uint16_t GESTURE_PEEKABOO_MAX_MM = 70;  // close cover threshold
    static const uint16_t GESTURE_HYSTERESIS_MM   = 25;

    // zone tracking
    bool     activeZoneLeft;     // true if current reading is Left zone
    uint16_t distLeftMm;
    uint16_t distRightMm;
    uint32_t leftTriggerTime;
    uint32_t rightTriggerTime;
    bool     leftInZone;
    bool     rightInZone;

    // peek-a-boo state machine
    PeekabooState peekState;
    uint32_t      coverStartTime;
    uint32_t      revealTime;

    // active gesture output
    GestureEvent lastEvent;
    float        gesturePanOffset;
    float        gesturePitchOffset;
    uint32_t     gestureAnimationStart;

public:
    AuraGestureEngine() :
        activeZoneLeft(true),
        distLeftMm(2000),
        distRightMm(2000),
        leftTriggerTime(0),
        rightTriggerTime(0),
        leftInZone(false),
        rightInZone(false),
        peekState(PEEKABOO_IDLE),
        coverStartTime(0),
        revealTime(0),
        gesturePanOffset(0.0f),
        gesturePitchOffset(0.0f),
        gestureAnimationStart(0) {
        lastEvent.type = GESTURE_NONE;
        lastEvent.timestamp = 0;
        lastEvent.speedMmPerSec = 0;
        lastEvent.minDistanceMm = 2000;
        lastEvent.panReactionDeg = 0.0f;
    }

    void init() {
        activeZoneLeft = true;
        distLeftMm = 2000;
        distRightMm = 2000;
        peekState = PEEKABOO_IDLE;
    }

    // feed alternating ToF measurements from the 50Hz sensor loop
    void updateZoneReading(bool isLeft, uint16_t distanceMm, uint32_t now) {
        if (isLeft) {
            distLeftMm = distanceMm;
        } else {
            distRightMm = distanceMm;
        }

        // 1. check for peek-a-boo (both zones covered very closely)
        bool bothCovered = (distLeftMm < GESTURE_PEEKABOO_MAX_MM) && 
                           (distRightMm < GESTURE_PEEKABOO_MAX_MM);

        if (bothCovered) {
            if (peekState == PEEKABOO_IDLE || peekState == PEEKABOO_WAITING_COVER) {
                if (coverStartTime == 0) coverStartTime = now;
                if (now - coverStartTime > 250) { // held covered for 250ms
                    peekState = PEEKABOO_COVERED;
                    triggerGesture(GESTURE_PEEKABOO_COVER, now, 0, (distLeftMm + distRightMm) / 2, 0.0f);
                }
            }
            return;
        } else {
            // hand was covering and is now removed
            if (peekState == PEEKABOO_COVERED) {
                peekState = PEEKABOO_REVEALED;
                revealTime = now;
                triggerGesture(GESTURE_PEEKABOO_REVEAL, now, 600, (distLeftMm + distRightMm) / 2, 0.0f);
                coverStartTime = 0;
                return;
            } else if (peekState == PEEKABOO_REVEALED) {
                if (now - revealTime > 1200) {
                    peekState = PEEKABOO_IDLE;
                }
            }
            coverStartTime = 0;
        }

        // 2. optical swipe detection across zones
        bool nowLeftActive  = (distLeftMm < GESTURE_ACTIVE_DIST_MM);
        bool nowRightActive = (distRightMm < GESTURE_ACTIVE_DIST_MM);

        // left edge triggered
        if (nowLeftActive && !leftInZone) {
            leftTriggerTime = now;
            leftInZone = true;
        } else if (!nowLeftActive) {
            leftInZone = false;
        }

        // right edge triggered
        if (nowRightActive && !rightInZone) {
            rightTriggerTime = now;
            rightInZone = true;
        } else if (!nowRightActive) {
            rightInZone = false;
        }

        // evaluate swipe timing window (40ms to 450ms between zones)
        if (leftInZone && rightInZone) {
            int32_t deltaT = (int32_t)rightTriggerTime - (int32_t)leftTriggerTime;
            if (deltaT > 40 && deltaT < 450) {
                // left triggered first -> swiping towards right
                triggerGesture(GESTURE_SWIPE_RIGHT, now, (int16_t)(80000 / deltaT), distRightMm, +18.0f);
                leftTriggerTime = 0;
                rightTriggerTime = 0;
            } else if (deltaT < -40 && deltaT > -450) {
                // right triggered first -> swiping towards left
                triggerGesture(GESTURE_SWIPE_LEFT, now, (int16_t)(80000 / (-deltaT)), distLeftMm, -18.0f);
                leftTriggerTime = 0;
                rightTriggerTime = 0;
            }
        }

        // decay gesture reaction smoothly
        if (now - gestureAnimationStart < 700) {
            float progress = (float)(now - gestureAnimationStart) / 700.0f;
            float decay = 1.0f - sinf(progress * (float)M_PI * 0.5f);
            gesturePanOffset *= decay;
        } else {
            gesturePanOffset = 0.0f;
            gesturePitchOffset = 0.0f;
        }
    }

    void triggerGesture(OpticalGestureType type, uint32_t now, int16_t speed, uint16_t dist, float panDeg) {
        lastEvent.type = type;
        lastEvent.timestamp = now;
        lastEvent.speedMmPerSec = speed;
        lastEvent.minDistanceMm = dist;
        lastEvent.panReactionDeg = panDeg;
        gesturePanOffset = panDeg;
        gestureAnimationStart = now;

        if (type == GESTURE_PEEKABOO_COVER) {
            gesturePitchOffset = -8.0f; // shy duck
        } else if (type == GESTURE_PEEKABOO_REVEAL) {
            gesturePitchOffset = +10.0f; // excited lift
        }
    }

    OpticalGestureType getActiveGesture() const { return lastEvent.type; }
    const GestureEvent& getLastEvent() const { return lastEvent; }
    bool isPeekabooCovered() const { return (peekState == PEEKABOO_COVERED); }
    bool isPeekabooRevealed() const { return (peekState == PEEKABOO_REVEALED); }
    float getGesturePanOffset() const { return gesturePanOffset; }
    float getGesturePitchOffset() const { return gesturePitchOffset; }

    void clearEvent() {
        lastEvent.type = GESTURE_NONE;
    }
};

#endif // AURA_GESTURE_ENGINE_H
