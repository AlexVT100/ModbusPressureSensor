// https://github.com/johngavel/Terminal

#include "TelnetServer.h"
#include "Config.h"
#include "States.h"
#include "WiFiHelper.h"
#include "functions.h"

#define STATIC

// The globals accessed by Telnet
extern SensorState Sensor;
extern DisplayState DispState;
// extern bool SetDisplayOn;
extern unsigned long RolloverCount;
extern FileConfig Conf;

void _banner(OutputInterface *terminal) {
    terminal->println(PROMPT, "\nPressure Sensor " + String(ESP.getChipId(), HEX));
}

TelnetServer::TelnetServer(uint port) :
    Terminal(&_client), _server(port) {
    _cmdExitFunc = [this](OutputInterface *terminal) { return this->cmdExit(terminal); };
    _cmdRestartFunc = [this](OutputInterface *terminal) { return this->cmdRestart(terminal); };
}

void TelnetServer::setup() {
    // Setup the terminal
    Terminal::setup();

    // Start the WiFi server
    _server.begin();
    //_server.setNoDelay(true);
    Serial.println("TCP server started. Status=" + String(_server.status()));

    // Session defaults
    // A clent's telnet must be set to character mode (e.g. in the classic Linux telnet, set "mode character")
    setColor(true);
    setPromptString(">");
    setPrompt(true);
    setBannerFunction(_banner);
    setEcho(true);

    // Set up commands
    addStandardTerminalCommands(TERM_CMD);
    TERM_CMD->addCmd("exit", "", "Close the session", _cmdExitFunc);
    TERM_CMD->addCmd("restart", "", "Restart the device", _cmdRestartFunc);
    TERM_CMD->addCmd("config", "[save|reset]", "Show/save/remove config file", cmdConfig);
    TERM_CMD->addCmd("min-in", "[N]", "Get/set minimum acceptable ADC value for serviceable sensor", cmdMinADC);
    TERM_CMD->addCmd("scale-in", "[min|max [N]]", "Get/set scaler ADC values (input)", cmdScalerADC);
    TERM_CMD->addCmd("scale-out", "[min|max [N]]", "Get/set scaler pressure values (output)", cmdScalerPress);
    TERM_CMD->addCmd("filter", "[ns|apha [N]]", "Get/set the filter parameters", cmdFilter);
    TERM_CMD->addCmd("alert", "[lo|hi|hyst [N]]", "Get/set pressure alerting thresholds", cmdAlerts);
    TERM_CMD->addCmd("sensor", "", "Get the sensor readings", cmdSensor);
    TERM_CMD->addCmd("display", "", "Turn on display", cmdDisp);
    TERM_CMD->addCmd("sysinfo", "", "Get the system info", cmdSys);
    TERM_CMD->addCmd("scale", "adc|press N", "Test the scaler settings", cmdTestScale);
}

//-----------------------------------------------------------------------------
// Process server events in loop()
//-----------------------------------------------------------------------------
//
void TelnetServer::loop() {
    // Check if the connected client (if any) got disconnected
    if (_isConnected && !_client.connected()) {
        Serial.println("Telnet client disconnected");
        _isConnected = false;
    }

    // Check if there is a new connection
    WiFiClient newClient = _server.accept();
    if (newClient) {
        // A new client is trying to connect
        Serial.println("[Telnet] Client " + newClient.remoteIP().toString() + " connected");
        if (_client.connected()) {
            // Another client is already connected
            newClient.println("Too many connections. Bye!");
            newClient.stop();
            Serial.println("[Telnet] Client diconnected - only 1 connection is allowed");
        } else {
            // Accept the client
            _client = newClient;
            // setStream(&_client);

            //_client.flush();

            _client.setNoDelay(true);
            _client.keepAlive(600, 60, 10);

            _isConnected = true;

            banner();
            prompt();
        }
    }

    if (_client.connected()) {
        Terminal::loop();
        _client.flush();
    }
}

//=============================================================================
// Static helper functions
//=============================================================================

//-----------------------------------------------------------------------------
// Calculates the uptime and returns it as a readable string
//-----------------------------------------------------------------------------
//
static const char *uptime(unsigned long rolloverCount) {
    // Calculate total seconds passed, factoring in rollovers
    // 4294967295 is the maximum value of an unsigned long (i.e. value of millis())
    unsigned long long totalSeconds = ((unsigned long long)rolloverCount * 4294967296ULL + millis()) / 1000ULL;

    int seconds = totalSeconds % 60;
    int minutes = (totalSeconds / 60) % 60;
    int hours = (totalSeconds / 3600) % 24;
    int days = totalSeconds / 86400;

    static char buffer[24];
    snprintf(buffer, sizeof(buffer), "%dd %02d:%02d:%02d", days, hours, minutes, seconds);
    return buffer;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
//
// STATIC void TelnetServer::printf(OutputInterface *t, PRINT_TYPES type, const char *fmt, ...) {
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buffer, sizeof(buffer), fmt, args);
//     va_end(args);

