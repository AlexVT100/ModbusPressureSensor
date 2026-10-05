// https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/server-class.html
// https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/client-class.html
//
// https://docs.arduino.cc/libraries/wifi/
// * https://docs.arduino.cc/libraries/wifi/#Server%20class
// * https://docs.arduino.cc/libraries/wifi/#Client%20class

#include <DNSServer.h>        // DNS server for the WiFi manager
#include <ESP8266WebServer.h> // The web server for WiFi manager
#include <WiFiManager.h>      // The WiFi manager

#include "WiFiHelper.h"

WiFiHelperClass::WiFiHelperClass(const char *ap_name, Adafruit_SSD1306 &display) :
    _display(display) {
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
// 
// If the connection is not established after timeout_s seconds,
// the device will be restarted.
//-----------------------------------------------------------------------------
//
bool WiFiHelperClass::connectToSavedWiFi(unsigned timeout_s) {
    // Attempt to connect to the saved WiFi network
    WiFi.mode(WIFI_STA);

    String chipID = String(ESP.getChipId(), HEX);
    chipID.toUpperCase();
    WiFi.hostname("PressureSensor-" + chipID);

    WiFi.begin();

    _display.clearDisplay();
    _display.setCursor(0, 0);
    _display.println("Connecting to WiFi...");
    _display.display();

    const int16_t cursor_x = _display.getCursorX();
    const int16_t cursor_y = _display.getCursorY();

    unsigned long finishTime = millis() + timeout_s * 1000;

    while (WiFi.status() != WL_CONNECTED && millis() < finishTime) {
        delay(500);

        _display.setCursor(cursor_x, cursor_y);
        _display.setTextColor(BLACK, WHITE);
        _display.printf_P("                     ");
        _display.setCursor(cursor_x, cursor_y);
        _display.setTextColor(WHITE, BLACK);
        _display.printf_P("restarting in %3lu sec", (finishTime - millis() + 500) / 1000);
        _display.display();
    }

    if (WiFi.status() == WL_CONNECTED) {
        _display.clearDisplay();
        _display.setCursor(0, 0);
        _display.println("Connected to WiFi");
        _display.println(WiFi.SSID());
        _display.display();
        delay(2000);

        return true;
    } else {
        _display.println("\nCould'n connect");
        _display.println("Restart after 5 sec");
        _display.display();
        delay(5000);
        ESP.restart();

        return false;
    }
}

// String WiFiHelperClass::SSID() { return WiFi.SSID(); }

// bool WiFiHelperClass::clientID(const WiFiClient &client, char (&buffer)[CLIENT_ID_SIZE]) {
//     if (!client.connected()) {
//         buffer[0] = '\0';
//         return false;
//     }
//     IPAddress ip = client.remoteIP();
//     snprintf(buffer, CLIENT_ID_SIZE, "%3u.%3u.%3u.%3u:%5u", ip[0], ip[1], ip[2], ip[3], client.remotePort());
//     return true;
// }
