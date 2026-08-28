#pragma once

enum class Status {
    NORMAL = 0,
    LO_PRESS = 1,
    HI_PRESS = 2,
    FAILURE = -1
};

struct SensorState {
    uint adcValue = 0;
    uint pressure = 0;
    int pressRaw = 0;
    Status status = Status::NORMAL;
};

struct DisplayState {
    bool isOn = true;
    bool setOn = false;
};
 
// Returns sensor status as human-readable string
inline const char *statusStr(Status status) {
    switch (status) {
        case Status::NORMAL:
            return "Normal";
        case Status::LO_PRESS:
            return "Pressure too low";
        case Status::HI_PRESS:
            return "Pressure too high";
        case Status::FAILURE:
            return "Failure";
        default:
            return "Unknown";
    }
}
