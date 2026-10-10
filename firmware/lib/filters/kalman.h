#pragma once

// ---------------------------------------------------------------------------
// Fixed-point types
// ---------------------------------------------------------------------------
using q16_16  = uint32_t; // unsigned Q16.16 (Q, R)
using uq16_16 = uint64_t; // unsigned Q16.16, wide (P: needs headroom for P + Q)
using sq16_16 = int64_t;  // signed   Q16.16, wide (X, errors)
using q15     = uint16_t; // unsigned Q15 (Kalman gain, range 0..1.0)

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
//
constexpr uint32_t Q1616_FRAC_BITS = 16;
constexpr uint32_t Q15_FRAC_BITS   = 15;
constexpr uint32_t Q1616_ONE       = 1u << Q1616_FRAC_BITS; // 65536
constexpr uint32_t Q15_ONE         = 1u << Q15_FRAC_BITS;   // 32768
constexpr q16_16   Q1616_MIN       = 1;                     // smallest meaningful value

// ---------------------------------------------------------------------------
// Kalman filter
// ---------------------------------------------------------------------------
//
class KalmanFilterClass {
  public:
    static constexpr float MIN_COV = 0.0f;     // Minimum covariance value
    static constexpr float MAX_COV = 65535.0f; // Maximum covariance value

  private:
    q16_16  Q; // Process noise variance (how fast the true value changes)
    q16_16  R; // Measurement noise variance (sensor noise level)
    uq16_16 P; // Estimation error covariance
    q15     K; // Kalman gain
    sq16_16 X; // Current state estimate

    bool _covsChanged = false; // Q and/or R changed; need to call setCovs() in loop()

  public:
    KalmanFilterClass();
    bool    setCovs(float q, float r, float p = 1.0f);
    bool    covsChanged(bool changed = false) { return changed ? (_covsChanged = true) : _covsChanged; }
    int32_t update(int16_t measurement);

  private:
    // ---------------------------------------------------------------------------
    // Conversion / arithmetic helpers
    // ---------------------------------------------------------------------------

    // float -> unsigned Q16.16 (clamped to 0 .. MAX_COV)
    constexpr q16_16 float_to_q16_16(float v) {
        return (q16_16)((v < MIN_COV ? MIN_COV : (v > MAX_COV ? MAX_COV : v)) * Q1616_ONE);
    }
    // Q16.16 -> float (handy for debugging)
    // constexpr float q16_16_to_float(q16_16 v) { return (float)v / Q1616_ONE; }

    // integer -> signed Q16.16
    constexpr sq16_16 int_to_sq16_16(int32_t v) { return (sq16_16)v * Q1616_ONE; }

    // signed integer division with symmetric rounding (half away from zero)
    constexpr int64_t round_div(int64_t num, int64_t den) {
        return ((num >= 0) == (den >= 0)) ? (num + (den >= 0 ? den : -den) / 2) / den
                                          : (num - (den >= 0 ? den : -den) / 2) / den;
    }

    // signed Q16.16 -> nearest integer (symmetric rounding)
    constexpr int32_t sq16_16_to_int(sq16_16 v) { return (int32_t)round_div(v, Q1616_ONE); }

    // ratio num/den -> Q15 (den > 0, num <= den)
    constexpr q15 ratio_to_q15(uint64_t num, uint64_t den) { return (q15)((num * Q15_ONE + den / 2) / den); }

    // signed Q16.16 value * Q15 factor -> signed Q16.16 (symmetric rounding)
    constexpr sq16_16 mul_sq16_16_q15(sq16_16 v, q15 k) { return round_div(v * (int64_t)k, Q15_ONE); }

    // unsigned Q16.16 value * Q15 factor -> unsigned Q16.16 (rounded)
    constexpr uq16_16 mul_uq16_16_q15(uq16_16 v, q15 k) { return (v * k + Q15_ONE / 2) / Q15_ONE; }
};
