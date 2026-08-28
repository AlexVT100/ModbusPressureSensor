#include <Terminal.h>

#include "ModbusHelper.h"
#include "States.h"
#include "Config.h"
#include "TerminalLogger.h"

extern TerminalLogger Logger;
extern FileConfig Conf;

//=============================================================================
// The Modbus Class
//  * Sends pressure (mbar) to the clients via the input register
//  * Keeps pressure thresholds for alerting in two holding
//    registers and stores them in the configuration.
//  * Monitors Modbus slave connections
//
// The underlying Modbus_ESP8266 library:
//  * https://github.com/emelianov/modbus-esp8266
//  * https://github.com/emelianov/modbus-esp8266/blob/master/documentation/API.md
//=============================================================================
//
ModbusHelperClass::ModbusHelperClass() : _modbus() {
    _onConnectFunc = [this](IPAddress ip) { return this->_onConnect(ip); };
    _onDisconnectFunc = [this](IPAddress ip) { return this->_onDisconnect(ip); };
    _onSetHregFunc = [this](TRegister *reg, uint16_t val) { return this->_onSetHreg(reg, val); };
}

void ModbusHelperClass::setup() {

    // Start the server on the default port 502
    _modbus.server();

    // Add registers
    _modbus.addIreg(regAddr::PRESS, 0);
    _modbus.addIreg(regAddr::STATUS, static_cast<uint16_t>(Status::NORMAL));
    _modbus.addHreg(regAddr::LO_THRESH, Conf.alertLo());
    _modbus.addHreg(regAddr::HI_THRESH, Conf.alertHi());

    // Set callback to monitor updates of the registers
    _modbus.onSetHreg(regAddr::LO_THRESH, _onSetHregFunc, 2);

    // Set callbacks to count connected clients
    _modbus.onConnect(_onConnectFunc);
    _modbus.onDisconnect(_onDisconnectFunc);

    _modbus.cbEnable();
}

void ModbusHelperClass::loop() {
    _modbus.task();

    lowLimit(Conf.alertLo());
    highLimit(Conf.alertHi());

    //modbus.task();
}

//-----------------------------------------------------------------------------
// Process the changes of the limits.
//      Implements the core functionality of the class.
//
//      When the Hreg register holding a limit changes,
//      the function validates the new value and stores it in EEPROM.
//
//      This callback is called on any change of a Hreg register,
//      both by the server itself or by the connected client
//-----------------------------------------------------------------------------
//
uint16_t ModbusHelperClass::_onSetHreg(TRegister *reg, uint16_t value) {

    if (reg->value == value) return reg->value;             // The value was not changed
    if (_modbus.eventSource() == uint32_t(-1)) return value; // The value was changed locally -- the change came from Conf

    // The vaue was changed by a remote client
    if (reg->address.address == regAddr::LO_THRESH) return Conf.alertLo(value) ? value : reg->value;
    if (reg->address.address == regAddr::HI_THRESH) return Conf.alertHi(value) ? value : reg->value;

    // Logger.println(ERROR, "Cannot change limit: wrong address %u\n", reg->address.address);
    return reg->value; // Do not allow to change the value
}

bool ModbusHelperClass::_onConnect(IPAddress ip) {
    _connCount++;
    Logger.printf(INFO, F("[Modbus] Client connected: %u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);
    return true;
}

bool ModbusHelperClass::_onDisconnect(IPAddress ip) {
    _connCount--;
    Logger.println(INFO, F("[Modbus] Client disconnected"));
    return true;
}
