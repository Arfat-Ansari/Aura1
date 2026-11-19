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
};

class AuraSensors {
private:
    SensorReadings readings;
    uint32_t lastReadTime;
    AuraAudioDSP audioDsp;
    AuraPettingEngine pettingEngine;

public:
    AuraSensors() : lastReadTime(0) {
        memset(&readings, 0, sizeof(SensorReadings));
        readings.distanceMm = 2000;
    }

    void init() {
        Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000); // 400kHz I2C
        audioDsp.init();
        pettingEngine.init();
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

        // 2. read distance sensor
        readings.visitorPresent = (readings.distanceMm < 3000);
        readings.visitorClose   = (readings.distanceMm < 800);

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
};

#endif // AURA_SENSORS_H
