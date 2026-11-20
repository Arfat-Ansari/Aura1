/*
 * sensors.h - AURA 1 Sensor Interface
 * Arfat Ansari | Nov 2026
 *
 * handles:
 * - VL53L1X laser time-of-flight distance sensor over I2C
 * - Capacitive touch petting engine through skull shell
 * - QMI8658C 6-axis IMU (detects table bumps / impacts)
 * - INMP441 I2S microphone (detects voices and claps)
 */

#ifndef AURA_SENSORS_H
#define AURA_SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include "audio_dsp.h"
#include "petting_purr.h"
#include "gesture_engine.h"

// I2C pins on ESP32-S3
#define PIN_I2C_SDA         41
#define PIN_I2C_SCL         42

// I2C sensor addresses
#define ADDR_VL53L1X        0x29
#define ADDR_QMI8658C       0x6B
#define ADDR_AS5600         0x36

struct SensorReadings {
    uint16_t distanceMm;    // distance in mm from laser sensor
    bool visitorPresent;    // someone is within 3 meters
    bool visitorClose;      // someone is within 80cm
    bool headPetted;        // hand is touching the head
    float purrVibration;    // 25Hz servo angle flutter for purring
    uint32_t petDuration;   // how long the user has been petting
    float accelX, accelY, accelZ;
    float gyroX, gyroY, gyroZ;
    bool tableBumped;       // table bumped or tapped
    float soundLevelDb;     // mic volume level
    bool speechDetected;    // voice detected
    bool clapDetected;      // sudden clap detected
    float soundAzimuthDeg;  // -90 to +90 deg sound direction
    bool soundLocated;      // sound direction actively locked
    OpticalGestureType activeGesture; // detected hand wave or swipe
    bool peekabooCovered;   // user covering robot's eyes
    bool peekabooRevealed;  // user uncovered eyes in peekaboo game
    float gesturePanOffset; // neck pan compensation for gestures
    float gesturePitchOffset; // neck pitch compensation
};

class AuraSensors {
private:
    SensorReadings readings;
    uint32_t lastReadTime;
    AuraAudioDSP audioDsp;
    AuraPettingEngine pettingEngine;
    AuraGestureEngine gestureEngine;
    bool toggleRoiZone;

public:
    AuraSensors() : lastReadTime(0), toggleRoiZone(false) {
        memset(&readings, 0, sizeof(SensorReadings));
        readings.distanceMm = 2000;
        readings.activeGesture = GESTURE_NONE;
    }

    void init() {
        Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000); // 400kHz I2C
        audioDsp.init();
        pettingEngine.init();
        gestureEngine.init();
    }

    void update() {
        uint32_t now = millis();
        if (now - lastReadTime < 16) return; // run around 60Hz
        lastReadTime = now;

        // 1. update petting engine and purr vibration
        pettingEngine.update(now);
        readings.headPetted    = pettingEngine.isPetted();
        readings.purrVibration = pettingEngine.getPurrOffset();
        readings.petDuration   = pettingEngine.getPetDuration();

        // 2. read distance sensor and multi-zone gesture engine
        readings.visitorPresent = (readings.distanceMm < 3000);
        readings.visitorClose   = (readings.distanceMm < 800);

        // alternate ROI zones between left and right at 50Hz
        toggleRoiZone = !toggleRoiZone;
        gestureEngine.updateZoneReading(toggleRoiZone, readings.distanceMm, now);
        readings.activeGesture      = gestureEngine.getActiveGesture();
        readings.peekabooCovered    = gestureEngine.isPeekabooCovered();
        readings.peekabooRevealed   = gestureEngine.isPeekabooRevealed();
        readings.gesturePanOffset   = gestureEngine.getGesturePanOffset();
        readings.gesturePitchOffset = gestureEngine.getGesturePitchOffset();

        // 3. update audio processing
        audioDsp.update(now);
        readings.soundLevelDb    = audioDsp.getSoundLevelDb();
        readings.speechDetected  = audioDsp.isSoundDetected();
        readings.clapDetected    = audioDsp.isClap();
        readings.soundAzimuthDeg = audioDsp.getSmoothedAzimuth();
        readings.soundLocated    = audioDsp.isSoundDetected();
    }

    const SensorReadings& getReadings() const { return readings; }
    void setSimulatedDistance(uint16_t dist) { readings.distanceMm = dist; }
    AuraPettingEngine& getPettingEngine() { return pettingEngine; }
    AuraGestureEngine& getGestureEngine() { return gestureEngine; }
};

#endif // AURA_SENSORS_H
