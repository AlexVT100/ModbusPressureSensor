#include "Config.h"
#include "TerminalLogger.h"

extern TerminalLogger Logger;

//-----------------------------------------------------------------------------
// Default constructor
//-----------------------------------------------------------------------------
//
FileConfig::FileConfig() {
    _setDefaults(); // Set default values in case if the config do not exists or won't load
    _dirty = false; // Do not invalidate

    if (!LittleFS.begin()) {
        // What to do next?
        Serial.println("[Config] Failed to mount LittleFS");
        return;
    }
}

//-----------------------------------------------------------------------------
// (Re)set the configuration to default values
//-----------------------------------------------------------------------------
//
void FileConfig::_setDefaults() {
    memcpy(&_config, &_config_def, sizeof(ConfigStruct));
    _dirty = true;
}

//-----------------------------------------------------------------------------
// Load the configuration from the file
//-----------------------------------------------------------------------------
//
bool FileConfig::load() {
    // Check if the file exists
    if (!LittleFS.exists(_fileName)) {
        Serial.println("[Config] No config file found. Using defaults.");
        _setDefaults();
        _dirty = true; // Force save() to save the file
        return false;
    }

    // Open the file
    _file = LittleFS.open(_fileName, "r");
    if (!_file) {
        Serial.println("[Config] Failed to open config file. Using defaults.");
        _setDefaults();
        _dirty = true; // Force save() to save the file
        return false;
    }

    // Deserialize
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, _file);
    _file.close();

    if (error) {
        Serial.println("[Config] Failed to parse config file. Using defaults.");
        _setDefaults();
        return false;
    }

    // Load the values into the structure
    _config.adcMinServ = doc["adcMinServ"] | _config_def.adcMinServ;

    _config.scalerAmin = doc["scaler"]["Amin"] | _config_def.scalerAmin;
    _config.scalerAmax = doc["scaler"]["Amax"] | _config_def.scalerAmax;
    _config.scalerPmin = doc["scaler"]["Pmin"] | _config_def.scalerPmin;
    _config.scalerPmax = doc["scaler"]["Pmax"] | _config_def.scalerPmax;

    _config.filtSamps = doc["filter"]["Samps"] | _config_def.filtSamps;
    _config.filtAlpha = doc["filter"]["Alpha"] | _config_def.filtAlpha;

    _config.alertLo = doc["alert"]["Lo"] | _config_def.alertLo;
    _config.alertHi = doc["alert"]["Hi"] | _config_def.alertHi;
    _config.alertHyst = doc["alert"]["Hyst"] | _config_def.alertHyst;

    Serial.println("[Config] Config file loaded");
    return true;
}

//-----------------------------------------------------------------------------
// Save the current settings
//-----------------------------------------------------------------------------
//
bool FileConfig::save(bool force) {
    if (!force && !_dirty) return true;

    // Open the file
    _file = LittleFS.open(_fileName, "w");
    if (!_file) {
        Logger.println(ERROR, F("[Config] Failed to open config file for writing"));
        return false;
    }

    // Create the Json document from the structure
    JsonDocument doc;
    doc["adcMinServ"] = _config.adcMinServ;

    doc["scaler"]["Amin"] = _config.scalerAmin;
    doc["scaler"]["Amax"] = _config.scalerAmax;
    doc["scaler"]["Pmin"] = _config.scalerPmin;
    doc["scaler"]["Pmax"] = _config.scalerPmax;

    doc["filter"]["Samps"] = _config.filtSamps;
    doc["filter"]["Alpha"] = _config.filtAlpha;

    doc["alert"]["Lo"] = _config.alertLo;
    doc["alert"]["Hi"] = _config.alertHi;
    doc["alert"]["Hyst"] = _config.alertHyst;

    // Serialize into the file
    if (serializeJsonPretty(doc, _file) == 0) {
        Logger.println(ERROR, F("[Config] Failed to serialize to JSON document"));
        _file.close();
        return false;
    }

    _file.close();

    _dirty = false;
    Logger.println(INFO, F("[Config] Config file saved"));
    return true;
}

