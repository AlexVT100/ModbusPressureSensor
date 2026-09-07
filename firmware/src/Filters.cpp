#include "Filters.h"
#include "TerminalLogger.h"

extern TerminalLogger Logger;

//-----------------------------------------------------------------------------
// Exponential Moving Average (EMA) filter
//-----------------------------------------------------------------------------
//
uint16_t ema_filter(uint16_t v, uint8_t alpha) {
    constexpr uint32_t scale = 256;
    static uint32_t state = uint32_t(v) * scale;

    int32_t delta = uint32_t(v) * scale - state;

    // Using intermediate int64_t to avoid signed/unsigned overflow problem when the input decreases
    state += (int64_t(alpha) * delta) / 100;

    return (state + scale / 2) / scale;
}

//-----------------------------------------------------------------------------
// Median filter
//
// https://zbotic.in/how-to-fix-noisy-sensor-readings-filtering-techniques-for-arduino/#median-filter
//-----------------------------------------------------------------------------
//
uint32_t median_filter(uint8_t pin) {
    const size_t MEDIAN_SIZE = 9;
    static uint32_t buf[MEDIAN_SIZE];

    // Ignore the first reading in a series as it usually stands out
    // of the rest (an ADC peculiarity? an RC-filter feature?)
    analogRead(pin);

    // Collect the samples
    for (size_t i = 0; i < MEDIAN_SIZE; i++) {
        delay(2);
        buf[i] = analogRead(pin);
    }

    // Simple insertion sort
    for (size_t i = 1; i < MEDIAN_SIZE; i++) {
        uint key = buf[i];
        size_t j = i - 1;
        while (j >= 0 && buf[j] > key) {
            buf[j + 1] = buf[j];
            j--;
        }
        buf[j + 1] = key;
    }

    // Return the middle element
    return buf[MEDIAN_SIZE / 2];
}

//-----------------------------------------------------------------------------
// Kalman filter
//-----------------------------------------------------------------------------

KalmanFilterClass::KalmanFilterClass() :
    Q(COV_SCALE / 10), R(COV_SCALE), P(COV_SCALE), K(0), X(0) {}

void KalmanFilterClass::init(float q, float r, float p) {
    Q = q * COV_SCALE;
    R = r * COV_SCALE;
    P = p * COV_SCALE;
    K = 0;
    X = 0;
    _reinit = false;

    //Logger.printf(TRACE, F("[Kalman] Init Q=%u R=%u P=%u"), Q, R, P);
}

int16_t KalmanFilterClass::update(int16_t measurement) {
    // Prediction step
    P += Q;

    // Update step

    // K = P / (P + R)
    K = (uint16_t)(((uint64_t)P * GAIN_SCALE) / ((uint64_t)P + R));

    // X = X + K * (measurement - X)
    const int32_t error = (int32_t)measurement - X;
    X += ((int64_t)K * error) / GAIN_SCALE;

    // P = (1 - K) * P
    P = (uint32_t)(((uint64_t)P * (GAIN_SCALE - K)) / GAIN_SCALE);

    return (int16_t)X;
}
