#pragma once

#include <Adafruit_SSD1306.h> // OLED display
#include <ESP8266WiFi.h>      // WiFi singleton
#include <Terminal.h>

class WiFiHelperClass {
  protected:
    Adafruit_SSD1306 &_display;
    const char *_apName;

  public:
    WiFiHelperClass(const char *ap_name, Adafruit_SSD1306 &display);
    void startWiFiManager();
    bool connectToSavedWiFi(unsigned timeout_s);
    String localIP() { return WiFi.localIP().toString(); };
    static String SSID() { return WiFi.SSID(); };
};
