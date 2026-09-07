#include <ModbusIP_ESP8266.h> // Modbus TCP

#include "States.h"

class ModbusHelperClass {
  protected:
    ModbusIP _modbus;
    unsigned _connCount = 0; // The number of connected clients
    std::function<bool(IPAddress)> _onConnectFunc;
    std::function<bool(IPAddress)> _onDisconnectFunc;
    std::function<uint16_t(TRegister *, uint16_t)> _onSetHregFunc;

  public:
    enum regAddr {
        // Ireg
        PRESS = 0,   // Pressure (mbar)
        STATUS = 1,  // Status
        ADC_RAW = 2, // Debug: raw ADC reading
        ADC = 3,     // Debug: filtered ADC reading
        // Hreg
        LO_THRESH = 0, // Low alert threshold
        HI_THRESH = 1  // High alert threshold
    };

  public:
    ModbusHelperClass();
    void setup();
    void loop();

    // Getters
    inline uint lowLimit() { return _modbus.Hreg(regAddr::LO_THRESH); }
    inline uint highLimit() { return _modbus.Hreg(regAddr::HI_THRESH); }

    // Setters
    inline void pressure(int pressure) { _modbus.Ireg(regAddr::PRESS, pressure); }
    inline void status(Status status) { _modbus.Ireg(regAddr::STATUS, static_cast<uint16_t>(status)); }
    inline void adcRaw(int value) { _modbus.Ireg(regAddr::ADC_RAW, value); }
    inline void adc(int value) { _modbus.Ireg(regAddr::ADC, value); }
    inline uint lowLimit(uint16_t value) { return _modbus.Hreg(regAddr::LO_THRESH, value); }
    inline uint highLimit(uint16_t value) { return _modbus.Hreg(regAddr::HI_THRESH, value); }

    inline unsigned connCount() { return _connCount; }

  protected:
    bool _onConnect(IPAddress ip);
    bool _onDisconnect(IPAddress ip);
    uint16_t _onSetHreg(TRegister *reg, uint16_t val);
};
