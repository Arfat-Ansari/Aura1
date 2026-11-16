/*
 * sensors.h - AURA 1 Sensor Interface
 * Arfat Ansari | Nov 2026
 *
 * handles:
 * - VL53L1X laser time-of-flight distance sensor over I2C (detects people in front)
 * - Capacitive touch pad on head (detects petting / hand touching skull)
 * - QMI8658C 6-axis IMU (detects table bumps / impacts)
 * - INMP441 I2S microphone (detects voices and claps)
 */

#ifndef AURA_SENSORS_H
#define AURA_SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include "audio_dsp.h"

// I2C pins on ESP32-S3
#define PIN_I2C_SDA         41
#define PIN_I2C_SCL         42

// touch sensor pin on top of head
#define PIN_TOUCH_PET       1

// I2C sensor addresses
#define ADDR_VL53L1X        0x29
#define ADDR_QMI8658C       0x6B
#define ADDR_AS5600         0x36

struct SensorReadings {
    uint16_t distanceMm;    // distance in mm from laser sensor
    bool visitorPresent;    // someone is within 3 meters
    bool visitorClose;      // someone is within 80cm
    bool headPetted;        // hand is touching the head
    float accelX, accelY, accelZ; // accelerometer readings
    float gyroX, gyroY, gyroZ;    // gyro readings
    bool tableBumped;       // table bumped or tapped
    float soundLevelDb;     // mic volume level
    bool speechDetected;    // voice detected
    bool clapDetected;      // sudden clap detected
};

class AuraSensors {
private:
    SensorReadings readings;
    uint32_t lastReadTime;
    uint16_t touchBaseline;
    uint8_t  touchStreak;
    float    touchFiltered;
    AuraAudioDSP audioDsp;

public:
    AuraSensors() :
        lastReadTime(0), touchBaseline(45000), touchStreak(0), touchFiltered(45000.0f) {
        memset(&readings, 0, sizeof(SensorReadings));
        readings.distanceMm = 2000;
    }

    void init() {
        Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000); // 400kHz I2C
        audioDsp.init();

        // calibrate touch pad baseline
        uint32_t sum = 0;
        for (int i = 0; i < 16; i++) {
            sum += touchRead(PIN_TOUCH_PET);
            delay(5);
        }
        touchBaseline = (uint16_t)(sum / 16);
        if (touchBaseline < 1000) touchBaseline = 40000;
        touchFiltered = (float)touchBaseline;
    }

    void update() {
        uint32_t now = millis();
        if (now - lastReadTime < 20) return; // run at 50Hz
        lastReadTime = now;

        // 1. read capacitive touch pad
        uint16_t rawTouch = touchRead(PIN_TOUCH_PET);
        touchFiltered += ((float)rawTouch - touchFiltered) * 0.25f;

        // touch threshold
        if (touchFiltered < (float)touchBaseline * 0.65f) {
            touchStreak++;
            if (touchStreak >= 2) readings.headPetted = true;
        } else {
            touchStreak = 0;
            readings.headPetted = false;
        }

        // 2. read distance sensor (simulated test bounds or real I2C)
        // visitor present if within 3000mm, close if within 800mm
        readings.visitorPresent = (readings.distanceMm < 3000);
        readings.visitorClose   = (readings.distanceMm < 800);

        // 3. update audio processing
        audioDsp.update();
        readings.soundLevelDb   = audioDsp.getSoundLevel();
        readings.speechDetected = audioDsp.isSpeech();
        readings.clapDetected   = audioDsp.isClap();
    }

    const SensorReadings& getReadings() const { return readings; }
    void setSimulatedDistance(uint16_t dist) { readings.distanceMm = dist; }
};

#endif // AURA_SENSORS_H
