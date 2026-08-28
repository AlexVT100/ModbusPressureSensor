#pragma once

#include <WiFiClient.h>
#include <WiFiServer.h>
#include <Adafruit_SSD1306.h> // OLED display
#include <ESP8266WiFi.h>      // IPAddress
#include <Terminal.h>

class WiFiHelperClass {
  protected:
    Adafruit_SSD1306 &_display;
    const char *_apName;
    IPAddress _localIP = INADDR_NONE;

  public:
    WiFiHelperClass(const char *ap_name, Adafruit_SSD1306 &display);
    void startWiFiManager();
    bool connectToSavedWiFi();
    String localIP() { return _localIP.toString(); };
    static String SSID();
};