//-----------------------------------------------------------------------------
// Reset the configuration to defaults
//-----------------------------------------------------------------------------
//
bool FileConfig::reset() {
    _setDefaults(); // sets _dirty flag so the config will be saved in the end of loop()
    Logger.println(INFO, F("[Config] Config reset to defaults"));
    return true;
}

//-----------------------------------------------------------------------------
// Remove the configuration file
//-----------------------------------------------------------------------------
//
bool FileConfig::remove() {

    if (!LittleFS.remove(_fileName)) {
        Logger.println(ERROR, F("[Config] Failed to remove config file"));
        return false;
    }

    _dirty = true; // force the config to be saved in the end of loop()
    Logger.println(INFO, F("[Config] Config file removed"));
    return true;
}

//-----------------------------------------------------------------------------
// Print the file contents
//-----------------------------------------------------------------------------
//
String FileConfig::print() {
    // Check if the file exists
    if (!LittleFS.exists(_fileName)) {
        Logger.println(ERROR, F("[Config] No config file found"));
        return "";
    }

    // Open the file
    _file = LittleFS.open(_fileName, "r");
    if (!_file) {
        Logger.println(ERROR, F("[Config] Failed to open config file"));
        return "";
    }

    return _file.readString();
}

//=============================================================================
// Setters
//=============================================================================

//-----------------------------------------------------------------------------
// Change ADCmin
//-----------------------------------------------------------------------------
//
bool FileConfig::scalerAmin(uint16_t value) {
    if (value == _config.scalerAmin) {
        Logger.printf(WARNING, F("[Config] ADCmin left unchanged (%u)"), value);
        return false;
    }

    if (value > 1023) {
        Logger.println(ERROR, F("[Config] ADCmin must be between 0 and 1023"));
        return false;
    }
    if (value >= _config.scalerAmax) {
        Logger.printf(ERROR, F("[Config] ADCmin must be less than ADCmax (%u)"), _config.scalerAmax);
        return false;
    }
    _config.scalerAmin = value;
    Logger.printf(INFO, F("[Config] ADCmin set to %u"), value);

    _dirty = true;
    return true;
};

//-----------------------------------------------------------------------------
// Change ADCmax
//-----------------------------------------------------------------------------
//
bool FileConfig::scalerAmax(uint16_t value) {
    if (value == _config.scalerAmax) {
        Logger.printf(WARNING, F("[Config] ADCmax left unchanged (%u)"), value);
        return false;
    }

    if (value > 1024) {
        Logger.println(ERROR, F("[Config] ADCmax must be between 0 and 1024"));
        return false;
    }
    if (value <= _config.scalerAmin) {
        Logger.printf(ERROR, F("[Config] ADCmax must be greater than ADCmin (%u)"), _config.scalerAmin);
        return false;
    }

    _config.scalerAmax = value;
    Logger.printf(INFO, F("[Config] ADCmax set to %u"), value);

    _dirty = true;
    return true;
};

//-----------------------------------------------------------------------------
// Change Pmin
//-----------------------------------------------------------------------------
//
bool FileConfig::scalerPmin(uint16_t value) {
    if (value == _config.scalerPmin) {
        Logger.printf(WARNING, F("[Config] Pmin left unchanged (%u)"), value);
        return false;
    }

    if (value >= _config.scalerPmax) {
        Logger.printf(ERROR, F("[Config] Pmin must be less than Pmax (%u)"), _config.scalerPmax);
        return false;
    }

    // Adjust low alert if becomes less than Pmin
    if (alertLo() < value) alertLo(value);

    _config.scalerPmin = value;
    Logger.printf(INFO, F("[Config] Pmin set to %u"), value);

    _dirty = true;
    return true;
};