//     t->println(type, buffer);
// }

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
//
STATIC void TelnetServer::printf(OutputInterface *t, PRINT_TYPES type, const __FlashStringHelper *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf_P(buffer, sizeof(buffer), (PGM_P)fmt, args);
    va_end(args);

    t->println(type, buffer);
}

// STATIC void TelnetServer::printf(OutputInterface *t, COLOR color, const __FlashStringHelper *fmt, ...) {
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf_P(buffer, sizeof(buffer), (PGM_P)fmt, args);
//     va_end(args);

//     t->print(color, buffer);
//     t->println();
// }

//=============================================================================
// Telnet Commands
//=============================================================================

//-----------------------------------------------------------------------------
// Read and identify a command parameter
//
// Arguments:
//      terminal    Terminal to print error to
//      params      The list of the acceptable parameter names
//
// Return value:
//      0-based index of the parameter in the list if found
//      -1 if the parameter is not found in the list
//-----------------------------------------------------------------------------
//
STATIC int TelnetServer::_readParam(OutputInterface *terminal, std::initializer_list<const char *> params) {
    // Read the parameter. Assume the empty string if it's omitted
    const char *param = terminal->readParameter();
    if (param == NULL) param = "";

    // Search for the parameter in the list
    for (size_t i = 0; i < params.size(); ++i)
        if (strcmp(param, params.begin()[i]) == 0) return i;

    // The command is not found
    terminal->println(ERROR, F("Wrong parameter"));
    return -1;
}

//-----------------------------------------------------------------------------
// Read and identify a command parameter with a value.
// Like the previous function but also reads an optional uint value from the input.
//
//  Arguments:
//      terminal    Terminal to print error to
//      params      The list of the acceptable parameter names
//      value       The value specified in the input or VAL_UNSET if not specified.
//
//  Return value:
//      See the previuos function
//-----------------------------------------------------------------------------
//
STATIC int TelnetServer::_readParam(OutputInterface *terminal, std::initializer_list<const char *> params,
                                    uint &value) {
    // Process the parameter
    int ci = _readParam(terminal, params);
    if (ci == -1) return ci; // Failed

    // Get the value
    if (_readValue(terminal, value)) return ci;

    // Value error
    return -1;
}

//-----------------------------------------------------------------------------
// Read a value from the command
//-----------------------------------------------------------------------------
//
STATIC bool TelnetServer::_readValue(OutputInterface *terminal, uint &value) {
    // Read the value
    const char *param = terminal->readParameter();
    if (param == NULL) {
        // No value specified
        value = VAL_UNSET;
        return true;
    }

    // Convert the value to an integer
    char *endPtr;
    long svalue = std::strtol(param, &endPtr, 10);
    if (param != endPtr && svalue >= 0) {
        // accept only non-negative numbers
        value = svalue;
        return true;
    }

    // Report the value error
    terminal->println(ERROR, F("Value must be a positive number"));
    return false;
}

//-----------------------------------------------------------------------------
// Exit the telnet session
//-----------------------------------------------------------------------------
//
void TelnetServer::cmdExit(OutputInterface *terminal) {
    if (terminal == this) {
        terminal->println(WARNING, F("Closing Telnet Session...."));
        delay(100);
        _client.stop();
    } else {
        terminal->println(ERROR, F("Not Supported on this terminal."));
        terminal->prompt();
    }
}

//-----------------------------------------------------------------------------
// Restart the unit
//-----------------------------------------------------------------------------
//
void TelnetServer::cmdRestart(OutputInterface *terminal) {
    terminal->println(WARNING, F("Restarting the device"));
    _client.stop();
    delay(1000);
    ESP.restart();
}

