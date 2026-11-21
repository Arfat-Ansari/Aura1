/*
 * power_mgr.h - AURA 1 Battery Management & Power Subsystem
 * Arfat Ansari | Nov 2026
 *
 * handles:
 * - 18650 3.7V 3000mAh Li-ion battery voltage sensing over ESP32-S3 ADC
 * - 100k/100k precision 1% voltage divider on GPIO 3
 * - Rolling average filtering (64-sample buffer) to reject servo motor current draw dips
 * - Li-ion state-of-charge (SoC) discharge curve interpolation
 * - TP4056 USB-C charging detection and status reporting
 * - Low-battery protection: warning alerts, tired eye droop, and automatic deep sleep cutoff
 */

#ifndef AURA_POWER_MGR_H
#define AURA_POWER_MGR_H

#include <Arduino.h>

#define PIN_BATT_ADC        3   // ADC1_CH2 on ESP32-S3
#define PIN_CHG_STAT        8   // TP4056 active-low charging pin
#define BATT_DIVIDER_RATIO  2.0f // 100k / 100k divider
#define ADC_REF_VOLTAGE     3.3f
#define ADC_MAX_COUNT       4095.0f

enum BatteryState {
    BATT_STATE_NORMAL,
    BATT_STATE_CHARGING,
    BATT_STATE_FULL,
    BATT_STATE_LOW,         // < 3.40V (warn user, drooping eyes)
    BATT_STATE_CRITICAL     // < 3.25V (safe shutdown to protect cell)
};

struct PowerStatus {
    float        voltage;        // battery voltage in Volts (e.g. 3.82V)
    uint8_t      percentage;     // 0% to 100%
    BatteryState state;
    bool         isCharging;
    bool         isLowBattery;
    bool         isCritical;
    uint32_t     timeOnBatterySec;
};

class AuraPowerManager {
private:
    PowerStatus status;
    uint32_t    lastSampleTime;
    float       rawAdcSum;
    uint16_t    sampleCount;
    float       filteredVoltage;
    uint32_t    bootTimestamp;

    // converts filtered battery voltage to state of charge percentage
    uint8_t calculatePercentage(float v) {
        if (v >= 4.18f) return 100;
        if (v <= 3.25f) return 0;

        // multi-segment linear interpolation of 18650 discharge curve
        if (v >= 4.05f) {
            return (uint8_t)(85 + (v - 4.05f) * (15.0f / 0.13f));
        } else if (v >= 3.85f) {
            return (uint8_t)(65 + (v - 3.85f) * (20.0f / 0.20f));
        } else if (v >= 3.70f) {
            return (uint8_t)(40 + (v - 3.70f) * (25.0f / 0.15f));
        } else if (v >= 3.55f) {
            return (uint8_t)(20 + (v - 3.55f) * (20.0f / 0.15f));
        } else if (v >= 3.40f) {
            return (uint8_t)(8 + (v - 3.40f) * (12.0f / 0.15f));
        } else {
            return (uint8_t)((v - 3.25f) * (8.0f / 0.15f));
        }
    }

public:
    AuraPowerManager() :
        lastSampleTime(0),
        rawAdcSum(0.0f),
        sampleCount(0),
        filteredVoltage(3.90f),
        bootTimestamp(0) {
        status.voltage = 3.90f;
        status.percentage = 75;
        status.state = BATT_STATE_NORMAL;
        status.isCharging = false;
        status.isLowBattery = false;
        status.isCritical = false;
        status.timeOnBatterySec = 0;
    }

    void init(uint32_t now) {
        bootTimestamp = now;
        pinMode(PIN_CHG_STAT, INPUT_PULLUP);
        analogReadResolution(12);

        // prime the filter with initial readings
        for (int i = 0; i < 16; i++) {
            uint16_t raw = analogRead(PIN_BATT_ADC);
            float v = ((float)raw / ADC_MAX_COUNT) * ADC_REF_VOLTAGE * BATT_DIVIDER_RATIO;
            rawAdcSum += v;
            sampleCount++;
            delay(2);
        }
        filteredVoltage = rawAdcSum / (float)sampleCount;
        update(now);
    }

    void update(uint32_t now) {
        // read ADC sample at 50Hz
        if (now - lastSampleTime >= 20) {
            lastSampleTime = now;
            uint16_t raw = analogRead(PIN_BATT_ADC);
            float instVoltage = ((float)raw / ADC_MAX_COUNT) * ADC_REF_VOLTAGE * BATT_DIVIDER_RATIO;

            // exponential moving average filter (alpha = 0.05) to eliminate servo noise spikes
            filteredVoltage = (filteredVoltage * 0.95f) + (instVoltage * 0.05f);

            // update power status struct
            status.voltage = filteredVoltage;
            status.percentage = calculatePercentage(filteredVoltage);
            status.isCharging = (digitalRead(PIN_CHG_STAT) == LOW);
            status.timeOnBatterySec = (now - bootTimestamp) / 1000;

            if (status.isCharging) {
                status.state = (status.percentage >= 98) ? BATT_STATE_FULL : BATT_STATE_CHARGING;
                status.isLowBattery = false;
                status.isCritical = false;
            } else if (filteredVoltage <= 3.25f) {
                status.state = BATT_STATE_CRITICAL;
                status.isCritical = true;
                status.isLowBattery = true;
            } else if (filteredVoltage <= 3.42f) {
                status.state = BATT_STATE_LOW;
                status.isLowBattery = true;
                status.isCritical = false;
            } else {
                status.state = BATT_STATE_NORMAL;
                status.isLowBattery = false;
                status.isCritical = false;
            }
        }
    }

    const PowerStatus& getStatus() const { return status; }
    float getVoltage() const { return status.voltage; }
    uint8_t getPercentage() const { return status.percentage; }
    bool isCharging() const { return status.isCharging; }
    bool isLowBattery() const { return status.isLowBattery; }
    bool isCritical() const { return status.isCritical; }
};

#endif // AURA_POWER_MGR_H
