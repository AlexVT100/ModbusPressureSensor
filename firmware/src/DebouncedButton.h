class DebouncedButton {
  private:
    uint8_t _pin;
    unsigned long _debounceDelay;

    int _lastButtonState;
    int _stableButtonState;
    unsigned long _lastDebounceTime;

  public:
    // Constructor initializes variables and sets defaults
    DebouncedButton(uint8_t pin, unsigned long delayMs = 50) {
        _pin = pin;
        _debounceDelay = delayMs;
        _lastButtonState = HIGH;
        _stableButtonState = HIGH;
        _lastDebounceTime = 0;
    }

    // Must be called in setup()
    void begin() { pinMode(_pin, INPUT_PULLUP); }

    // Must be called continuously in loop(). Returns true ONLY on the exact press event frame.
    bool wasPressed() {
        int currentReading = digitalRead(_pin);
        bool pressDetected = false;

        // Reset timer if the physical pin noise shifts
        if (currentReading != _lastButtonState) _lastDebounceTime = millis();

        // Check if state has been stable long enough
        if ((millis() - _lastDebounceTime) > _debounceDelay) {
            if (currentReading != _stableButtonState) {
                _stableButtonState = currentReading;

                // If the newly verified state is LOW, it's a valid press
                if (_stableButtonState == LOW) pressDetected = true;
            }
        }

        _lastButtonState = currentReading;
        return pressDetected;
    }
};
