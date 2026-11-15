/*
 * servo_smooth.h — AURA-1 Servo Smoothing Engine
 * Arfat Ansari | Nov 2026
 *
 * makes the neck not jerk around like a broken toy
 * uses cubic easing so movements look natural and organic
 * each servo gets its own position tracker and velocity limit
 */

#ifndef AURA_SERVO_SMOOTH_H
#define AURA_SERVO_SMOOTH_H

#include <Arduino.h>
#include <ESP32Servo.h>

// how many servos we got total (pan + tilt left + tilt right)
#define AURA_SERVO_COUNT 3

// servo pin assignments on ESP32-S3
#define PIN_PAN   4   // base rotation
#define PIN_TILT_L 5  // left tilt pushrod
#define PIN_TILT_R 6  // right tilt pushrod

// movement limits in degrees
#define PAN_MIN    30
#define PAN_MAX   150
#define PAN_CENTER 90

#define TILT_MIN   60
#define TILT_MAX  120
#define TILT_CENTER 90

// smoothing parameters
#define SMOOTH_STEP_MS   20    // update every 20ms (50Hz refresh)
#define MAX_SPEED_DEG_S  180.0 // fastest the servo can move per second
#define EASE_FACTOR      0.12  // lower = smoother but slower response

struct ServoState {
    Servo servo;
    uint8_t pin;
    float current_pos;   // where the servo actually is right now
    float target_pos;    // where we want it to go
    float velocity;      // current movement speed
    int16_t limit_min;
    int16_t limit_max;
    int16_t center;
};

static ServoState aura_servos[AURA_SERVO_COUNT];
static unsigned long last_update_ms = 0;

// cubic ease in/out for natural motion feel
static float cubic_ease(float t) {
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    } else {
        float f = (2.0f * t) - 2.0f;
        return 0.5f * f * f * f + 1.0f;
    }
}

void servo_init() {
    // pan servo
    aura_servos[0].pin = PIN_PAN;
    aura_servos[0].limit_min = PAN_MIN;
    aura_servos[0].limit_max = PAN_MAX;
    aura_servos[0].center = PAN_CENTER;

    // tilt left
    aura_servos[1].pin = PIN_TILT_L;
    aura_servos[1].limit_min = TILT_MIN;
    aura_servos[1].limit_max = TILT_MAX;
    aura_servos[1].center = TILT_CENTER;

    // tilt right
    aura_servos[2].pin = PIN_TILT_R;
    aura_servos[2].limit_min = TILT_MIN;
    aura_servos[2].limit_max = TILT_MAX;
    aura_servos[2].center = TILT_CENTER;

    for (int i = 0; i < AURA_SERVO_COUNT; i++) {
        aura_servos[i].servo.attach(aura_servos[i].pin);
        aura_servos[i].current_pos = aura_servos[i].center;
        aura_servos[i].target_pos  = aura_servos[i].center;
        aura_servos[i].velocity    = 0.0f;
        aura_servos[i].servo.write(aura_servos[i].center);
    }

    last_update_ms = millis();
}

// set where a servo should move to
// it wont jump there instantly it will glide smoothly
void servo_set_target(uint8_t index, float degrees) {
    if (index >= AURA_SERVO_COUNT) return;

    // clamp to safe range so we dont strip gears
    if (degrees < aura_servos[index].limit_min)
        degrees = aura_servos[index].limit_min;
    if (degrees > aura_servos[index].limit_max)
        degrees = aura_servos[index].limit_max;

    aura_servos[index].target_pos = degrees;
}

// call this from main loop every frame
// it moves all servos one tiny step closer to their targets
void servo_update() {
    unsigned long now = millis();
    unsigned long dt_ms = now - last_update_ms;

    if (dt_ms < SMOOTH_STEP_MS) return;  // not time yet

    float dt_sec = dt_ms / 1000.0f;
    last_update_ms = now;

    for (int i = 0; i < AURA_SERVO_COUNT; i++) {
        float error = aura_servos[i].target_pos - aura_servos[i].current_pos;

        // if we are close enough just snap and stop
        if (fabs(error) < 0.3f) {
            aura_servos[i].current_pos = aura_servos[i].target_pos;
            aura_servos[i].velocity = 0.0f;
        } else {
            // exponential smoothing with velocity limiting
            float step = error * EASE_FACTOR;

            // dont let it move faster than our max speed
            float max_step = MAX_SPEED_DEG_S * dt_sec;
            if (step > max_step) step = max_step;
            if (step < -max_step) step = -max_step;

            aura_servos[i].velocity = step / dt_sec;
            aura_servos[i].current_pos += step;
        }

        // actually move the physical servo
        aura_servos[i].servo.write((int)aura_servos[i].current_pos);
    }
}

// move head using pan and combined tilt (converts to left/right servo angles)
// pan_deg: left right rotation from center
// pitch_deg: nod up down
// roll_deg: tilt sideways
void servo_move_head(float pan_deg, float pitch_deg, float roll_deg) {
    servo_set_target(0, PAN_CENTER + pan_deg);

    // differential tilt: left and right servos work together
    // pitch = both servos same direction
    // roll = servos opposite direction
    float left_angle  = TILT_CENTER + pitch_deg + roll_deg;
    float right_angle = TILT_CENTER + pitch_deg - roll_deg;

    servo_set_target(1, left_angle);
    servo_set_target(2, right_angle);
}

// send all servos back to center position smoothly
void servo_center_all() {
    for (int i = 0; i < AURA_SERVO_COUNT; i++) {
        servo_set_target(i, aura_servos[i].center);
    }
}

// check if all servos have reached their targets
bool servo_all_settled() {
    for (int i = 0; i < AURA_SERVO_COUNT; i++) {
        if (fabs(aura_servos[i].target_pos - aura_servos[i].current_pos) > 0.5f) {
            return false;
        }
    }
    return true;
}

#endif // AURA_SERVO_SMOOTH_H
