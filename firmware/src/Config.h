#pragma once

#include <ArduinoJson.h>
#include <LittleFS.h>

class FileConfig {
  protected:
    const char *_fileName = "/config.json";
    File _file;

    class AlertConfig {
        uint16_t lo;
        uint16_t hi;
        uint16_t hyst;
    };

    struct ConfigStruct {
        uint16_t adcMinServ = 80; // Minimum acceptable ADC value for serviceable sensor

        // The linear scaler definition points
        uint16_t scalerAmin = 114;  // ADC reading corresponding to P_MIN
        uint16_t scalerAmax = 1024; // ADC reading corresponding to P_MAX
        uint16_t scalerPmin = 0;    // Minimum pressure, mbar
        uint16_t scalerPmax = 6245; // Maximum pressure, mbar

        // Filters
        uint8_t filtSamps = 32; // Number of samples in one oversampling cycle
        uint8_t filtAlpha = 8;  // Pressure EMA filter alpha

        // Pressure alert thresholds
        uint16_t alertLo = scalerPmin; // Low pressure
        uint16_t alertHi = scalerPmax; // High pressure
        uint16_t alertHyst = 50;       // Back to normal hysteresis
    } _config;

    const ConfigStruct _config_def = _config;
    bool _dirty = false;

  protected:
    void _setDefaults();

  public:
    FileConfig();
    bool load();
    bool save(bool force = false);
    bool reset();
    bool remove();
    String print();

    // Getters
    inline uint16_t adcMinServ() const { return _config.adcMinServ; }
    inline uint16_t scalerAmin() const { return _config.scalerAmin; }
    inline uint16_t scalerAmax() const { return _config.scalerAmax; }
    inline uint16_t scalerPmin() const { return _config.scalerPmin; }
    inline uint16_t scalerPmax() const { return _config.scalerPmax; }
    inline uint8_t filtSamps() const { return _config.filtSamps; }
    inline uint8_t filtAlpha() const { return _config.filtAlpha; }
    inline uint16_t alertLo() const { return _config.alertLo; };
    inline uint16_t alertHi() const { return _config.alertHi; };
    inline uint16_t alertHyst() const { return _config.alertHyst; };

    // Setters
    bool adcMinServ(uint16_t value);
    bool scalerAmin(uint16_t value);
    bool scalerAmax(uint16_t value);
    bool scalerPmin(uint16_t value);
    bool scalerPmax(uint16_t value);
    bool filtSamps(uint8_t value);
    bool filtAlpha(uint8_t value);
    bool alertLo(uint16_t value);
    bool alertHi(uint16_t value);
    bool alertHyst(uint8_t value);
};
