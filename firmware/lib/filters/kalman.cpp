#include "kalman.h"

KalmanFilterClass::KalmanFilterClass() :
    Q(Q1616_ONE / 10), // 0.1
    R(Q1616_ONE),      // 1.0
    P(Q1616_ONE),      // 1.0
    K(0),
    X(0) {}

//-----------------------------------------------------------------------------
// Change the covariance values Q, R and P
//-----------------------------------------------------------------------------
bool KalmanFilterClass::setCovs(float q, float r, float p) {
    if (q < MIN_COV || r < MIN_COV || p < MIN_COV) return false;

    Q = float_to_q16_16(q);
    R = float_to_q16_16(r);
    P = float_to_q16_16(p);
    if (P < Q1616_MIN) P = Q1616_MIN;

    K = 0;
    // X = 0;   // Uncomment to reset the current state as well
    _covsChanged = false;

    return true;
}

//-----------------------------------------------------------------------------
// Pass the measurement through the filter
//-----------------------------------------------------------------------------
int32_t KalmanFilterClass::update(int16_t measurement) {
    // Passthrough mode: R == 0 means measurements are perfect.
    if (R == 0) {
        X = int_to_sq16_16(measurement);
        P = 0;
        return measurement;
    }

    // Prediction step (P becomes predicted covariance)
    P += Q;

    // K = P / (P + R)
    K = ratio_to_q15(P, P + R);

    // Innovation (Q16.16)
    const sq16_16 error = int_to_sq16_16(measurement) - X;

    // X = X + K * (measurement - X)
    X += mul_sq16_16_q15(error, K);

    // P = (1 - K) * P
    P = mul_uq16_16_q15(P, (q15)(Q15_ONE - K));
    if (P < Q1616_MIN) P = Q1616_MIN;

    // Round back to integer for the caller
    return sq16_16_to_int(X);
}