// https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/server-class.html
// https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/client-class.html
//
// https://docs.arduino.cc/libraries/wifi/
// * https://docs.arduino.cc/libraries/wifi/#Server%20class
// * https://docs.arduino.cc/libraries/wifi/#Client%20class

#include <Esp.h>
#include <Server.h>
#include <DNSServer.h>        // DNS server for the WiFi manager
#include <ESP8266WebServer.h> // The web server for WiFi manager
#include <WiFiManager.h>      // The WiFi manager

#include "WiFiHelper.h"

WiFiHelperClass::WiFiHelperClass(const char *ap_name, Adafruit_SSD1306 &display) : _display(display) {
    _apName = ap_name;
}

//-----------------------------------------------------------------------------
// Start WiFiManager and save WiFi settings
//-----------------------------------------------------------------------------
//
void WiFiHelperClass::startWiFiManager() {
    _display.clearDisplay();
    _display.setCursor(0, 0);
    _display.println("AP MODE");
    _display.println(_apName);
    _display.display();

    WiFiManager wifiManager;
    wifiManager.resetSettings();

    // Start in AP mode to get WiFi settings
    if (!wifiManager.autoConnect(_apName)) {
        _display.println("Error!");
        _display.println("Restart after 5 sec");
        _display.display();
        delay(5000);
        ESP.restart();
    }
    Serial.println("Connected to WiFi:");
    Serial.println(WiFi.SSID());
}

//-----------------------------------------------------------------------------
// Connect to the saved WiFi network
//-----------------------------------------------------------------------------
//
bool WiFiHelperClass::connectToSavedWiFi() {
    // Attempt to connect to the saved WiFi network
    WiFi.mode(WIFI_STA);

    String chipID = String(ESP.getChipId(), HEX);
    chipID.toUpperCase();
    WiFi.hostname("PressureSensor-" + chipID);

    WiFi.begin();

    unsigned long startAttemptTime = millis();
    const unsigned long timeout = 10000; // Timeout in ms

    _display.clearDisplay();
    _display.setCursor(0, 0);
    _display.print("WiFi Init");
    _display.display();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
        delay(500);
        _display.print(".");
        _display.display();
    }

    if (WiFi.status() == WL_CONNECTED) {
        _display.clearDisplay();
        _display.setCursor(0, 0);
        _display.println("Connected to WiFi");
        _display.println(WiFi.SSID());
        _display.display();
        delay(2000);
        _localIP = WiFi.localIP();

        //_server.begin();

        return true;
    } else {
        _display.println("\nNot connected");
        _display.display();
        return false;
    }
}

String WiFiHelperClass::SSID() { return WiFi.SSID(); }

// bool WiFiHelperClass::clientID(const WiFiClient &client, char (&buffer)[CLIENT_ID_SIZE]) {
//     if (!client.connected()) {
//         buffer[0] = '\0';
//         return false;
//     }
//     IPAddress ip = client.remoteIP();
//     snprintf(buffer, CLIENT_ID_SIZE, "%3u.%3u.%3u.%3u:%5u", ip[0], ip[1], ip[2], ip[3], client.remotePort());
//     return true;
// }
