#ifndef AURA_HALO_LEDS_H
#define AURA_HALO_LEDS_H

#include <Arduino.h>
#include <math.h>

// WS2812B Addressable RGB Halo Ring Pin (12x NeoPixels around neck turntable)
#define PIN_HALO_RGB        38
#define HALO_LED_COUNT      12

enum HaloEffect {
    HALO_IDLE_CYAN_BREATHE,     // Slow breathing glow during idle wander
    HALO_ALERT_ELECTRIC_BLUE,   // Crisp tracking ring when attendee detected
    HALO_PET_WARM_AMBER,        // Warm soft purring glow when being petted
    HALO_SURPRISE_WHITE_FLASH,  // Instant bright impact flash on table bump
    HALO_SLEEPY_DIM_VIOLET      // Dim relaxed hue
};

struct HaloColor {
    uint8_t r, g, b;
};

class AuraHaloLEDs {
private:
    HaloEffect currentEffect;
    HaloColor frameBuffer[HALO_LED_COUNT];
    uint32_t lastUpdateTime;
    float phaseAngle;

public:
    AuraHaloLEDs() : currentEffect(HALO_IDLE_CYAN_BREATHE), lastUpdateTime(0), phaseAngle(0.0f) {
        for (int i = 0; i < HALO_LED_COUNT; i++) {
            frameBuffer[i] = {0, 0, 0};
        }
    }

    void init() {
        pinMode(PIN_HALO_RGB, OUTPUT);
        setEffect(HALO_IDLE_CYAN_BREATHE);
    }

    void setEffect(HaloEffect effect) {
        currentEffect = effect;
    }

    void update(uint32_t now) {
        if (now - lastUpdateTime < 25) return; // 40 Hz animation rate
        lastUpdateTime = now;

        phaseAngle += 0.08f;
        if (phaseAngle > 2.0f * PI) phaseAngle -= 2.0f * PI;

        switch (currentEffect) {
            case HALO_IDLE_CYAN_BREATHE: {
                // Soft breathing cyan (0.0 to 1.0 sine wave)
                float brightness = 0.35f + 0.35f * sinf(phaseAngle);
                for (int i = 0; i < HALO_LED_COUNT; i++) {
                    frameBuffer[i].r = (uint8_t)(10 * brightness);
                    frameBuffer[i].g = (uint8_t)(180 * brightness);
                    frameBuffer[i].b = (uint8_t)(240 * brightness);
                }
                break;
            }

            case HALO_ALERT_ELECTRIC_BLUE: {
                // Rotating highlight around the ring to show tracking
                float rot = phaseAngle * 2.0f;
                for (int i = 0; i < HALO_LED_COUNT; i++) {
                    float angle = (float)i * (2.0f * PI / HALO_LED_COUNT);
                    float diff = cosf(angle - rot);
                    float intensity = (diff > 0.0f) ? diff : 0.0f;
                    frameBuffer[i].r = (uint8_t)(0);
                    frameBuffer[i].g = (uint8_t)(120 * intensity + 40);
                    frameBuffer[i].b = (uint8_t)(255 * intensity);
                }
                break;
            }

            case HALO_PET_WARM_AMBER: {
                // Warm breathing golden amber purr glow
                float pulse = 0.5f + 0.45f * sinf(phaseAngle * 2.5f); // 25Hz harmonic feel
                for (int i = 0; i < HALO_LED_COUNT; i++) {
                    frameBuffer[i].r = (uint8_t)(255 * pulse);
                    frameBuffer[i].g = (uint8_t)(140 * pulse);
                    frameBuffer[i].b = (uint8_t)(15 * pulse);
                }
                break;
            }

            case HALO_SURPRISE_WHITE_FLASH: {
                // Instant bright shock flash
                for (int i = 0; i < HALO_LED_COUNT; i++) {
                    frameBuffer[i].r = 255;
                    frameBuffer[i].g = 255;
                    frameBuffer[i].b = 255;
                }
                break;
            }

            case HALO_SLEEPY_DIM_VIOLET: {
                for (int i = 0; i < HALO_LED_COUNT; i++) {
                    frameBuffer[i].r = 60;
                    frameBuffer[i].g = 20;
                    frameBuffer[i].b = 120;
                }
                break;
            }
        }
    }

    const HaloColor* getBuffer() const { return frameBuffer; }
    HaloEffect getEffect() const { return currentEffect; }
};

#endif // AURA_HALO_LEDS_H
