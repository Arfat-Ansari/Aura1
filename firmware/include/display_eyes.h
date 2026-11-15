#ifndef AURA_DISPLAY_EYES_H
#define AURA_DISPLAY_EYES_H

#include <Arduino.h>
#include <math.h>

// GC9A01 Hardware Pin Assignments on Dual SPI DMA Buses
#define PIN_SPI2_MOSI       11
#define PIN_SPI2_SCLK       12
#define PIN_LCD_L_CS        10
#define PIN_LCD_L_DC        9
#define PIN_LCD_L_RST       14

#define PIN_SPI3_MOSI       17
#define PIN_SPI3_SCLK       18
#define PIN_LCD_R_CS        15
#define PIN_LCD_R_DC        16
#define PIN_LCD_R_RST       7

// Eye Geometry Constants (in pixels for 240x240 circular panel)
static const int EYE_CENTER_X     = 120;
static const int EYE_CENTER_Y     = 120;
static const int SCLERA_RADIUS    = 110;
static const int IRIS_RADIUS_NOM  = 54;
static const int PUPIL_RADIUS_NOM = 26;

// Precomputed 256-entry Trigonometry LUT (Q15 fixed-point or float lookup)
class TrigLUT {
public:
    static float sinVal[256];
    static float cosVal[256];
    static bool initialized;

    static void init() {
        if (!initialized) {
            for (int i = 0; i < 256; i++) {
                float rad = ((float)i / 256.0f) * 2.0f * PI;
                sinVal[i] = sinf(rad);
                cosVal[i] = cosf(rad);
            }
            initialized = true;
        }
    }

    static inline float getSin(uint8_t index) { return sinVal[index]; }
    static inline float getCos(uint8_t index) { return cosVal[index]; }
};

// Initialize static members
__attribute__((weak)) float TrigLUT::sinVal[256];
__attribute__((weak)) float TrigLUT::cosVal[256];
__attribute__((weak)) bool  TrigLUT::initialized = false;

enum EyeExpression {
    EXPR_NORMAL,
    EXPR_HAPPY,
    EXPR_CURIOUS,
    EXPR_ALERT,
    EXPR_SLEEPY,
    EXPR_WINK
};

struct EyeState {
    float pupilX;          // Horizontal offset (-36.0 to +36.0)
    float pupilY;          // Vertical offset   (-32.0 to +32.0)
    float pupilDilation;   // Multiplier (0.75 to 1.45)
    float eyelidOpen;      // 0.0 (closed) to 1.0 (fully open)
    EyeExpression expression;
};

class AuraEyeEngine {
private:
    EyeState currentState;
    EyeState targetState;
    uint32_t lastBlinkTime;
    uint32_t nextBlinkInterval;
    bool isBlinking;
    float blinkProgress;

    // Saccadic Eye Movement Parameters
    uint32_t lastSaccadeTime;
    uint32_t nextSaccadeInterval;
    float saccadeOffsetX;
    float saccadeOffsetY;

public:
    AuraEyeEngine() : 
        lastBlinkTime(0), nextBlinkInterval(3200), isBlinking(false), blinkProgress(0.0f),
        lastSaccadeTime(0), nextSaccadeInterval(1800), saccadeOffsetX(0.0f), saccadeOffsetY(0.0f) {
        currentState.pupilX = 0.0f;
        currentState.pupilY = 0.0f;
        currentState.pupilDilation = 1.0f;
        currentState.eyelidOpen = 1.0f;
        currentState.expression = EXPR_NORMAL;

        targetState = currentState;
    }

    void init() {
        TrigLUT::init();
    }

    // Set intended gaze target (-35.0 to +35.0 degrees/pixels)
    void setGaze(float x, float y) {
        targetState.pupilX = constrain(x, -35.0f, 35.0f);
        targetState.pupilY = constrain(y, -30.0f, 30.0f);
    }

