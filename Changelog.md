# Changelog

## Firmware

### 1.1.0 (2026-09-08)
* Reworked filtering:
  * The simple average filter replaced with the median filter
  * Added the Kalman filter
  * Added two additional Modbus input registers for debugging - 
    the ADC readings after the median filter (raw) and after
	the Kalman filter (to be scaled).
  * The filters moved from functions.h to Filters.h/Filters.cpp.
  * scale() moved from functions.h to main.cpp.
  * Removed functions.h.
  * Removed the test scaler command from the Telnet server.

### 1.0.0 (2026-09-02)
* The first release

## Home Assistant Integration

### 1.0.0 (2026-09-02)
* The first release

## Home Assistant Automation

### 1.0.0 (2026-09-02)
* The first release
