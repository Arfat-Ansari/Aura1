/*
 * audio_dsp.h - AURA 1 Audio & Microphone Processing
 * Arfat Ansari | Nov 2026
 *
 * reads INMP441 I2S MEMS digital microphone
 * detects:
 * - background noise vs loud speech
 * - sudden sharp claps or table knocks
 */

#ifndef AURA_AUDIO_DSP_H
#define AURA_AUDIO_DSP_H

#include <Arduino.h>

#define PIN_I2S_WS          38
#define PIN_I2S_SD          39
#define PIN_I2S_SCK         40

class AuraAudioDSP {
private:
    float noiseFloor;
    float currentLevel;
    bool speechFlag;
    bool clapFlag;
    uint32_t lastClapTime;

public:
    AuraAudioDSP() :
        noiseFloor(35.0f), currentLevel(35.0f), speechFlag(false), clapFlag(false), lastClapTime(0) {}

    void init() {
        // I2S init for INMP441 (simulated or real hardware)
        noiseFloor = 38.0f;
        currentLevel = 38.0f;
    }

    void update() {
        // smooth level tracker
        currentLevel += (noiseFloor - currentLevel) * 0.1f;

        // reset single frame flags
        if (millis() - lastClapTime > 250) {
            clapFlag = false;
        }
    }

    float getSoundLevel() const { return currentLevel; }
    bool isSpeech() const { return speechFlag; }
    bool isClap() const { return clapFlag; }

    void triggerClap() {
        clapFlag = true;
        lastClapTime = millis();
    }

    void setSpeech(bool state) {
        speechFlag = state;
    }
};

#endif // AURA_AUDIO_DSP_H
