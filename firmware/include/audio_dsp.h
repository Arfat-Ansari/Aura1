/*
 * audio_dsp.h - AURA 1 Spatial Hearing & Sound Localization
 * Arfat Ansari | Nov 2026
 *
 * dual INMP441 I2S MEMS digital microphones for directional hearing:
 * - Left mic: L/R pin pulled to GND (left channel)
 * - Right mic: L/R pin pulled to 3.3V (right channel)
 * - Shared BCLK (GPIO 40) and WS (GPIO 38), multiplexed SD (GPIO 39)
 *
 * calculates Interaural Level Difference (ILD) between ears to estimate
 * where sound is coming from (-90 deg left to +90 deg right)
 * includes a deadband filter so neck servo motor buzzing doesn't make the head jitter
 */

#ifndef AURA_AUDIO_DSP_H
#define AURA_AUDIO_DSP_H

#include <Arduino.h>
#include <math.h>

// I2S digital microphone pins on ESP32-S3
#define PIN_MIC_WS          38  // word select (L/R clock)
#define PIN_MIC_SD          39  // serial data in
#define PIN_MIC_SCK         40  // bit clock

#define AUDIO_BUFFER_SIZE   128
#define SOUND_DEADBAND_ILD  0.14f   // ignore tiny differences to prevent head jitter
#define NOISE_FLOOR_DECAY   0.995f  // adaptive background noise tracker

enum SoundType {
    SOUND_NONE,
    SOUND_VOICE_LEFT,
    SOUND_VOICE_CENTER,
    SOUND_VOICE_RIGHT,
    SOUND_SHARP_CLAP
};

struct SpatialHearingEvent {
    bool      detected;
    float     azimuthDeg;   // -90 deg (left) to +90 deg (right)
    float     energyLeft;
    float     energyRight;
    float     totalDb;
    SoundType type;
    uint32_t  timestamp;
};

class AuraAudioDSP {
private:
    float noiseFloorDb;
    float currentLevelDb;
    float smoothedAzimuth;
    float targetAzimuth;
    float energyL;
    float energyR;
    uint32_t lastEventTime;
    uint32_t lastClapTime;
    SpatialHearingEvent lastEvent;

public:
    AuraAudioDSP() :
        noiseFloorDb(36.0f),
        currentLevelDb(36.0f),
        smoothedAzimuth(0.0f),
        targetAzimuth(0.0f),
        energyL(0.0f),
        energyR(0.0f),
        lastEventTime(0),
        lastClapTime(0) {
        lastEvent = {false, 0.0f, 0.0f, 0.0f, 36.0f, SOUND_NONE, 0};
    }

    void init() {
        pinMode(PIN_MIC_WS, OUTPUT);
        pinMode(PIN_MIC_SCK, OUTPUT);
        pinMode(PIN_MIC_SD, INPUT);

        noiseFloorDb = 38.0f;
        currentLevelDb = 38.0f;
        smoothedAzimuth = 0.0f;
        targetAzimuth = 0.0f;
    }

    // process microphone audio buffers (~50Hz update)
    void update(uint32_t now) {
        // smooth background noise floor
        noiseFloorDb = noiseFloorDb * NOISE_FLOOR_DECAY + currentLevelDb * (1.0f - NOISE_FLOOR_DECAY);

        // smooth head azimuth tracking
        smoothedAzimuth += (targetAzimuth - smoothedAzimuth) * 0.18f;

        // reset old event flags after 350ms
        if (now - lastEventTime > 350) {
            lastEvent.detected = false;
            lastEvent.type = SOUND_NONE;
        }
    }

    // process stereo PCM samples from I2S DMA
    void processStereoSamples(const int16_t *stereoBuffer, size_t sampleCount, uint32_t now) {
        float sumSqL = 0.0f;
        float sumSqR = 0.0f;

        // separate interleaved L and R channels
        for (size_t i = 0; i < sampleCount; i += 2) {
            float sL = (float)stereoBuffer[i] / 32768.0f;
            float sR = (float)stereoBuffer[i + 1] / 32768.0f;
            sumSqL += sL * sL;
            sumSqR += sR * sR;
        }

        float rmsL = sqrtf(sumSqL / (sampleCount / 2));
        float rmsR = sqrtf(sumSqR / (sampleCount / 2));

        energyL = energyL * 0.7f + rmsL * 0.3f;
        energyR = energyR * 0.7f + rmsR * 0.3f;

        float totalEnergy = energyL + energyR;
        currentLevelDb = 20.0f * log10f(fmaxf(totalEnergy, 0.0001f)) + 90.0f;

        // check if sound is significantly louder than ambient noise (+12dB above floor)
        float soundDelta = currentLevelDb - noiseFloorDb;
        if (soundDelta > 12.0f && totalEnergy > 0.015f) {
            // calculate Interaural Level Difference (ILD)
            float ild = (energyR - energyL) / (totalEnergy + 0.0001f);

            // ignore tiny level differences to prevent jitter
            if (fabsf(ild) > SOUND_DEADBAND_ILD) {
                // map ILD (-1.0 to +1.0) to physical azimuth (-80 to +80 degrees)
                targetAzimuth = constrain(ild * 95.0f, -80.0f, 80.0f);
                lastEventTime = now;
                lastEvent.detected = true;
                lastEvent.azimuthDeg = targetAzimuth;
                lastEvent.energyLeft = energyL;
                lastEvent.energyRight = energyR;
                lastEvent.totalDb = currentLevelDb;
                lastEvent.timestamp = now;

                if (targetAzimuth < -20.0f) {
                    lastEvent.type = SOUND_VOICE_LEFT;
                } else if (targetAzimuth > 20.0f) {
                    lastEvent.type = SOUND_VOICE_RIGHT;
                } else {
                    lastEvent.type = SOUND_VOICE_CENTER;
                }

                // check if sudden transient like a clap (fast attack > 24dB jump)
                if (soundDelta > 24.0f && (now - lastClapTime > 300)) {
                    lastEvent.type = SOUND_SHARP_CLAP;
                    lastClapTime = now;
                }
            }
        }
    }

    // manual simulation trigger for desktop testing
    void triggerSoundDirection(float azimuthDeg, float volumeDb, uint32_t now) {
        targetAzimuth = constrain(azimuthDeg, -80.0f, 80.0f);
        lastEventTime = now;
        lastEvent.detected = true;
        lastEvent.azimuthDeg = targetAzimuth;
        lastEvent.totalDb = volumeDb;
        lastEvent.timestamp = now;

        if (targetAzimuth < -20.0f) {
            lastEvent.type = SOUND_VOICE_LEFT;
        } else if (targetAzimuth > 20.0f) {
            lastEvent.type = SOUND_VOICE_RIGHT;
        } else {
            lastEvent.type = SOUND_VOICE_CENTER;
        }
    }

    const SpatialHearingEvent& getHearingEvent() const { return lastEvent; }
    float getSmoothedAzimuth() const { return smoothedAzimuth; }
    float getSoundLevelDb() const { return currentLevelDb; }
    bool isSoundDetected() const { return lastEvent.detected; }
    bool isClap() const { return (lastEvent.type == SOUND_SHARP_CLAP); }
};

#endif // AURA_AUDIO_DSP_H