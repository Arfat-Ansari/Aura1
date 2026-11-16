/*
 * display_eyes.h - AURA 1 Dual GC9A01 Eye Animation Engine
 * Arfat Ansari | Nov 2026
 *
 * renders procedural eyes on two 240x240 circular LCDs over SPI
 * features:
 * - saccadic eye jitter so the robot looks alive and not frozen
 * - natural blinking with randomized timing every few seconds
 * - pupil dilation (closer objects = bigger pupils)
 * - moods: normal, happy anime squint, curious tilt, alert, sleepy
 */

#ifndef AURA_DISPLAY_EYES_H
#define AURA_DISPLAY_EYES_H

#include <Arduino.h>
#include <math.h>

// GC9A01 pin connections on ESP32-S3 (dual SPI buses)
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

// dimensions for 240x240 circular screen
static const int EYE_CENTER_X     = 120;
static const int EYE_CENTER_Y     = 120;
static const int SCLERA_RADIUS    = 110;
static const int IRIS_RADIUS_NOM  = 54;
static const int PUPIL_RADIUS_NOM = 26;

// precomputed trig lookup table for fast circle drawing
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
    float pupilX;          // horizontal offset (-36 to +36 px)
    float pupilY;          // vertical offset (-32 to +32 px)
    float pupilDilation;   // pupil size scale (0.75 to 1.45)
    float eyelidOpen;      // 0.0 = shut, 1.0 = fully open
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

    // random saccades so eyes glance around naturally
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

    // move eyes toward a target direction (-35 to +35 degrees/px)
    void setGaze(float x, float y) {
        targetState.pupilX = constrain(x, -35.0f, 35.0f);
        targetState.pupilY = constrain(y, -30.0f, 30.0f);
    }

    // dilate pupils when something gets close to the laser sensor
    void adaptToDistance(uint16_t distMm) {
        if (currentState.expression == EXPR_ALERT) return;
        if (distMm < 200) {
            targetState.pupilDilation = 1.35f; // curious close look
        } else if (distMm < 600) {
            targetState.pupilDilation = 1.15f;
        } else if (distMm > 2500) {
            targetState.pupilDilation = 0.90f; // relaxed far away
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
        // 1. subtle micro saccades every 1.5 to 3.5 seconds
        if (currentTimeMs - lastSaccadeTime > nextSaccadeInterval) {
            lastSaccadeTime = currentTimeMs;
            nextSaccadeInterval = random(1400, 3200);
            saccadeOffsetX = ((float)random(-50, 50)) * 0.08f;
            saccadeOffsetY = ((float)random(-30, 30)) * 0.08f;
        }

        // 2. auto blink timer
        if (!isBlinking && (currentTimeMs - lastBlinkTime > nextBlinkInterval)) {
            isBlinking = true;
            blinkProgress = 0.0f;
            lastBlinkTime = currentTimeMs;
            nextBlinkInterval = random(2200, 5500);
        }

        if (isBlinking) {
            blinkProgress += 0.22f; // quick 90ms blink
            if (blinkProgress >= 1.0f) {
                isBlinking = false;
                currentState.eyelidOpen = targetState.eyelidOpen;
            } else {
                // fast close then smooth reopen
                currentState.eyelidOpen = (blinkProgress < 0.45f) ? 
                    (1.0f - blinkProgress * 2.22f) : ((blinkProgress - 0.45f) * 1.82f);
            }
        } else {
            currentState.eyelidOpen += (targetState.eyelidOpen - currentState.eyelidOpen) * 0.16f;
        }

        // 3. smooth gaze interpolation with saccade offsets
        float targetWithSaccadeX = targetState.pupilX + saccadeOffsetX;
        float targetWithSaccadeY = targetState.pupilY + saccadeOffsetY;
        
        currentState.pupilX += (targetWithSaccadeX - currentState.pupilX) * 0.22f;
        currentState.pupilY += (targetWithSaccadeY - currentState.pupilY) * 0.22f;
        currentState.pupilDilation += (targetState.pupilDilation - currentState.pupilDilation) * 0.12f;
        
        // clamp inside circular screen bounds
        float distSq = currentState.pupilX * currentState.pupilX + currentState.pupilY * currentState.pupilY;
        if (distSq > (42.0f * 42.0f)) {
            float scale = 42.0f / sqrtf(distSq);
            currentState.pupilX *= scale;
            currentState.pupilY *= scale;
        }
    }

    const EyeState& getState() const { return currentState; }
};

#endif // AURA_DISPLAY_EYES_H
