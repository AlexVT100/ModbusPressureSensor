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
        PRESS = 0,     // Ireg
        STATUS = 1,    // Ireg
        LO_THRESH = 0, // Hreg
        HI_THRESH = 1  // Hreg
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
    inline uint lowLimit(uint16_t value) { return _modbus.Hreg(regAddr::LO_THRESH, value); }
    inline uint highLimit(uint16_t value) { return _modbus.Hreg(regAddr::HI_THRESH, value); }

    inline unsigned connCount() { return _connCount; }

  protected:
    bool _onConnect(IPAddress ip);
    bool _onDisconnect(IPAddress ip);
    uint16_t _onSetHreg(TRegister *reg, uint16_t val);
};
