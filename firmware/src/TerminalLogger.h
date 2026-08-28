#pragma once

#include <Terminal.h>

// Note about Terminal library:
//      Terminal::print(PRINT_TYPES type, ...) group prints colored text WITHOUT type prefixes [...]
//      Terminal::println(PRINT_TYPES type, ...) group prints colored text WITH type prefixes [...]

class TerminalLogger {
  protected:
    Terminal _serial;
    Terminal *_telnet = nullptr;
    static inline char buffer[160]; // Buffer for snprintf()

  public:
    TerminalLogger() :
        _serial(&Serial) {}

    TerminalLogger(Terminal &telnet) :
        _serial(&Serial) {
        _telnet = &telnet;
    }

    void setup() { _serial.setup(); }

    void printf(PRINT_TYPES type, const __FlashStringHelper *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vsnprintf_P(buffer, sizeof(buffer), (PGM_P)fmt, args);
        va_end(args);

        _serial.println(type, buffer);
        if (_telnet) _telnet->println(type, buffer);
    }

    void println(PRINT_TYPES type, String line) {
        _serial.println(type, line);
        if (_telnet) _telnet->println(type, line);
    }
};