    // Adapt pupil dilation according to target distance (closer = more dilated/curious)
    void adaptToDistance(uint16_t distMm) {
        if (currentState.expression == EXPR_ALERT) return;
        if (distMm < 200) {
            targetState.pupilDilation = 1.35f; // Inquisitive close inspection
        } else if (distMm < 600) {
            targetState.pupilDilation = 1.15f;
        } else if (distMm > 2500) {
            targetState.pupilDilation = 0.90f; // Relaxed / resting
        } else {
            targetState.pupilDilation = 1.00f;
        }
    }

    void setExpression(EyeExpression expr) {
        targetState.expression = expr;
        switch(expr) {
            case EXPR_ALERT:
                targetState.pupilDilation = 1.40f;
                targetState.eyelidOpen = 1.0f;
                break;
            case EXPR_HAPPY:
                targetState.pupilDilation = 1.05f;
                targetState.eyelidOpen = 0.70f;
                break;
            case EXPR_CURIOUS:
                targetState.pupilDilation = 1.25f;
                targetState.eyelidOpen = 0.95f;
                break;
            case EXPR_SLEEPY:
                targetState.pupilDilation = 0.80f;
                targetState.eyelidOpen = 0.35f;
                break;
            case EXPR_NORMAL:
            default:
                targetState.pupilDilation = 1.0f;
                targetState.eyelidOpen = 1.0f;
                break;
        }
    }

    void update(uint32_t currentTimeMs) {
        // 1. Natural Saccadic Micro-Movements (eyes dart gently every 1.5 - 3.5s)
        if (currentTimeMs - lastSaccadeTime > nextSaccadeInterval) {
            lastSaccadeTime = currentTimeMs;
            nextSaccadeInterval = random(1400, 3200);
            saccadeOffsetX = ((float)random(-50, 50)) * 0.08f; // +-4.0px jitter
            saccadeOffsetY = ((float)random(-30, 30)) * 0.08f; // +-2.4px jitter
        }

        // 2. Natural automatic blink scheduling
        if (!isBlinking && (currentTimeMs - lastBlinkTime > nextBlinkInterval)) {
            isBlinking = true;
            blinkProgress = 0.0f;
            lastBlinkTime = currentTimeMs;
            nextBlinkInterval = random(2200, 5500);
        }

        if (isBlinking) {
            blinkProgress += 0.22f; // ~90ms fast blink close and open
            if (blinkProgress >= 1.0f) {
                isBlinking = false;
                currentState.eyelidOpen = targetState.eyelidOpen;
            } else {
                // Non-linear easing curve: rapid closure, smooth organic reopen
                currentState.eyelidOpen = (blinkProgress < 0.45f) ? 
                    (1.0f - blinkProgress * 2.22f) : ((blinkProgress - 0.45f) * 1.82f);
            }
        } else {
            currentState.eyelidOpen += (targetState.eyelidOpen - currentState.eyelidOpen) * 0.16f;
        }

        // 3. Smooth gaze interpolation combining macro target and micro saccades
        float targetWithSaccadeX = targetState.pupilX + saccadeOffsetX;
        float targetWithSaccadeY = targetState.pupilY + saccadeOffsetY;
        
        currentState.pupilX += (targetWithSaccadeX - currentState.pupilX) * 0.22f;
        currentState.pupilY += (targetWithSaccadeY - currentState.pupilY) * 0.22f;
        currentState.pupilDilation += (targetState.pupilDilation - currentState.pupilDilation) * 0.12f;
        
        // Safety clamp within circular GC9A01 boundary
        float currentDistSq = currentState.pupilX * currentState.pupilX + currentState.pupilY * currentState.pupilY;
        if (currentDistSq > (42.0f * 42.0f)) {
            float scale = 42.0f / sqrtf(currentDistSq);
            currentState.pupilX *= scale;
            currentState.pupilY *= scale;
        }
    }

    const EyeState& getState() const { return currentState; }
};

#endif // AURA_DISPLAY_EYES_H