//-----------------------------------------------------------------------------
// Config file manupulations
//-----------------------------------------------------------------------------
//
STATIC void TelnetServer::cmdConfig(OutputInterface *term) {
    switch (_readParam(term, {"", "save", "reset"})) {
        case 0: // no parameter
            term->println(Conf.print());
            term->println();

            term->println(F("Scaling parameters:"));
            printf(term, INFO, F("  Input (ADC): %4u...%4u"), Conf.scalerAmin(), Conf.scalerAmax());
            printf(term, INFO, F("   Output (P): %4u...%4u mbar"), Conf.scalerPmin(), Conf.scalerPmax());
            printf(term, INFO, F("      Min ADC: %4u"), Conf.adcMinServ());
            term->println(F("Filters:"));
            printf(term, INFO, F("  ADC Samples: %u"), Conf.filtSamps());
            printf(term, INFO, F("    EMA Alpha: %u"), Conf.filtAlpha());
            term->println(F("Pressure alerts:"));
            printf(term, INFO, F("          Low: %u mbar"), Conf.alertLo());
            printf(term, INFO, F("         High: %u mbar"), Conf.alertHi());
            printf(term, INFO, F("   Hysteresis: %u mbar"), Conf.alertHyst());
            break;
        case 1: // save
            Conf.save(true);
            break;
        case 2: // reset
            Conf.remove();
    }
    term->prompt();
}

//-----------------------------------------------------------------------------
// Show the current pressure and the ADC reading
//-----------------------------------------------------------------------------
//
STATIC void TelnetServer::cmdSensor(OutputInterface *term) {
    printf(term, INFO, F("     Pressure: %u mbar"), Sensor.pressure);
    printf(term, INFO, F("\e[38;5;246m Raw pressure: %d mbar\033[0m"), Sensor.pressRaw);
    printf(term, INFO, F("    ADC value: %u"), Sensor.adcValue);
    printf(term, INFO, F("       Status: %s"), statusStr(Sensor.status));

    term->prompt();
}

//-----------------------------------------------------------------------------
// Switch the display on (emulates the button pressing)
//-----------------------------------------------------------------------------
//
STATIC void TelnetServer::cmdDisp(OutputInterface *terminal) {
    DispState.setOn = true;
    terminal->prompt();
}

//-----------------------------------------------------------------------------
// Show/change the minimum ADC value at which the sensor is considered serviceable
//-----------------------------------------------------------------------------
//
STATIC void TelnetServer::cmdMinADC(OutputInterface *term) {
    uint value;

    if (!_readValue(term, value)) return;

    if (value == VAL_UNSET)
        printf(term, INFO, F("ADCMinServ is %u"), Conf.adcMinServ());
    else if (Conf.adcMinServ(value))
        printf(term, INFO, F("ADCMinServ set to %u"), Conf.adcMinServ());
    else
        term->println(WARNING, F("ADCMinServ value left unchanged"));

    term->prompt();
}

//-----------------------------------------------------------------------------
// Show/change the input ADC values for the scaler
//-----------------------------------------------------------------------------
//
STATIC void TelnetServer::cmdScalerADC(OutputInterface *term) {
    uint value;

    switch (_readParam(term, {"", "min", "max"}, value)) {
        case 0: // no parameter
            printf(term, INFO, F("Amin is %u"), Conf.scalerAmin());
            printf(term, INFO, F("Amax is %u"), Conf.scalerAmax());
            break;
        case 1: // min
            if (value == VAL_UNSET)
                printf(term, INFO, F("Amin is %u"), Conf.scalerAmin());
            else
                Conf.scalerAmin(value);
            break;
        case 2: // max
            if (value == VAL_UNSET)
                printf(term, INFO, F("Amax is %u"), Conf.scalerAmax());
            else
                Conf.scalerAmax(value);
            break;
    }

    term->prompt();
}

//-----------------------------------------------------------------------------
// Show/change the output pressure values for the scaler
//-----------------------------------------------------------------------------
//
STATIC void TelnetServer::cmdScalerPress(OutputInterface *term) {
    uint value;
    switch (_readParam(term, {"", "min", "max"}, value)) {
        case 0: // no parameter
            printf(term, INFO, F("Pmin is %u mbar"), Conf.scalerPmin());
            printf(term, INFO, F("Pmax is %u mbar"), Conf.scalerPmax());
            break;
        case 1: // min
            if (value == VAL_UNSET) {
                printf(term, INFO, F("Pmin is %u mbar"), Conf.scalerPmin());
                break;
            }
            if (Conf.scalerPmin(value))
                printf(term, INFO, F("Pmin set to %u mbar"), Conf.scalerPmin());
            else
                term->println(WARNING, F("Pmin left unchanged"));
            break;
        case 2: // max
            if (value == VAL_UNSET) {
                printf(term, INFO, F("Pmax is %u mbar"), Conf.scalerPmax());
                break;
            }
            if (Conf.scalerPmax(value))
                printf(term, INFO, F("Pmax set to %u mbar"), Conf.scalerPmax());
            else
                term->println(WARNING, F("Pmax left unchanged"));
            break;
    }

    term->prompt();
}