//-----------------------------------------------------------------------------
// Change Pmax
//-----------------------------------------------------------------------------
//
bool FileConfig::scalerPmax(uint16_t value) {
    if (value == _config.scalerPmax) {
        Logger.printf(WARNING, F("[Config] Pmax left unchanged (%u)"), value);
        return false;
    }

    if (value <= _config.scalerPmin) {
        Logger.printf(ERROR, F("[Config] Pmax must be greater than Pmin (%u)"), _config.scalerPmin);
        return false;
    }

    // Adjust high alert if becomes greater than Pmax
    if (alertHi() > value) alertHi(value);

    _config.scalerPmax = value;
    Logger.printf(INFO, F("[Config] Pmax set to %u"), value);

    _dirty = true;
    return true;
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
//
bool FileConfig::filtSamps(uint8_t value) {
    if (value == _config.filtSamps) {
        Logger.printf(WARNING, F("[Config] Number of samples left unchanged (%u)"), value);
        return false;
    }

    if (value > 64) {
        Logger.println(ERROR, F("[Config] Number of samples must be between 1 and 64"));
        return false;
    }

    _config.filtSamps = value;
    Logger.printf(INFO, F("[Config] Number of samples set to %u"), value);

    _dirty = true;
    return true;
};

//-----------------------------------------------------------------------------
// Change EMA filter alpha
//-----------------------------------------------------------------------------
//
bool FileConfig::filtAlpha(uint8_t value) {
    if (value == _config.filtAlpha) {
        Logger.printf(WARNING, F("[Config] EMA alpha left unchanged (%u)"), value);
        return false;
    }

    if (value > 100) {
        Logger.println(ERROR, F("[Config] EMA alpha must be between 1 and 100"));
        return false;
    }

    _config.filtAlpha = value;
    Logger.printf(INFO, F("[Config] EMA alpha set to %u"), value);

    _dirty = true;
    return true;
};

//-----------------------------------------------------------------------------
// Set the low alert threshold
//-----------------------------------------------------------------------------
//
bool FileConfig::alertLo(uint16_t value) {
    if (value == _config.alertLo) {
        Logger.printf(WARNING, F("[Config] Low threshold left unchanged (%u)"), value);
        return false;
    }

    if (value > alertHi()) {
        Logger.printf(ERROR, F("[Config] New low threshold %u is higher than the high one %u"), value, alertHi());
        return false;
    }
    if (value > scalerPmax()) {
        Logger.printf(ERROR, F("[Config] New low threshold %u is higher than Pmax (%u)"), value, scalerPmax());
        return false;
    }

    _config.alertLo = value;
    Logger.printf(INFO, F("[Config] Low threshold set to %u mbar"), value);

    _dirty = true;
    return true;
};

//-----------------------------------------------------------------------------
// Set the high alert threshold
//-----------------------------------------------------------------------------
//
bool FileConfig::alertHi(uint16_t value) {
    if (value == _config.alertHi) {
        Logger.printf(WARNING, F("[Config] High threshold left unchanged (%u mbar)"), value);
        return false;
    }

    if (value < _config.alertLo) {
        Logger.printf(ERROR, F("[Config] New high limit %u is lower than the low limit %u"), value, _config.alertLo);
        return false;
    }
    if (value > _config.scalerPmax) {
        Logger.printf(ERROR, F("[Config] New high limit %u is higher than the maximum pressure (%u)"), value,
                      _config.scalerPmax);
        return false;
    }

    _config.alertHi = value;
    Logger.printf(INFO, F("[Config] High threshold set to %u mbar"), value);

    _dirty = true;
    return true;
};

//-----------------------------------------------------------------------------
// Set the back-to-normal alert hysteresis
//-----------------------------------------------------------------------------
//
bool FileConfig::alertHyst(uint8_t value) {
    if (value == _config.alertHyst) {
        Logger.printf(WARNING, F("[Config] Alert hysteresis left unchanged (%u mbar)"), value);
        return false;
    }

    _config.alertHyst = value;
    Logger.printf(INFO, F("[Config] Alert hysteresis set to %u mbar"), value);

    _dirty = true;
    return true;
};

//-----------------------------------------------------------------------------
// Set the minimum ADC value at which the sensor is considered serviceable
//-----------------------------------------------------------------------------
//
bool FileConfig::adcMinServ(uint16_t value) {
    if (value == _config.adcMinServ) {
        Logger.printf(WARNING, F("[Config] ADCminServ left unchanged (%u mbar)"), value);
        return false;
    }

    if (value >= _config.scalerAmin) {
        Logger.printf(ERROR, F("[Config] New ADCminServ %u is greater than ADCmin (%u)"), value, _config.scalerAmin);
        return false;
    }

    _config.adcMinServ = value;
    Logger.printf(INFO, F("[Config] ADCminServ set to %u"), value);

    _dirty = true;
    return true;
};
