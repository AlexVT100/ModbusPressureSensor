#include "scaler.h"

//-----------------------------------------------------------------------------
// A linear value conversion
//
// Requires:
//      inMin != inMax (violation will result in division by zero)
//      inMax > inMin
//      outMax >= outMin
//-----------------------------------------------------------------------------
//
int16_t scale(uint16_t value, uint16_t inMin, uint16_t inMax, uint16_t outMin, uint16_t outMax) {
    const int32_t offset   = int32_t(value) - inMin;
    const int32_t inRange  = int32_t(inMax) - inMin;   // always > 0
    const int32_t outRange = int32_t(outMax) - outMin; // may be negative (inverted mapping)

    const int32_t num  = offset * outRange;
    const int32_t half = inRange / 2;

    // round-to-nearest, ties away from zero
    const int32_t rounded = (num >= 0) ? (num + half) : (num - half);

    return int16_t(rounded / inRange + outMin);
}
