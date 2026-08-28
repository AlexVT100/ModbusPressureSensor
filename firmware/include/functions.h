#pragma once

//-----------------------------------------------------------------------------
// Integer division with rounding
//
// https://blog.demofox.org/2022/07/21/rounding-modes-for-integer-division/
//-----------------------------------------------------------------------------
//
inline int16_t div_round(int32_t a, int32_t b) { return (a ^ b) < 0 ? (a - b / 2) / b : (a + b / 2) / b; }

//-----------------------------------------------------------------------------
// A linear value conversion
//
// Requires:
//      inMin != inMax (violation will result in division by zero)
//      inMax > inMin
//      outMax >= outMin
//-----------------------------------------------------------------------------
//
// inline int16_t scale(int16_t value, int16_t inMin, int16_t inMax, int16_t outMin, int16_t outMax) {
//    return div_round(int32_t(value - inMin) * (outMax - outMin), int32_t(inMax - inMin)) + outMin;
//}

inline int16_t scale(
    uint16_t value,
    uint16_t inMin,
    uint16_t inMax,
    uint16_t outMin,
    uint16_t outMax)
{
    const int32_t offset   = int32_t(value) - inMin;
    const int32_t inRange  = int32_t(inMax) - inMin;
    const int32_t outRange = int32_t(outMax) - outMin;

    return int16_t(div_round(offset * outRange, inRange) + outMin);
}

//-----------------------------------------------------------------------------
// Exponential Moving Average (EMA) filter
//-----------------------------------------------------------------------------
//
// inline int ema_filter(int value, int alpha) {
//     static int expAvg = value;
//     expAvg = div_round(alpha * value + (100 - alpha) * expAvg, 100);
//     return expAvg;
// }

inline uint16_t ema_filter(uint16_t v, uint8_t alpha) {
    constexpr uint32_t scale = 256;
    static uint32_t state = uint32_t(v) * scale;

    int32_t delta = uint32_t(v) * scale - state;

    // Using intermediate int64_t to avoid signed/unsigned overflow problem when the input decreases
    state += (int64_t(alpha) * delta) / 100;

    return (state + scale / 2) / scale;
}

//-----------------------------------------------------------------------------
// Simple Moving Average filter
//-----------------------------------------------------------------------------
//
inline int sma_filter(int value) {
    static const uint winSize = 10;
    static int readings[winSize];
    static uint index = 0;
    static uint count = 0;
    static int sum = 0.0;

    if (count < winSize) {
        readings[index] = value;
        sum += value;
        count++;
    } else {
        sum -= readings[index];
        readings[index] = value;
        sum += value;
    }
    index = (index + 1) % winSize;

    return div_round(sum, count);
}
