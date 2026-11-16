/*
 * halo_leds.h - AURA 1 12-LED RGB Halo Ring
 * Arfat Ansari | Nov 2026
 *
 * WS2812B addressable LED ring around the neck/chest
 * patterns:
 * - cyan breathing when idle
 * - bright electric blue when alert / tracking
 * - warm amber pulse when getting petted
 * - slow dim blue when falling asleep
 */

#ifndef AURA_HALO_LEDS_H
#define AURA_HALO_LEDS_H

#include <Arduino.h>
#include <FastLED.h>

#define PIN_HALO_DATA       2
#define NUM_HALO_LEDS       12
#define HALO_COLOR_ORDER    GRB
#define HALO_CHIPSET        WS2812B

enum HaloEffect {
    HALO_IDLE_CYAN_BREATHE,
    HALO_ALERT_ELECTRIC_BLUE,
    HALO_PET_WARM_AMBER,
    HALO_STARTLE_FLASH,
    HALO_SLEEP_DIM
};

class AuraHaloLEDs {
private:
    CRGB leds[NUM_HALO_LEDS];
    HaloEffect currentEffect;
    float phase;

public:
    AuraHaloLEDs() : currentEffect(HALO_IDLE_CYAN_BREATHE), phase(0.0f) {}

    void init() {
        FastLED.addLeds<HALO_CHIPSET, PIN_HALO_DATA, HALO_COLOR_ORDER>(leds, NUM_HALO_LEDS);
        FastLED.setBrightness(120);
        FastLED.clear(true);
    }

    void setEffect(HaloEffect effect) {
        currentEffect = effect;
    }

    void update(uint32_t now) {
        phase += 0.04f;
        if (phase > 2.0f * PI) phase -= 2.0f * PI;

        switch (currentEffect) {
            case HALO_IDLE_CYAN_BREATHE: {
                // gentle breathing pulse between cyan and deep blue
                uint8_t bright = (uint8_t)(100 + sinf(phase) * 60);
                for (int i = 0; i < NUM_HALO_LEDS; i++) {
                    leds[i] = CRGB(0, bright, bright + 30);
                }
                break;
            }

            case HALO_ALERT_ELECTRIC_BLUE: {
                // rotating light dot chasing around the ring
                uint8_t leadIdx = (uint8_t)((now / 60) % NUM_HALO_LEDS);
                for (int i = 0; i < NUM_HALO_LEDS; i++) {
                    if (i == leadIdx) {
                        leds[i] = CRGB(80, 200, 255);
                    } else {
                        leds[i] = CRGB(0, 40, 100);
                    }
                }
                break;
            }

            case HALO_PET_WARM_AMBER: {
                // warm glowing amber/gold while being petted
                uint8_t bright = (uint8_t)(140 + sinf(phase * 2.0f) * 40);
                for (int i = 0; i < NUM_HALO_LEDS; i++) {
                    leds[i] = CRGB(bright, (bright * 7) / 10, 0);
                }
                break;
            }

            case HALO_STARTLE_FLASH: {
                // quick bright flash
                for (int i = 0; i < NUM_HALO_LEDS; i++) {
                    leds[i] = CRGB(255, 255, 255);
                }
                break;
            }

            case HALO_SLEEP_DIM: {
                // barely visible deep blue
                for (int i = 0; i < NUM_HALO_LEDS; i++) {
                    leds[i] = CRGB(0, 5, 20);
                }
                break;
            }
        }

        FastLED.show();
    }
};

#endif // AURA_HALO_LEDS_H
