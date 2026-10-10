# Changelog

## Firmware

### 1.2.0 (2026-10-10)
* WiFi connection logic improvement:
  * Previously, if the configured WiFi network was unavailable when the sensor started up,
    the WiFi subsystem was switched to AP mode and deleted the saved Wi-Fi credentials.
    Now, if the sensor already has saved WiFi credentials, it waits up to 5 minutes to
    connect and then restarts. It switches to AP mode only when the button is pressed
    or when no credentials are saved.
* Code reorganization:
  * The filters and the scaler have been moved to the separate files in lib/.
* Kalman filter improvements:
  * Renamed several methods.
  * Added checks for the covariance values.
  * Extended some variables to `uint64_t` to prevent potential overflow.
  * Changed `X` and `P` to use rounding instead of flooring.
* Added a unit test for Kalman filter.
* Telnet server optimization:
  * Joined commands `scale_in` and `scale_out` into single `scaler` with four parameters.
  * Removed custom GIT_REVISION and BUILD_TIME macros as they cause full rebuild on each build.
  
### 1.1.1 (2026-09-09)
* Optimized the telnet server:
	* Simplfied output of some telnet commands.
	* Removed duplicated output from the `config` command.
	* Optimized some command functions.

### 1.1.0 (2026-09-08)
* Reworked filtering:
  * The simple average filter replaced with the median filter.
  * Added the Kalman filter.
  * Added two additional Modbus input registers for debugging - the ADC readings after the median
    filter (raw) and after the Kalman filter (to be scaled).
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
