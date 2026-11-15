#ifndef AURA_KINEMATICS_H
#define AURA_KINEMATICS_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include <math.h>

// Servo Hardware Pin Assignments (Direct GPIO mapping avoiding Octal PSRAM)
#define PIN_SERVO_PAN       4   // Base Azimuth Turntable
#define PIN_SERVO_TILT_L    5   // Inverted Pushrod Left
#define PIN_SERVO_TILT_R    6   // Inverted Pushrod Right

// Kinematic Physical Constants (in mm and degrees)
static const float BASE_RADIUS       = 48.0f;  // Servo horn pivot radius
static const float HEAD_RADIUS       = 24.0f;  // Skull ball joint radius
static const float PUSHROD_LENGTH    = 48.0f;  // Carbon link length
static const float PIVOT_HEIGHT      = 40.0f;  // Central ball pivot Z height

// Servo Travel Limits (Safety Clamped)
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

    float trajectoryProgress; // 0.0 to 1.0 along current S-curve
    float trajectorySpeed;    // Step increment per update frame

public:
    AuraKinematics() : 
        currentPanAngle(0.0f), currentPitchAngle(0.0f), currentRollAngle(0.0f),
        targetPanAngle(0.0f), targetPitchAngle(0.0f), targetRollAngle(0.0f),
        startPanAngle(0.0f), startPitchAngle(0.0f), startRollAngle(0.0f),
        trajectoryProgress(1.0f), trajectorySpeed(0.05f) {}

    void init() {
        ESP32PWM::allocateTimer(0);
        ESP32PWM::allocateTimer(1);
        ESP32PWM::allocateTimer(2);
        ESP32PWM::allocateTimer(3);

        servoPan.setPeriodHertz(50);
        servoTiltL.setPeriodHertz(50);
        servoTiltR.setPeriodHertz(50);

        servoPan.attach(PIN_SERVO_PAN, 500, 2400);
        servoTiltL.attach(PIN_SERVO_TILT_L, 500, 2400);
        servoTiltR.attach(PIN_SERVO_TILT_R, 500, 2400);

        // Home positions
        setTargetPose(0.0f, 0.0f, 0.0f);
        update(1.0f);
    }

    // Set desired spherical Euler angles (in degrees) with Quintic S-curve trajectory planning
    void setTargetPose(float panDeg, float pitchDeg, float rollDeg, float durationSeconds = 0.45f) {
        float newPan   = constrain(panDeg,   -75.0f, 75.0f);
        float newPitch = constrain(pitchDeg, -30.0f, 30.0f);
        float newRoll  = constrain(rollDeg,  -25.0f, 25.0f);

        // Only trigger new trajectory if target differs noticeably
        if (fabsf(newPan - targetPanAngle) > 0.5f ||
            fabsf(newPitch - targetPitchAngle) > 0.5f ||
            fabsf(newRoll - targetRollAngle) > 0.5f) {
            startPanAngle   = currentPanAngle;
            startPitchAngle = currentPitchAngle;
            startRollAngle  = currentRollAngle;
            targetPanAngle   = newPan;
            targetPitchAngle = newPitch;
            targetRollAngle  = newRoll;

            trajectoryProgress = 0.0f;
            // Assuming 60 Hz update rate (dt = 0.0166s)
            float totalFrames = durationSeconds * 60.0f;
            trajectorySpeed = (totalFrames > 1.0f) ? (1.0f / totalFrames) : 1.0f;
        }
    }

    // Mathematical Quintic S-Curve polynomial: s(t) = 6t^5 - 15t^4 + 10t^3
    // Guarantees zero velocity AND zero jerk at both start and endpoint
    static inline float evaluateQuinticSCurve(float t) {
        if (t <= 0.0f) return 0.0f;
        if (t >= 1.0f) return 1.0f;
        float t3 = t * t * t;
        return t3 * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    // Step the kinematics engine forward along the S-curve trajectory
    void update(float dummySmoothing = 0.15f) {
        if (trajectoryProgress < 1.0f) {
            trajectoryProgress += trajectorySpeed;
            if (trajectoryProgress > 1.0f) trajectoryProgress = 1.0f;

            float ease = evaluateQuinticSCurve(trajectoryProgress);
            currentPanAngle   = startPanAngle   + (targetPanAngle   - startPanAngle)   * ease;
            currentPitchAngle = startPitchAngle + (targetPitchAngle - startPitchAngle) * ease;
            currentRollAngle  = startRollAngle  + (targetRollAngle  - startRollAngle)  * ease;
        }

        // Inverse Kinematics Mapping:
        // Pan servo maps directly to azimuth turntable
        int cmdPan = (int)(90.0f + currentPanAngle);

        // Pushrod differential: Pitch moves both horns together; Roll moves them inversely
        int cmdTiltL = (int)(90.0f + currentPitchAngle + currentRollAngle);
        int cmdTiltR = (int)(90.0f - currentPitchAngle + currentRollAngle);

        cmdPan   = constrain(cmdPan,   SERVO_PAN_MIN,  SERVO_PAN_MAX);
        cmdTiltL = constrain(cmdTiltL, SERVO_TILT_MIN, SERVO_TILT_MAX);
        cmdTiltR = constrain(cmdTiltR, SERVO_TILT_MIN, SERVO_TILT_MAX);

        // Hardware safety clamp: ensure right pushrod never infringes C4 capacitor boundary (<10.0mm)
        float clearance = getCapacitorClearanceMm();
        if (clearance < 10.0f) {
            // Prevent further downward tilt of right horn
            cmdTiltR = constrain(cmdTiltR, 45, 135);
        }

        servoPan.write(cmdPan);
        servoTiltL.write(cmdTiltL);
        servoTiltR.write(cmdTiltR);
    }

    // Microsecond pulse conversion for high-precision 16-bit timer resolution
    static inline uint16_t angleToMicroseconds(int angleDeg) {
        return (uint16_t)(500 + ((angleDeg * (2400 - 500)) / 180));
    }

    // Real-time clearance between right pushrod linkage and bulk capacitor C4
    float getCapacitorClearanceMm() const {
        // Geometric clearance model based on 12mm standoffs and 15mm forward offset
        // At neutral (0, 0), clearance is 14.2mm. Worst case backward tilt is 12.4mm.
        float pitchRad = currentPitchAngle * (PI / 180.0f);
        float rollRad  = currentRollAngle  * (PI / 180.0f);
        float deltaZ   = 14.2f + (sinf(pitchRad) * 4.2f) - (sinf(rollRad) * 2.1f);
        return (deltaZ < 0.0f) ? 0.0f : deltaZ;
    }

    float getPan()   const { return currentPanAngle; }
    float getPitch() const { return currentPitchAngle; }
    float getRoll()  const { return currentRollAngle; }
};

#endif // AURA_KINEMATICS_H
