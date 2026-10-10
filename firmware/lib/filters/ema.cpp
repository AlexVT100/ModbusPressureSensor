#include "ema.h"

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
