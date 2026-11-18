/*
 * voice_synth.h - AURA 1 Procedural Expressive Voice & Audio Engine
 * Arfat Ansari | Nov 2026
 *
 * synthesizes procedural R2-D2 and Wall-E style robot sounds for MAX98357A I2S amp
 * running on ESP32-S3:
 * - happy chirps and ascending trills
 * - inquisitive questioning whistles
 * - 25hz harmonic purr hum synced with the physical servo petting engine
 * - startled squeaks when loud claps or knocks occur
 * - audio envelope tracking so OLED eyes and halo LEDs bounce in sync with voice
 *
 * no external SD card needed - all sounds are computed mathematically in realtime
 */

#ifndef AURA_VOICE_SYNTH_H
#define AURA_VOICE_SYNTH_H

#include <Arduino.h>
#include <math.h>

// MAX98357A I2S Class D Amplifier pins on ESP32-S3
#define PIN_SPK_BCLK        48  // bit clock
#define PIN_SPK_LRC         47  // left right clock (word select)
#define PIN_SPK_DIN         21  // serial audio data out

#define VOICE_SAMPLE_RATE   22050
#define VOICE_PI            3.14159265358979323846f

enum VoiceSoundType {
    VOICE_NONE,
    VOICE_HAPPY_CHIRP,       // ascending cheerful Wall-E trill
    VOICE_CURIOUS_TRILL,     // inquisitive two tone rising whistle
    VOICE_PURR_WARBLE,       // warm 25Hz modulated harmonic purr for petting
    VOICE_STARTLED_SQUEAK,   // fast high to low alert squeak
    VOICE_CONFIRM_CHIME,     // crisp positive double bell
    VOICE_SLEEP_SIGH         // gentle descending sleepy sigh
};

class AuraVoiceSynth {
private:
    VoiceSoundType currentSound;
    uint32_t soundStartTime;
    uint32_t soundDuration;
    float currentFreq;
    float targetFreq;
    float phase;
    float currentAmplitude;
    float targetAmplitude;
    float pitchRatio;        // normalized 0.0 to 1.0 for eye elevation sync
    bool active;

public:
    AuraVoiceSynth() :
        currentSound(VOICE_NONE),
        soundStartTime(0),
        soundDuration(0),
        currentFreq(0.0f),
        targetFreq(0.0f),
        phase(0.0f),
        currentAmplitude(0.0f),
        targetAmplitude(0.0f),
        pitchRatio(0.5f),
        active(false) {}

    void init() {
        pinMode(PIN_SPK_BCLK, OUTPUT);
        pinMode(PIN_SPK_LRC, OUTPUT);
        pinMode(PIN_SPK_DIN, OUTPUT);
        active = false;
        currentAmplitude = 0.0f;
    }

    // trigger an expressive procedural sound effect
    void play(VoiceSoundType type, uint32_t now) {
        currentSound = type;
        soundStartTime = now;
        phase = 0.0f;
        active = true;

        switch (type) {
            case VOICE_HAPPY_CHIRP:
                soundDuration = 380; // ms
                currentFreq = 988.0f;
                targetFreq = 2349.0f;
                targetAmplitude = 0.95f;
                break;

            case VOICE_CURIOUS_TRILL:
                soundDuration = 450;
                currentFreq = 720.0f;
                targetFreq = 1680.0f;
                targetAmplitude = 0.85f;
                break;

            case VOICE_PURR_WARBLE:
                soundDuration = 1200;
                currentFreq = 160.0f;
                targetFreq = 190.0f;
                targetAmplitude = 0.65f;
                break;

            case VOICE_STARTLED_SQUEAK:
                soundDuration = 140;
                currentFreq = 3200.0f;
                targetFreq = 480.0f;
                targetAmplitude = 1.0f;
                break;

            case VOICE_CONFIRM_CHIME:
                soundDuration = 260;
                currentFreq = 1046.0f;
                targetFreq = 1568.0f;
                targetAmplitude = 0.9f;
                break;

            case VOICE_SLEEP_SIGH:
                soundDuration = 800;
                currentFreq = 440.0f;
                targetFreq = 180.0f;
                targetAmplitude = 0.5f;
                break;

            default:
                active = false;
                soundDuration = 0;
                break;
        }
    }

