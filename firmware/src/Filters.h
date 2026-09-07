#pragma once

uint16_t ema_filter(uint16_t v, uint8_t alpha);
uint32_t median_filter(uint8_t pin);

class KalmanFilterClass {
  public:
    static constexpr uint32_t COV_SCALE = 65536;  // Q16.16
    static constexpr uint32_t GAIN_SCALE = 32768; // Q15

  private:
    uint32_t Q; // Process noise variance (how fast the true value changes), Q16.16
    uint32_t R; // Measurement noise variance (sensor noise level), Q16.16
    uint32_t P; // Estimation error covariance, Q16.16
    uint16_t K; // Kalman Gain, Q15
    int32_t X;  // Current state estimate

    static constexpr uint32_t S = 2 ^ 15; // Scale factor [2^15]
    bool _reinit = false;                 // Q and/or R changed; need to call init() in loop()

  public:
    KalmanFilterClass();
    void init(float q, float r, float p = 1.0);
    bool needsInit() { return _reinit; };
    void forceInit() { _reinit = true; };
    int16_t update(int16_t measurement);
};