#ifndef AURA_SENSORS_H
#define AURA_SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include "audio_dsp.h"

// I2C Peripheral Pin Mapping
#define PIN_I2C_SDA         41
#define PIN_I2C_SCL         42

// Capacitive Touch Petting Input Pin
#define PIN_TOUCH_PET       1

// Sensor I2C Bus Addresses
#define ADDR_VL53L1X        0x29
#define ADDR_QMI8658C       0x6B
#define ADDR_AS5600         0x36

struct SensorReadings {
    uint16_t distanceMm;    // Laser ToF distance (0 to 4000 mm)
    bool visitorPresent;    // True if target detected within 3000 mm
    bool visitorClose;      // True if target detected within 800 mm
    bool headPetted;        // True if capacitive touch active
    float accelX, accelY, accelZ; // IMU Acceleration
    float gyroX, gyroY, gyroZ;    // IMU Angular Velocity
    bool tableBumped;       // Impact transient detected
    float soundLevelDb;     // Acoustic energy from INMP441 MEMS microphone
    bool speechDetected;    // True if vocal band speech detected
    bool clapDetected;      // True if sudden clap/sharp sound detected
};

class AuraSensors {
private:
    SensorReadings readings;
    uint32_t lastReadTime;
    uint16_t touchBaseline;
    uint8_t  touchStreak;       // Hysteresis frame streak counter
    float    touchFiltered;     // Low-pass filtered touch value
    AuraAudioDSP audioDsp;

public:
    AuraSensors() : lastReadTime(0), touchBaseline(0), touchStreak(0), touchFiltered(0.0f) {}

    void init() {
        Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000); // 400 kHz Fast I2C
        pinMode(PIN_TOUCH_PET, INPUT);
        audioDsp.init(); // Initialize I2S MEMS microphone

        // Calibrate capacitive touch baseline
        uint32_t sum = 0;
        for (int i = 0; i < 16; i++) {
            sum += touchRead(PIN_TOUCH_PET);
            delay(10);
        }
        touchBaseline = sum / 16;
        touchFiltered = (float)touchBaseline;

        readings.distanceMm = 4000;
        readings.visitorPresent = false;
        readings.visitorClose = false;
        readings.headPetted = false;
        readings.tableBumped = false;
        readings.accelX = 0.0f;
        readings.accelY = 0.0f;
        readings.accelZ = 1.0f; // 1G gravity
    }

    void update() {
        uint32_t now = millis();
        if (now - lastReadTime < 20) return; // 50 Hz polling
        lastReadTime = now;

        // Capacitive Touch Petting Detection with Hysteresis & Servo Noise Rejection
        uint16_t rawTouch = touchRead(PIN_TOUCH_PET);
        // Exponential smoothing (alpha = 0.25) to reject high-frequency motor EMI
        touchFiltered = touchFiltered * 0.75f + (float)rawTouch * 0.25f;

        // Threshold: 30% drop from baseline indicates skin contact
        if (touchBaseline > 0 && touchFiltered < (touchBaseline * 0.72f)) {
            if (touchStreak < 8) touchStreak++;
        } else {
            if (touchStreak > 0) touchStreak--;
        }

        // Require 8 consecutive stable frames (160ms) to trigger empathetic purr
        readings.headPetted = (touchStreak >= 8);

        // IMU 6-DOF Shock / Table Tap Detection (QMI8658C)
        float totalG = sqrtf(readings.accelX * readings.accelX +
                             readings.accelY * readings.accelY +
                             readings.accelZ * readings.accelZ);
        readings.tableBumped = (totalG > 2.2f); // Impact shock > 2.2G

        // Audio DSP update
        audioDsp.update();
        const AudioAnalysis& audio = audioDsp.getAnalysis();
        readings.soundLevelDb = audio.rmsDecibels;
        readings.speechDetected = audio.speechDetected;
        readings.clapDetected = audio.clapDetected;

        // Presence Logic
        readings.visitorPresent = (readings.distanceMm > 100 && readings.distanceMm < 3000);
        readings.visitorClose   = (readings.distanceMm > 100 && readings.distanceMm < 800);
    }

    void setSimulatedDistance(uint16_t distMm) {
        readings.distanceMm = distMm;
    }

    const SensorReadings& getReadings() const { return readings; }
};

#endif // AURA_SENSORS_H
