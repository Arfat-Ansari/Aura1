#ifndef AURA_AUDIO_DSP_H
#define AURA_AUDIO_DSP_H

#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>

// INMP441 Omnidirectional Digital MEMS Microphone Pin Mapping
#define PIN_I2S_SCK         47  // Serial Clock (BCLK)
#define PIN_I2S_WS          48  // Word Select (LRCK)
#define PIN_I2S_SD          21  // Serial Data Output (DOUT)

#define AUDIO_SAMPLE_RATE   16000
#define AUDIO_DMA_BUF_LEN   256
#define AUDIO_DMA_BUF_COUNT 4

enum AudioEventType {
    AUDIO_SILENCE,
    AUDIO_VOICE_SPEECH,     // Sustained acoustic energy in 300Hz - 3.4kHz vocal band
    AUDIO_CLAP_TRANSIENT,   // Sharp impulse attack (>18dB rise in <30ms)
    AUDIO_AMBIENT_NOISE     // Background exhibition chatter
};

struct AudioAnalysis {
    float rmsDecibels;          // Current acoustic energy (-60.0 dBFS to 0.0 dBFS)
    float peakDecibels;         // Fast transient peak
    float vocalBandRatio;       // Ratio of vocal energy (300-3400Hz) vs motor noise (<250Hz)
    bool speechDetected;        // True if vocal ratio > 0.65 and dB > -36.0 dBFS
    bool clapDetected;          // True if fast transient detected
    AudioEventType eventType;
};

class AuraAudioDSP {
private:
    AudioAnalysis analysis;
    float noiseFloorDb;
    int32_t dmaSampleBuffer[AUDIO_DMA_BUF_LEN];
    uint32_t lastAnalysisTime;

    // Second-order digital IIR bandpass filter state for vocal band isolation
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;

public:
    AuraAudioDSP() : noiseFloorDb(-48.0f), lastAnalysisTime(0),
                     x1(0), x2(0), y1(0), y2(0) {
        analysis.rmsDecibels = -60.0f;
        analysis.peakDecibels = -60.0f;
        analysis.vocalBandRatio = 0.0f;
        analysis.speechDetected = false;
        analysis.clapDetected = false;
        analysis.eventType = AUDIO_SILENCE;

        // Precompute 2nd-order Butterworth vocal bandpass filter coefficients (fc = 1200Hz, Q = 0.707)
        float w0 = 2.0f * PI * 1200.0f / (float)AUDIO_SAMPLE_RATE;
        float alpha = sinf(w0) / (2.0f * 0.707f);
        float a0 = 1.0f + alpha;
        b0 = (sinf(w0) / 2.0f) / a0;
        b1 = 0.0f;
        b2 = (-sinf(w0) / 2.0f) / a0;
        a1 = (-2.0f * cosf(w0)) / a0;
        a2 = (1.0f - alpha) / a0;
    }

    void init() {
        // Configure ESP32-S3 hardware I2S peripheral
        i2s_config_t i2s_config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
            .sample_rate = AUDIO_SAMPLE_RATE,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
            .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
            .communication_format = I2S_COMM_FORMAT_STAND_I2S,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = AUDIO_DMA_BUF_COUNT,
            .dma_buf_len = AUDIO_DMA_BUF_LEN,
            .use_apll = true,
            .tx_desc_auto_clear = false,
            .fixed_mclk = 0
        };

        i2s_pin_config_t pin_config = {
            .bck_io_num = PIN_I2S_SCK,
            .ws_io_num = PIN_I2S_WS,
            .data_out_num = I2S_PIN_NO_CHANGE,
            .data_in_num = PIN_I2S_SD
        };

        i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
        i2s_set_pin(I2S_NUM_0, &pin_config);
        i2s_zero_dma_buffer(I2S_NUM_0);
    }

    void update() {
        uint32_t now = millis();
        if (now - lastAnalysisTime < 16) return; // ~60 Hz DSP refresh
        lastAnalysisTime = now;

        size_t bytesRead = 0;
        esp_err_t err = i2s_read(I2S_NUM_0, (void*)dmaSampleBuffer, sizeof(dmaSampleBuffer), &bytesRead, 0);
        if (err != ESP_OK || bytesRead == 0) return;

        int samples = bytesRead / sizeof(int32_t);
        float sumSquaresTotal = 0.0f;
        float sumSquaresVocal = 0.0f;
        float maxPeak = 0.0f;

        for (int i = 0; i < samples; i++) {
            // INMP441 delivers 24-bit sign-extended data in upper 24 bits of 32-bit word
            float rawSample = (float)(dmaSampleBuffer[i] >> 8) / 8388608.0f; // Normalize to -1.0 .. +1.0
            float absSample = fabsf(rawSample);
            if (absSample > maxPeak) maxPeak = absSample;

            sumSquaresTotal += rawSample * rawSample;

            // Apply 2nd-order bandpass filter to isolate human vocal spectrum from servo PWM whine
            float filteredSample = b0 * rawSample + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1;
            x1 = rawSample;
            y2 = y1;
            y1 = filteredSample;

            sumSquaresVocal += filteredSample * filteredSample;
        }

        float totalRms = sqrtf(sumSquaresTotal / (float)samples);
        float vocalRms = sqrtf(sumSquaresVocal / (float)samples);

        // Convert to decibels relative to full scale (dBFS)
        float db = (totalRms > 0.0001f) ? (20.0f * log10f(totalRms)) : -60.0f;
        if (db < -60.0f) db = -60.0f;

        analysis.rmsDecibels = db;
        analysis.peakDecibels = (maxPeak > 0.0001f) ? (20.0f * log10f(maxPeak)) : -60.0f;

        // Calculate vocal energy ratio to reject internal mechanical vibrations
        analysis.vocalBandRatio = (totalRms > 0.001f) ? (vocalRms / totalRms) : 0.0f;

        // Dynamic Noise Floor Tracking
        if (db < noiseFloorDb) {
            noiseFloorDb = noiseFloorDb * 0.95f + db * 0.05f;
        } else {
            noiseFloorDb = noiseFloorDb * 0.999f + db * 0.001f;
        }

        // Speech & Event Classification
        bool loudEnough = (db > (noiseFloorDb + 10.0f)) && (db > -38.0f);
        analysis.speechDetected = loudEnough && (analysis.vocalBandRatio > 0.45f);
        analysis.clapDetected = (analysis.peakDecibels > (noiseFloorDb + 22.0f)) && (analysis.peakDecibels > -15.0f);

        if (analysis.clapDetected) {
            analysis.eventType = AUDIO_CLAP_TRANSIENT;
        } else if (analysis.speechDetected) {
            analysis.eventType = AUDIO_VOICE_SPEECH;
        } else if (loudEnough) {
            analysis.eventType = AUDIO_AMBIENT_NOISE;
        } else {
            analysis.eventType = AUDIO_SILENCE;
        }
    }

    const AudioAnalysis& getAnalysis() const { return analysis; }
};

#endif // AURA_AUDIO_DSP_H