//-----------------------------------------------------------------------------
// Show/change the filtering parameters
//-----------------------------------------------------------------------------
//
STATIC void TelnetServer::cmdFilter(OutputInterface *term) {
    uint value;
    switch (_readParam(term, {"", "ns", "alpha"}, value)) {
        case 0: // no parameter
            printf(term, INFO, F("Number of samples is %u"), Conf.filtSamps());
            printf(term, INFO, F("Pressure EMA alpha is %u"), Conf.filtAlpha());
            break;
        case 1: // ns
            if (value == VAL_UNSET)
                printf(term, INFO, F("Number of samples is %u"), Conf.filtSamps());
            else
                Conf.filtSamps(value);
            break;
        case 2: // alpha
            if (value == VAL_UNSET)
                printf(term, INFO, F("Pressure EMA alpha is %u"), Conf.filtAlpha());
            else
                Conf.filtAlpha(value);
            break;
    }

    term->prompt();
}

//-----------------------------------------------------------------------------
// Show/change the pressure alert thresholds
//-----------------------------------------------------------------------------
//
STATIC void TelnetServer::cmdAlerts(OutputInterface *term) {
    uint value;
    switch (_readParam(term, {"", "lo", "hi", "hyst"}, value)) {
        case 0: // no parameter
            printf(term, INFO, F("Low pressure threshold is %u mbar"), Conf.alertLo());
            printf(term, INFO, F("High pressure threshold is %u mbar"), Conf.alertHi());
            printf(term, INFO, F("Back-to-normal hysteresis is %u mbar"), Conf.alertHyst());
            break;
        case 1: // lo
            if (value == VAL_UNSET)
                printf(term, INFO, F("Low pressure threshold is %u mbar"), Conf.alertLo());
            else
                Conf.alertLo(value);
            break;
        case 2: // hi
            if (value == VAL_UNSET)
                printf(term, INFO, F("High pressure threshold is %u mbar"), Conf.alertHi());
            else
                Conf.alertHi(value);
            break;
        case 3: // hyst
            if (value == VAL_UNSET)
                printf(term, INFO, F("Back-to-normal hysteresis is %u mbar"), Conf.alertHyst());
            else
                Conf.alertHyst(value);
            break;
    }

    term->prompt();
}

//-----------------------------------------------------------------------------
// Display the current settings and  other useful info
//-----------------------------------------------------------------------------
//
//#define STRING(x) #x
//#define GIT_REVISION "123"
STATIC void TelnetServer::cmdSys(OutputInterface *term) {
    printf(term, INFO, F("  Firmware version: %s"), GIT_REVISION);
    printf(term, INFO, F("        Flash time: %s"), BUILD_TIME);
    printf(term, INFO, F("           Chip ID: %X"), ESP.getChipId());
    printf(term, INFO, F("  Flash chip speed: %u MHz"), ESP.getFlashChipSpeed() / 1000000);
    printf(term, INFO, F("    Free heap size: %u bytes"), ESP.getFreeHeap());
    printf(term, INFO, F("Heap Fragmentation: %u %%"), ESP.getHeapFragmentation());
    printf(term, INFO, F("            Uptime: %s"), uptime(RolloverCount));
    printf(term, INFO, F("        Reset info: %s"), ESP.getResetInfo().c_str());

    term->prompt();
}

//-----------------------------------------------------------------------------
// Convert ADC value to pressure and vice versa with the current scaler parameters
//-----------------------------------------------------------------------------
//
STATIC void TelnetServer::cmdTestScale(OutputInterface *term) {
    uint value;
    int res;

    int i = _readParam(term, {"adc", "p"}, value);
    do {
        if (i < 0) break; // Command error

        if (value == VAL_UNSET) { // The value is required
            term->println(ERROR, F("Value cannot be empty"));
            break;
        }

        switch (i) {
            case 0: // adc: forward scaling
                res = scale(value, Conf.scalerAmin(), Conf.scalerAmax(), Conf.scalerPmin(), Conf.scalerPmax());
                printf(term, INFO, F("Output pressure is %d"), res, F(" mbar"));
                break;
            case 1: // p: backward scaling
                res = scale(value, Conf.scalerPmin(), Conf.scalerPmax(), Conf.scalerAmin(), Conf.scalerAmax());
                printf(term, INFO, F("Input ADC value is %d"), res, F(" mbar"));
                break;
        }
    } while (0);

    term->prompt();
}
