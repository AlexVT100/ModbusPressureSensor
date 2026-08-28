#include <Terminal.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

void execCommand(const char *command);

class TelnetServer : public Terminal {
  protected:
    uint _port;
    WiFiServer _server;
    WiFiClient _client;
    bool _isConnected = false;

    static inline char buffer[160];
    static inline const uint VAL_UNSET = -1;

    // Callbacks for non-static functions
    std::function<void(OutputInterface *)> _cmdExitFunc;
    std::function<void(OutputInterface *)> _cmdRestartFunc;

  public:
    using Terminal::Terminal;
    TelnetServer(uint port);
    bool isConnected() { return _isConnected; }
    void setup();
    void loop();

  protected:
    static int _readParam(OutputInterface *terminal, std::initializer_list<const char *> params);
    static int _readParam(OutputInterface *terminal, std::initializer_list<const char *> params, uint &value);
    static bool _readValue(OutputInterface *terminal, uint &value);

    static void printf(OutputInterface *t, PRINT_TYPES type, const char *fmt, ...);
    static void printf(OutputInterface *t, PRINT_TYPES type, const __FlashStringHelper *fmt, ...);
    //static void printf(OutputInterface *t, COLOR color, const __FlashStringHelper *fmt, ...);

    void cmdExit(OutputInterface *terminal);
    void cmdRestart(OutputInterface *terminal);
    static void cmdConfig(OutputInterface *terminal);
    static void cmdMinADC(OutputInterface *terminal);
    static void cmdScalerADC(OutputInterface *terminal);
    static void cmdScalerPress(OutputInterface *terminal);
    static void cmdFilter(OutputInterface *terminal);
    static void cmdAlerts(OutputInterface *terminal);
    static void cmdSensor(OutputInterface *terminal);
    static void cmdDisp(OutputInterface *terminal);
    static void cmdSys(OutputInterface *terminal);
    static void cmdTestScale(OutputInterface *terminal);
};