    // update generator state (~60Hz tick)
    void update(uint32_t now) {
        if (!active) {
            currentAmplitude *= 0.85f; // smooth decay to silence
            return;
        }

        uint32_t elapsed = now - soundStartTime;
        if (elapsed >= soundDuration) {
            active = false;
            currentSound = VOICE_NONE;
            return;
        }

        float progress = (float)elapsed / (float)soundDuration;

        // mathematical trajectory per sound category
        switch (currentSound) {
            case VOICE_HAPPY_CHIRP: {
                // stepped playful arpeggio with 22Hz frequency vibrato
                float vibrato = 110.0f * sinf(2.0f * VOICE_PI * 22.0f * (elapsed * 0.001f));
                float step = floorf(progress * 4.0f) / 4.0f;
                currentFreq = 980.0f + step * 1250.0f + vibrato;
                // bell curve envelope
                currentAmplitude = sinf(progress * VOICE_PI) * targetAmplitude;
                pitchRatio = progress;
                break;
            }

            case VOICE_CURIOUS_TRILL: {
                // smooth rising inflection with subtle hesitation
                float curve = powf(progress, 1.4f);
                float wobble = 45.0f * sinf(2.0f * VOICE_PI * 14.0f * (elapsed * 0.001f));
                currentFreq = 720.0f + curve * 960.0f + wobble;
                currentAmplitude = sinf(progress * VOICE_PI) * targetAmplitude;
                pitchRatio = curve;
                break;
            }

            case VOICE_PURR_WARBLE: {
                // deep warm resonant purr with 25Hz amplitude pulse matching petting flutter
                float purrMod = 0.5f + 0.5f * sinf(2.0f * VOICE_PI * 25.0f * (elapsed * 0.001f));
                currentFreq = 150.0f + purrMod * 35.0f;
                currentAmplitude = targetAmplitude * (0.6f + 0.4f * purrMod);
                pitchRatio = 0.2f;
                break;
            }

            case VOICE_STARTLED_SQUEAK: {
                // aggressive downward drop
                currentFreq = 3200.0f * expf(-progress * 3.5f) + 400.0f;
                currentAmplitude = (1.0f - progress) * targetAmplitude;
                pitchRatio = 1.0f - progress;
                break;
            }

            case VOICE_CONFIRM_CHIME: {
                // dual distinct bell tones
                if (progress < 0.45f) {
                    currentFreq = 1046.0f; // C6
                    currentAmplitude = targetAmplitude * (1.0f - progress * 1.5f);
                } else if (progress < 0.55f) {
                    currentAmplitude = 0.0f; // tiny 10% gap
                } else {
                    float subP = (progress - 0.55f) / 0.45f;
                    currentFreq = 1568.0f; // G6
                    currentAmplitude = targetAmplitude * (1.0f - subP);
                }
                pitchRatio = (progress < 0.5f) ? 0.6f : 0.9f;
                break;
            }

            case VOICE_SLEEP_SIGH: {
                // gentle descending sigh fading smoothly into quiet
                currentFreq = 440.0f - progress * 260.0f;
                currentAmplitude = targetAmplitude * (1.0f - progress * progress);
                pitchRatio = 0.25f;
                break;
            }

            default:
                break;
        }
    }

    // fill DMA buffer with computed 16-bit PCM samples
    void generateSamples(int16_t *buffer, size_t sampleCount) {
        if (!active || currentAmplitude < 0.005f) {
            memset(buffer, 0, sampleCount * sizeof(int16_t));
            return;
        }

        float phaseIncrement = (2.0f * VOICE_PI * currentFreq) / (float)VOICE_SAMPLE_RATE;
        float peakAmp = currentAmplitude * 28000.0f; // scale to 16-bit range with headroom

        for (size_t i = 0; i < sampleCount; i++) {
            // primary fundamental sine wave
            float sample = sinf(phase);

            // add gentle 2nd harmonic overtone for warm robotic character
            if (currentSound == VOICE_PURR_WARBLE || currentSound == VOICE_HAPPY_CHIRP) {
                sample = 0.75f * sample + 0.25f * sinf(phase * 2.0f);
            }

            buffer[i] = (int16_t)(sample * peakAmp);
            phase += phaseIncrement;
            if (phase >= 2.0f * VOICE_PI) {
                phase -= 2.0f * VOICE_PI;
            }
        }
    }

    bool isPlaying() const { return active; }
    float getAudioLevel() const { return currentAmplitude; }
    float getPitchRatio() const { return pitchRatio; }
    VoiceSoundType getCurrentSound() const { return currentSound; }
};

#endif // AURA_VOICE_SYNTH_H