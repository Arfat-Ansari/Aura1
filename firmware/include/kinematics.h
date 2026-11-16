/*
 * kinematics.h - AURA 1 3-DOF Neck Kinematics
 * Arfat Ansari | Nov 2026
 *
 * controls the 3 MG90S servos:
 * - Servo 1 (Pin 4): base pan (turns head left and right)
 * - Servo 2 (Pin 5): tilt pushrod left
 * - Servo 3 (Pin 6): tilt pushrod right
 *
 * differential tilt lets the head nod up/down and cock side to side
 */

#ifndef AURA_KINEMATICS_H
#define AURA_KINEMATICS_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include <math.h>

// servo pins on ESP32-S3
#define PIN_SERVO_PAN       4   // base pan
#define PIN_SERVO_TILT_L    5   // left tilt link
#define PIN_SERVO_TILT_R    6   // right tilt link

// geometry constants (in mm)
static const float BASE_RADIUS       = 48.0f;  // servo horn pivot circle
static const float HEAD_RADIUS       = 24.0f;  // skull link pivot circle
static const float PUSHROD_LENGTH    = 48.0f;  // pushrod length
static const float PIVOT_HEIGHT      = 40.0f;  // ball joint height

// safety limits so servos dont collide
static const int SERVO_PAN_MIN       = 15;
static const int SERVO_PAN_MAX       = 165;
static const int SERVO_TILT_MIN      = 30;
static const int SERVO_TILT_MAX      = 150;

class AuraKinematics {
private:
    Servo servoPan;
    Servo servoTiltL;
    Servo servoTiltR;

    float currentPanAngle;
    float currentPitchAngle;
    float currentRollAngle;

    float targetPanAngle;
    float targetPitchAngle;
    float targetRollAngle;

    float startPanAngle;
    float startPitchAngle;
    float startRollAngle;

    uint32_t moveStartTime;
    uint32_t moveDurationMs;
    bool isMoving;

public:
    AuraKinematics() :
        currentPanAngle(90.0f), currentPitchAngle(0.0f), currentRollAngle(0.0f),
        targetPanAngle(90.0f), targetPitchAngle(0.0f), targetRollAngle(0.0f),
        startPanAngle(90.0f), startPitchAngle(0.0f), startRollAngle(0.0f),
        moveStartTime(0), moveDurationMs(300), isMoving(false) {}

    void init() {
        servoPan.attach(PIN_SERVO_PAN, 500, 2500);
        servoTiltL.attach(PIN_SERVO_TILT_L, 500, 2500);
        servoTiltR.attach(PIN_SERVO_TILT_R, 500, 2500);

        servoPan.write(90);
        servoTiltL.write(90);
        servoTiltR.write(90);
    }

    // set desired head pose in degrees
    void setTargetPose(float panDeg, float pitchDeg, float rollDeg, float durationSec = 0.5f) {
        startPanAngle = currentPanAngle;
        startPitchAngle = currentPitchAngle;
        startRollAngle = currentRollAngle;

        targetPanAngle = constrain(90.0f + panDeg, (float)SERVO_PAN_MIN, (float)SERVO_PAN_MAX);
        targetPitchAngle = constrain(pitchDeg, -25.0f, 25.0f);
        targetRollAngle = constrain(rollDeg, -20.0f, 20.0f);

        moveStartTime = millis();
        moveDurationMs = (uint32_t)(durationSec * 1000.0f);
        if (moveDurationMs < 20) moveDurationMs = 20;
        isMoving = true;
    }

    // cubic easing function
    inline float easeInOutCubic(float t) {
        return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
    }

    // update servo angles along the eased curve
    void update(float dt = 0.02f) {
        if (!isMoving) return;

        uint32_t elapsed = millis() - moveStartTime;
        float progress = (float)elapsed / (float)moveDurationMs;

        if (progress >= 1.0f) {
            progress = 1.0f;
            isMoving = false;
        }

        float eased = easeInOutCubic(progress);

        currentPanAngle = startPanAngle + (targetPanAngle - startPanAngle) * eased;
        currentPitchAngle = startPitchAngle + (targetPitchAngle - startPitchAngle) * eased;
        currentRollAngle = startRollAngle + (targetRollAngle - startRollAngle) * eased;

        // calculate differential servo angles
        // left and right servos push or pull together for pitch, opposite for roll
        float leftTiltAngle  = 90.0f + currentPitchAngle + currentRollAngle;
        float rightTiltAngle = 90.0f - currentPitchAngle + currentRollAngle;

        leftTiltAngle = constrain(leftTiltAngle, (float)SERVO_TILT_MIN, (float)SERVO_TILT_MAX);
        rightTiltAngle = constrain(rightTiltAngle, (float)SERVO_TILT_MIN, (float)SERVO_TILT_MAX);

        servoPan.write((int)currentPanAngle);
        servoTiltL.write((int)leftTiltAngle);
        servoTiltR.write((int)rightTiltAngle);
    }

    float getPan() const { return currentPanAngle - 90.0f; }
    float getPitch() const { return currentPitchAngle; }
    float getRoll() const { return currentRollAngle; }
};

#endif // AURA_KINEMATICS_H
